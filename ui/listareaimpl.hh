// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LIST_AREA_IMPL_HH__
#define __RAPICORN_LIST_AREA_IMPL_HH__

#include <ui/listarea.hh>
#include <ui/container.hh>
#include <ui/adjustment.hh>
#include <ui/layoutcontainers.hh>
#include <ui/table.hh>
#include <ui/models.hh>
#include <deque>

namespace Rapicorn {

struct ListRow {
  vector<ItemImpl*> cols;
  ContainerImpl *rowbox;
  Allocation     area;
  uint           allocated : 1;
  ListRow() : rowbox (NULL), allocated (0) {}
};

struct ModelSizes {
  vector<int>    row_sizes;
  map<int64,int> size_cache; // ca. 28bytes per node
  int64          total_height;
  ListRow       *measurement_row;
  explicit ModelSizes() : total_height (0), measurement_row (NULL) {}
  void     clear     () { row_sizes.clear(); size_cache.clear(); total_height = 0; }
};

class ItemListImpl : public virtual MultiContainerImpl,
                     public virtual ItemList,
                     public virtual AdjustmentSource,
                     public virtual EventHandler
{
  typedef map<int64,ListRow*>  RowMap;
  typedef std::deque<int>      SizeQueue;
  ListModelIface        *m_model;
  vector<bool>           m_selection;
  mutable Adjustment    *m_hadjustment, *m_vadjustment;
  bool                   m_browse;
  uint                   m_n_cols;
  RowMap                 m_row_map;
  vector<ListRow*>       m_row_cache;
  vector<SizeGroup*>     m_size_groups;
  bool                   m_need_resize_scroll;
  bool                   m_block_invalidate;
  uint64                 m_current_row;
  ModelSizes             m_model_sizes;
  void                  model_inserted          (int first, int last);
  void                  model_changed           (int first, int last);
  void                  model_removed           (int first, int last);
  void                  selection_changed       (int first, int last);
  virtual void          invalidate_parent ();
protected:
  virtual bool          handle_event            (const Event    &event);
  virtual void          reset                   (ResetMode       mode);
  virtual bool          can_focus               () const { return true; }
  SelectionMode         selection_mode          () { return SELECTION_MULTIPLE; }
  bool                  selected                (int row) { return size_t (row) < m_selection.size() && m_selection[row]; }
  void                  toggle_selected         (int row);
public:
  explicit              ItemListImpl            ();
  virtual              ~ItemListImpl            ();
  virtual void          constructed             ();
  virtual bool          browse                  () const        { return m_browse; }
  virtual void          browse                  (bool b)        { m_browse = b; invalidate(); }
  virtual void          model                   (const String &modelurl);
  virtual String        model                   () const;
  virtual void          hierarchy_changed       (ItemImpl *old_toplevel);
  Adjustment&           hadjustment             () const;
  Adjustment&           vadjustment             () const;
  Adjustment*           get_adjustment          (AdjustmentSourceType adj_source,
                                                 const String        &name);
  void                  invalidate_model        (bool invalidate_heights,
                                                 bool invalidate_widgets);
  virtual void          visual_update           ();
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area, bool changed);
  /* sizing and positioning */
  bool                  pixel_positioning       (const int64       mcount,
                                                 const ModelSizes &ms) const;
  void                  measure_rows            (int64    maxpixels);
  int64                 position2row            (double   list_fraction,
                                                 double  *row_fraction);
  double                row2position            (int64    list_row,
                                                 double   list_alignment = 0.5);
  int                   row_height              (ModelSizes &ms,
                                                 int64       list_row);
  int64                 row_layout              (double      vscrollpos,
                                                 int64       mcount,
                                                 ModelSizes &ms,
                                                 int64       list_row);

  void                  measure_rows            (int64    maxpixels,
                                                 double   fraction);
  int64                 scroll_row_layout       (ListRow *lr_current,
                                                 int64 *scrollrowy,
                                                 int64 *scrollrowupper,
                                                 int64 *scrollrowlower,
                                                 int64 *listupperp,
                                                 int64 *listheightp);
  void                  resize_scroll           ();
  void                  resize_scroll_preserving();
  void                  cache_row               (ListRow *lr);
  void                  fill_row                (ListRow *lr,
                                                 uint64   row);
  ListRow*              create_row              (uint64 row,
                                                 bool   with_size_groups = true);
  ListRow*              lookup_row              (uint64 row);
  ListRow*              fetch_row               (uint64 row);
  uint64                measure_row             (ListRow *lr,
                                                 uint64  *allocation_offset = NULL);
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_IMPL_HH__ */
