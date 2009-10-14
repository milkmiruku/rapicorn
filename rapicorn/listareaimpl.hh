/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_LIST_AREA_IMPL_HH__
#define __RAPICORN_LIST_AREA_IMPL_HH__

#include <rapicorn/listarea.hh>
#include <rapicorn/containerimpl.hh>
#include <rapicorn/adjustment.hh>
#include <rapicorn/table.hh>

namespace Rapicorn {

struct ListRow {
  vector<Item*> cols;
};

class ItemListImpl : public virtual SingleContainerImpl,
                     public virtual ItemList,
                     public virtual AdjustmentSource
{
  typedef map<uint64,ListRow*> RowMap;
  Table                 *m_table;
  Model1                *m_model;
  mutable Adjustment    *m_hadjustment, *m_vadjustment;
  uint                   m_n_cols;
  RowMap                 m_row_map;
  vector<ListRow*>       m_row_cache;
  bool                   m_browse;
public:
  explicit              ItemListImpl            ();
  virtual              ~ItemListImpl            ();
  virtual void          constructed             ();
  virtual bool          browse                  () const        { return m_browse; }
  virtual void          browse                  (bool b)        { m_browse = b; invalidate(); }
  virtual void          model                   (const String &modelurl);
  virtual String        model                   () const;
  virtual void          hierarchy_changed       (Item *old_toplevel);
  Adjustment&           hadjustment             () const;
  Adjustment&           vadjustment             () const;
  Adjustment*           get_adjustment          (AdjustmentSourceType adj_source,
                                                 const String        &name);
  void                  invalidate_model        (bool invalidate_heights,
                                                 bool invalidate_widgets);
  virtual void          visual_update           ();
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area);
  void                  cache_row               (ListRow *lr);
  void                  fill_row                (ListRow *lr,
                                                 uint64   row);
  ListRow*              create_row              (uint64 row);
  ListRow*              fetch_row               (uint64 row);
  void                  position_row            (ListRow *lr,
                                                 uint64   visible_slot);
  uint64                measure_row             (ListRow *lr,
                                                 uint64  *allocation_offset = NULL);
  uint64                get_scroll_item         (double *row_offsetp,
                                                 double *pixel_offsetp);
  bool                  need_pixel_scrolling    ();
  void                  layout_list             ();
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_IMPL_HH__ */
