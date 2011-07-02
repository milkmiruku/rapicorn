/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_CONTAINER_IMPL_HH__
#define __RAPICORN_CONTAINER_IMPL_HH__

#include <ui/container.hh>

namespace Rapicorn {

/* --- Single Child Container Impl --- */
class SingleContainerImpl : public virtual ItemImpl, public virtual Container {
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
class MultiContainerImpl : public virtual ItemImpl, public virtual Container {
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

#endif  /* __RAPICORN_CONTAINER_IMPL_HH__ */
