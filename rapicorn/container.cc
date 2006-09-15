/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "container.hh"
#include "containerimpl.hh"
#include "root.hh"
using namespace std;

namespace Rapicorn {

/* --- CrossLinks --- */
struct CrossLink {
  Item                             *owner, *link;
  Signals::Trampoline1<void,Item&> *uncross;
  CrossLink                        *next;
  explicit      CrossLink (Item                             *o,
                           Item                             *l,
                           Signals::Trampoline1<void,Item&> &it) :
    owner (o), link (l),
    uncross (ref_sink (&it)), next (NULL)
  {}
  /*Des*/       ~CrossLink()
  {
    unref (uncross);
  }
  BIRNET_PRIVATE_CLASS_COPY (CrossLink);
};
struct CrossLinks {
  Container *container;
  CrossLink *links;
};
static inline void      container_uncross_link_R        (Container *container,
                                                         CrossLink **clinkp,
                                                         bool        notify_callback = true);
struct CrossLinksKey : public DataKey<CrossLinks*> {
  virtual void
  destroy (CrossLinks *clinks)
  {
    while (clinks->links)
      container_uncross_link_R (clinks->container, &clinks->links);
    delete (clinks);
  }
};
static CrossLinksKey cross_links_key;

struct UncrossNode {
  UncrossNode *next;
  Container   *mutable_container;
  CrossLink   *clink;
  explicit      UncrossNode (Container *xcontainer,
                             CrossLink *xclink) :
    next (NULL), mutable_container (xcontainer), clink (xclink)
  {}
  BIRNET_PRIVATE_CLASS_COPY (UncrossNode);
};
static UncrossNode *uncross_callback_stack = NULL;
static Mutex        uncross_callback_stack_mutex;

void
Container::item_cross_link (Item           &owner,
                            Item           &link,
                            const ItemSlot &uncross)
{
#ifdef PARANOID
  assert (&owner != &link);
  assert (owner.common_ancestor (link) == this);
#endif
  CrossLinks *clinks = get_data (&cross_links_key);
  if (!clinks)
    {
      clinks =  new CrossLinks();
      clinks->container = this;
      clinks->links = NULL;
      set_data (&cross_links_key, clinks);
    }
  CrossLink *clink = new CrossLink (&owner, &link, *uncross.get_trampoline());
  clink->next = clinks->links;
  clinks->links = clink;
}

void
Container::item_cross_unlink (Item           &owner,
                              Item           &link,
                              const ItemSlot &uncross)
{
  bool found_one = false;
  ref (this);
  ref (owner);
  ref (link);
  /* _first_ check whether a currently uncrossing link (recursing from
   * container_uncross_link_R()) needs to be unlinked.
   */
  uncross_callback_stack_mutex.lock();
  for (UncrossNode *unode = uncross_callback_stack; unode; unode = unode->next)
    if (unode->mutable_container == this &&
        unode->clink->owner == &owner &&
        unode->clink->link == &link &&
        *unode->clink->uncross == *uncross.get_trampoline())
      {
        unode->mutable_container = NULL; /* prevent more cross_unlink() calls */
        found_one = true;
        break;
      }
  uncross_callback_stack_mutex.unlock();
  if (!found_one)
    {
      CrossLinks *clinks = get_data (&cross_links_key);
      for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
        if (clink->owner == &owner &&
            clink->link == &link &&
            *clink->uncross == *uncross.get_trampoline())
          {
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links, false);
            found_one = true;
            break;
          }
    }
  if (!found_one)
    throw Exception ("no cross link from \"" + owner.name() + "\" to \"" + link.name() + "\" on \"" + name() + "\" to remove");
  unref (link);
  unref (owner);
  unref (this);
}

void
Container::item_uncross_links (Item &owner,
                               Item &link)
{
  ref (this);
  ref (owner);
  ref (link);
 restart_search:
  CrossLinks *clinks = get_data (&cross_links_key);
  for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
    if (clink->owner == &owner &&
        clink->link == &link)
      {
        container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
        clinks = get_data (&cross_links_key);
        goto restart_search;
      }
  unref (link);
  unref (owner);
  unref (this);
}

static inline bool
item_has_ancestor (const Item *item,
                   const Item *ancestor)
{
  /* this duplicates item->has_ancestor() to optimize speed and
   * to cover the case where item == ancestor.
   */
  do
    if (item == ancestor)
      return true;
    else
      item = item->parent();
  while (item);
  return false;
}

