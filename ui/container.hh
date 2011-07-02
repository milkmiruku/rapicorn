// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CONTAINER_HH__
#define __RAPICORN_CONTAINER_HH__

#include <ui/item.hh>

namespace Rapicorn {

/* --- Container --- */
struct ContainerImpl : public virtual ItemImpl {
  friend              class ItemImpl;
  friend              class WindowImpl;
  void                uncross_descendant(ItemImpl          &descendant);
  void                item_cross_link   (ItemImpl           &owner,
                                         ItemImpl           &link,
                                         const ItemSlot &uncross);
  void                item_cross_unlink (ItemImpl           &owner,
                                         ItemImpl           &link,
                                         const ItemSlot &uncross);
  void                item_uncross_links(ItemImpl           &owner,
                                         ItemImpl           &link);
protected:
  virtual            ~ContainerImpl     ();
  virtual void        add_child         (ItemImpl           &item) = 0;
  virtual void        repack_child      (ItemImpl           &item,
                                         const PackInfo &orig,
                                         const PackInfo &pnew);
  virtual void        remove_child      (ItemImpl           &item) = 0;
  virtual void        unparent_child    (ItemImpl           &item);
  virtual void        dispose_item      (ItemImpl           &item);
  virtual void        hierarchy_changed (ItemImpl           *old_toplevel);
  virtual bool        move_focus        (FocusDirType    fdir);
  void                expose_enclosure  (); /* expose without children */
  virtual void        set_focus_child   (ItemImpl           *item);
  virtual void        dump_test_data    (TestStream     &tstream);
public:
  ItemImpl*               get_focus_child   () const;
  typedef Walker<ItemImpl>  ChildWalker;
  void                  child_container (ContainerImpl  *child_container);
  ContainerImpl&        child_container ();
  virtual ChildWalker   local_children  () const = 0;
  virtual bool          has_children    () = 0;
  void                  remove          (ItemImpl           &item);
  void                  remove          (ItemImpl           *item)  { RAPICORN_CHECK (item != NULL); remove (*item); }
  void                  add             (ItemImpl                   &item);
  void                  add             (ItemImpl                   *item);
  virtual Affine        child_affine    (const ItemImpl             &item); /* container => item affine */
  virtual
  const PropertyList&   list_properties (); /* essentially chaining to ItemImpl:: */
  const CommandList&    list_commands   (); /* essentially chaining to ItemImpl:: */
  virtual void          point_children  (Point                   p, /* item coordinates relative */
                                         std::vector<ItemImpl*>     &stack);
  void         viewp0rt_point_children  (Point                   p, /* viewp0rt coordinates relative */
                                         std::vector<ItemImpl*>     &stack);
  virtual void          render          (Display                &display);
  void                  debug_tree      (String indent = String());
};

/* --- Single Child Container Impl --- */
class SingleContainerImpl : public virtual ItemImpl, public virtual ContainerImpl {
  ItemImpl             *child_item;
protected:
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area);
  ItemImpl&             get_child               () { RAPICORN_CHECK (child_item != NULL); return *child_item; }
  virtual void          pre_finalize            ();
  virtual              ~SingleContainerImpl     ();
  virtual ChildWalker   local_children          () const;
  virtual bool          has_children            () { return child_item != NULL; }
  bool                  has_visible_child       () { return child_item && child_item->visible(); }
  bool                  has_drawable_child      () { return child_item && child_item->drawable(); }
  bool                  has_allocatable_child   () { return child_item && child_item->allocatable(); }
  virtual void          add_child               (ItemImpl   &item);
  virtual void          remove_child            (ItemImpl   &item);
  Allocation            layout_child            (ItemImpl         &child,
                                                 const Allocation &carea);
  explicit              SingleContainerImpl     ();
};

/* --- Multi Child Container Impl --- */
class MultiContainerImpl : public virtual ItemImpl, public virtual ContainerImpl {
  std::vector<ItemImpl*>    items;
protected:
  virtual void          pre_finalize            ();
  virtual              ~MultiContainerImpl      ();
  virtual ChildWalker   local_children          () const { return value_walker (items); }
  virtual bool          has_children            () { return items.size() > 0; }
  virtual void          add_child               (ItemImpl   &item);
  virtual void          remove_child            (ItemImpl   &item);
  void                  raise_child             (ItemImpl   &item);
  void                  lower_child             (ItemImpl   &item);
  void                  remove_all_children     ();
  explicit              MultiContainerImpl      ();
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