void
Container::uncross_descendant (Item &descendant)
{
#ifdef PARANOID
  assert (descendant.has_ancestor (*this));
#endif
  Item *item = &descendant;
  ref (this);
  ref (item);
  Container *cc = dynamic_cast<Container*> (item);
 restart_search:
  CrossLinks *clinks = get_data (&cross_links_key);
  if (!cc || !cc->has_children()) /* suppress tree walks where possible */
    for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
      {
        if (clink->owner == item || clink->link == item)
          {
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
            goto restart_search;
          }
      }
  else /* need to check whether item is ancestor of any of our cross-link items */
    {
      /* we do some minor hackery here, for optimization purposes. since item
       * is a descendant of this container, we don't need to walk ->owner's or
       * ->link's ancestor lists any further than up to reaching this container.
       * to suppress extra checks in item_has_ancestor() in this regard, we
       * simply set parent() to NULL temporarily and with that cause
       * item_has_ancestor() to return earlier.
       */
      Item *saved_parent = *_parent_loc();
      *_parent_loc() = NULL;
      for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
        if (item_has_ancestor (clink->owner, item) ||
            item_has_ancestor (clink->link, item))
          {
            *_parent_loc() = saved_parent;
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
            goto restart_search;
          }
      *_parent_loc() = saved_parent;
    }
  unref (item);
  unref (this);
}

static inline void
container_uncross_link_R (Container *container,
                          CrossLink **clinkp,
                          bool        notify_callback)
{
  CrossLink *clink = *clinkp;
  /* remove cross link */
  *clinkp = clink->next;
  /* notify */
  if (notify_callback)
    {
      /* record execution */
      UncrossNode unode (container, clink);
      uncross_callback_stack_mutex.lock();
      unode.next = uncross_callback_stack;
      uncross_callback_stack = &unode;
      uncross_callback_stack_mutex.unlock();
      /* exec callback, note that this may recurse */
      (*clink->uncross) (*clink->link);
      /* unrecord execution */
      uncross_callback_stack_mutex.lock();
      UncrossNode *walk, *last = NULL;
      for (walk = uncross_callback_stack; walk; last = walk, walk = last->next)
        if (walk == &unode)
          {
            if (!last)
              uncross_callback_stack = unode.next;
            else
              last->next = unode.next;
            break;
          }
      uncross_callback_stack_mutex.unlock();
      assert (walk != NULL); /* paranoid */
    }
  /* delete cross link */
  delete clink;
}

/* --- Container --- */
const PropertyList&
Container::list_properties()
{
  static Property *properties[] = {
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

const CommandList&
Container::list_commands()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands, Item::list_commands());
  return command_list;
}

static DataKey<Container*> child_container_key;

void
Container::child_container (Container *child_container)
{
  if (child_container && !child_container->has_ancestor (*this))
    throw Exception ("child container is not descendant of container \"", name(), "\": ", child_container->name());
  set_data (&child_container_key, child_container);
}

Container&
Container::child_container ()
{
  Container *container = get_data (&child_container_key);
  if (!container)
    container = this;
  return *container;
}

void
Container::add (Item                   &item,
                const PackPropertyList &pack_plist,
                PackPropertyList       *unused_props)
{
  if (item.parent())
    throw Exception ("not adding item with parent: ", item.name());
  Container &container = child_container();
  if (this != &container)
    {
      container.add (item, pack_plist, unused_props);
      return;
    }
  item.ref();
  try {
    container.add_child (item);
  } catch (...) {
    item.unref();
    throw;
  }
  /* always run packer code */
  Packer packer = create_packer (item);
  packer.apply_properties (pack_plist, unused_props);
  /* can invalidate etc. the fully setup item now */
  item.invalidate();
  item.unref();
}

void
Container::add (Item                   *item,
                const PackPropertyList &pack_plist,
                PackPropertyList       *unused_props)
{
  if (!item)
    throw NullPointer();
  add (*item, pack_plist, unused_props);
}

void
Container::remove (Item &item)
{
  Container *container = item.parent_container();
  if (!container)
    throw NullPointer();
  item.ref();
  item.invalidate();
  Container *dcontainer = container;
  while (dcontainer)
    {
      dcontainer->dispose_item (item);
      dcontainer = dcontainer->parent_container();
    }
  container->remove_child (item);
  item.unref();
}

Affine
Container::child_affine (Item &item)
{
  return Affine(); // Identity
}

void
Container::hierarchy_changed (Item *old_toplevel)
{
  Item::hierarchy_changed (old_toplevel);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    cw->sig_hierarchy_changed.emit (old_toplevel);
}

void
Container::dispose_item (Item &item)
{
  if (&item == get_data (&child_container_key))
    child_container (NULL);
}

static DataKey<Item*> focus_child_key;

void
Container::unparent_child (Item &item)
{
  if (&item == get_data (&focus_child_key))
    delete_data (&focus_child_key);
  Container *ancestor = this;
  do
    {
      ancestor->uncross_descendant (item);
      ancestor = ancestor->parent_container();
    }
  while (ancestor);
}

void
Container::set_focus_child (Item *item)
{
  if (!item)
    delete_data (&focus_child_key);
  else
    {
      assert (item->parent() == this);
      set_data (&focus_child_key, item);
    }
}

Item*
Container::get_focus_child ()
{
  return get_data (&focus_child_key);
}

struct LesserItemByHBand {
  bool
  operator() (Item *const &i1,
              Item *const &i2) const
  {
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    /* sort items by horizontal bands first */
    if (a1.y >= a2.y + a2.height)
      return true;
    if (a1.y + a1.height <= a2.y)
      return false;
    /* sort items with overlapping horizontal bands by vertical position */
    if (a1.x != a2.x)
      return a1.x < a2.x;
    /* resort to center */
    Point m1 (a1.x + a1.width * 0.5, a1.y + a1.height * 0.5);
    Point m2 (a2.x + a2.width * 0.5, a2.y + a2.height * 0.5);
    if (m1.y != m2.y)
      return m1.y < m2.y;
    else
      return m1.x < m2.x;
  }
};

struct LesserItemByDirection {
  FocusDirType dir;
  Point        middle;
  Item        *last_item;
  LesserItemByDirection (FocusDirType d,
                         const Point &p,
                         Item        *li) :
    dir (d), middle (p), last_item (li)
  {}
  double
  directional_distance (const Allocation &a) const
  {
    switch (dir)
      {
      case FOCUS_RIGHT:
        return a.x - middle.x;
      case FOCUS_UP:
        return a.y - middle.y;
      case FOCUS_LEFT:
        return middle.x - (a.x + a.width);
      case FOCUS_DOWN:
        return middle.y - (a.y + a.height);
      default:
        return -1;      /* unused */
      }
  }
  static inline Rect
  rect_from_allocation (const Allocation &a)
  {
    return Rect (Point (a.x, a.y), a.width, a.height);
  }
  bool
  operator() (Item *const &i1,
              Item *const &i2) const
  {
    /* calculate item distances along dir, dist >= 0 lies ahead */
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    double dd1 = directional_distance (a1);
    double dd2 = directional_distance (a2);
    /* current focus item comes last in the list of negative distances */
    if (dd1 < 0 && dd2 < 0 && (last_item == i1 || last_item == i2))
      return last_item == i2;
    /* sort items along dir */
    if (dd1 != dd2)
      return dd1 < dd2;
    /* same horizontal/vertical band distance, sort by closest edge distance */
    dd1 = rect_from_allocation (a1).dist (middle);
    dd2 = rect_from_allocation (a2).dist (middle);
    if (dd1 != dd2)
      return dd1 < dd2;
    /* same edge distance, resort to center distance */
    dd1 = middle.dist (Point (a1.x + a1.width * 0.5, a1.y + a1.height * 0.5));
    dd2 = middle.dist (Point (a2.x + a2.width * 0.5, a2.y + a2.height * 0.5));
    return dd1 < dd2;
  }
};

static inline Point
rect_center (const Allocation &a)
{
  return Point (a.x + a.width * 0.5, a.y + a.height * 0.5);
}

bool
Container::move_focus (FocusDirType fdir,
                       bool         reset_history)
{
  /* check focus ability */
  if (!visible() || !sensitive())
    return false;
  Item *last_child = get_data (&focus_child_key);
  /* let last focus descendant handle movement */
  if (last_child && last_child->move_focus (fdir, reset_history))
    return true;
  /* copy children */
  vector<Item*> children;
  ChildWalker lw = local_children();
  while (lw.has_next())
    children.push_back (&*lw++);
  /* sort children according to direction and current focus */
  const Allocation &area = allocation();
  Point upper_left (area.x, area.y + area.height);
  Point lower_right (area.x + area.width, area.y);
  Point refpoint;
  switch (fdir)
    {
      Item *current;
    case FOCUS_NEXT:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      break;
    case FOCUS_PREV:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      reverse (children.begin(), children.end());
      break;
    case FOCUS_UP:
    case FOCUS_LEFT:
      current = root()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : lower_right;
      stable_sort (children.begin(), children.end(), LesserItemByDirection (fdir, refpoint, last_child));
      break;
    case FOCUS_RIGHT:
    case FOCUS_DOWN:
      current = root()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : upper_left;
      stable_sort (children.begin(), children.end(), LesserItemByDirection (fdir, refpoint, last_child));
      break;
    }
  /* skip children beyond last focus descendant */
  Walker<Item*> cw = walker (children);
  if (last_child)
    while (cw.has_next())
      if (last_child == *cw++)
        break;
  /* let remaining descendants handle movement */
  while (cw.has_next())
    {
      Item *child = *cw;
      if (child->move_focus (fdir, reset_history))
        return true;
      cw++;
    }
  /* no descendant accepts focus */
  if (reset_history)
    delete_data (&focus_child_key);
  return false;
}

void
Container::point_children (Point               p, /* root coordinates relative */
                           std::vector<Item*> &stack)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Item &child = *cw;
      Point cp = child_affine (child).point (p);
      if (child.point (cp))
        {
          child.ref();
          stack.push_back (&child);
          Container *cc = dynamic_cast<Container*> (&child);
          if (cc)
            cc->point_children (cp, stack);
        }
    }
}

void
Container::root_point_children (Point                   p, /* root coordinates relative */
                                std::vector<Item*>     &stack)
{
  point_children (point_from_root (p), stack);
}

bool
Container::match_interface (InterfaceMatch &imatch,
                            const String   &ident)
{
  if (imatch.done() ||
      sig_find_interface.emit (imatch, ident) ||
      ((!ident[0] || ident == name()) && imatch.match (this)))
    return true;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->match_interface (imatch, ident))
      break;
  return imatch.done();
}

void
Container::render (Display &display)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->drawable())
        continue;
      const Allocation area = cw->allocation();
      display.push_clip_rect (area.x, area.y, area.width, area.height);
      if (cw->test_flags (INVALID_REQUISITION))
        warning ("rendering item with invalid %s: %s (%p)", "requisition", cw->name().c_str(), &*cw);
      if (cw->test_flags (INVALID_ALLOCATION))
        warning ("rendering item with invalid %s: %s (%p)", "allocation", cw->name().c_str(), &*cw);
      if (!display.empty())
        cw->render (display);
      display.pop_clip_rect();
    }
}

void
Container::debug_tree (String indent)
{
  printf ("%s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), this->name().c_str(), this,
          allocation().width, allocation().height, allocation().x, allocation().y);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Item &child = *cw;
      Container *c = dynamic_cast<Container*> (&child);
      if (c)
        c->debug_tree (indent + "  ");
      else
        printf ("  %s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), child.name().c_str(), &child,
                child.allocation().width, child.allocation().height, child.allocation().x, child.allocation().y);
    }
}

Container::ChildPacker::ChildPacker ()
{}

Container::Packer::Packer (ChildPacker *cp) :
  m_child_packer (ref_sink (cp))
{}

Container::Packer::Packer (const Packer &src) :
  m_child_packer (ref_sink (src.m_child_packer))
{}

Property*
Container::Packer::lookup_property (const String &property_name)
{
  typedef std::map<const String, Property*> PropertyMap;
  static std::map<const PropertyList*,PropertyMap*> plist_map;
  /* find/construct property map */
  const PropertyList &plist = list_properties();
  PropertyMap *pmap = plist_map[&plist];
  if (!pmap)
    {
      pmap = new PropertyMap;
      for (uint i = 0; i < plist.n_properties; i++)
        (*pmap)[plist.properties[i]->ident] = plist.properties[i];
      plist_map[&plist] = pmap;
    }
  PropertyMap::iterator it = pmap->find (property_name);
  if (it != pmap->end())
    return it->second;
  else
    return NULL;
}

String
Container::Packer::get_property (const String   &property_name)
{
  Property *prop = lookup_property (property_name);
  if (!prop)
    throw Exception ("no such property: ", property_name);
  m_child_packer->update();
  return prop->get_value (m_child_packer);
}

void
Container::Packer::set_property (const String    &property_name,
                                 const String    &value,
                                 const nothrow_t &nt)
{
  Property *prop = lookup_property (property_name);
  if (prop)
    {
      m_child_packer->update();
      prop->set_value (m_child_packer, value);
      m_child_packer->commit();
    }
  else if (&nt == &dothrow)
    throw Exception ("no such property: ", property_name);
}

void
Container::Packer::apply_properties (const PackPropertyList &pack_plist,
                                     PackPropertyList       *unused_props)
{
  m_child_packer->update();
  for (PackPropertyList::const_iterator it = pack_plist.begin(); it != pack_plist.end(); it++)
    {
      Property *prop = lookup_property (it->first);
      if (prop)
        prop->set_value (m_child_packer, it->second);
      else if (unused_props)
        (*unused_props)[it->first] = it->second;
      else
        throw Exception ("no such pack property: " + it->first);
    }
  m_child_packer->commit();
}

const PropertyList&
Container::Packer::list_properties()
{
  return m_child_packer->list_properties();
}

Container::Packer::~Packer ()
{
  unref (m_child_packer);
}

Container::Packer
Container::child_packer (Item &item)
{
  Container *container = item.parent_container();
  if (!container)
    throw NullPointer();
  return container->create_packer (item);
}

Container::ChildPacker*
Container::void_packer ()
{
  class PackerSingleton : public ChildPacker {
    PackerSingleton() { ref_sink(); }
    PRIVATE_CLASS_COPY (PackerSingleton);
  public:
    static PackerSingleton*
    dummy_packer()
    {
      static PackerSingleton *singleton = new PackerSingleton;
      return singleton;
    }
    ~PackerSingleton() { assert_not_reached(); }
    virtual const PropertyList&
    list_properties ()
    {
      static Property *properties[] = { };
      static const PropertyList property_list (properties);
      return property_list;
    }
    virtual void update () {}
    virtual void commit () {}
  };
  return PackerSingleton::dummy_packer();
}

SingleContainerImpl::SingleContainerImpl () :
  child_item (NULL)
{}

Container::ChildWalker
SingleContainerImpl::local_children ()
{
  Item **iter = &child_item, **iend = iter;
  if (child_item)
    iend++;
  return value_walker (PointerIterator<Item*> (iter), PointerIterator<Item*> (iend));
}

void
SingleContainerImpl::add_child (Item &item)
{
  if (child_item)
    throw Exception ("invalid attempt to add child \"", item.name(), "\" to single-child container \"", name(), "\" ",
                     "which already has a child \"", child_item->name(), "\"");
  item.ref_sink();
  item.set_parent (this);
  child_item = &item;
}

void
SingleContainerImpl::remove_child (Item &item)
{
  assert (child_item == &item); /* ensured by remove() */
  child_item = NULL;
  item.set_parent (NULL);
  item.unref();
}

void
SingleContainerImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_visible_child())
    {
      Item &child = get_child();
      requisition = child.size_request ();
      chspread = child.hspread();
      cvspread = child.vspread();
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
SingleContainerImpl::size_allocate (Allocation area)
{
  allocation (area);
  if (has_visible_child())
    {
      Item &child = get_child();
      child.set_allocation (area);
    }
}

Container::Packer
SingleContainerImpl::create_packer (Item &item)
{
  return void_packer(); /* no child properties */
}    

SingleContainerImpl::~SingleContainerImpl()
{
  while (child_item)
    remove (child_item);
}

MultiContainerImpl::MultiContainerImpl ()
{}

void
MultiContainerImpl::add_child (Item &item)
{
  item.ref_sink();
  item.set_parent (this);
  items.push_back (&item);
}

void
MultiContainerImpl::remove_child (Item &item)
{
  vector<Item*>::iterator it;
  for (it = items.begin(); it != items.end(); it++)
    if (*it == &item)
      {
        items.erase (it);
        item.set_parent (NULL);
        item.unref();
        return;
      }
  assert_not_reached();
}

MultiContainerImpl::~MultiContainerImpl()
{
  while (items.size())
    remove (*items[items.size() - 1]);
}

} // Rapicorn
