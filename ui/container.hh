// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CONTAINER_HH__
#define __RAPICORN_CONTAINER_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class ResizeContainerImpl;

// == Container ==
struct ContainerImpl : public virtual WidgetImpl, public virtual ContainerIface {
  friend              class WidgetImpl;
  friend              class WindowImpl;
  void                uncross_descendant    (WidgetImpl          &descendant);
  size_t              widget_cross_link     (WidgetImpl           &owner,
                                             WidgetImpl           &link,
                                             const WidgetSlot &uncross);
  void                widget_cross_unlink   (WidgetImpl           &owner,
                                             WidgetImpl           &link,
                                             size_t              link_id);
  void                widget_uncross_links  (WidgetImpl           &owner,
                                             WidgetImpl           &link);
  WidgetGroup*        retrieve_widget_group (const String &group_name, WidgetGroupType group_type, bool force_create);
protected:
  virtual            ~ContainerImpl     ();
  virtual void        add_child         (WidgetImpl           &widget) = 0;
  virtual void        repack_child      (WidgetImpl &widget, const PackInfo &orig, const PackInfo &pnew);
  virtual void        remove_child      (WidgetImpl           &widget) = 0;
  virtual void        unparent_child    (WidgetImpl           &widget);
  virtual void        dispose_widget    (WidgetImpl           &widget);
  virtual void        hierarchy_changed (WidgetImpl           *old_toplevel);
  virtual bool        move_focus        (FocusDirType    fdir);
  void                expose_enclosure  (); /* expose without children */
  void                change_unviewable (WidgetImpl &child, bool);
  virtual void        focus_lost        ()                              { set_focus_child (NULL); }
  virtual void        set_focus_child   (WidgetImpl *widget);
  virtual void        scroll_to_child   (WidgetImpl &widget);
  virtual void        dump_test_data    (TestStream &tstream);
  static Allocation   layout_child      (WidgetImpl &child, const Allocation &carea);
public:
  virtual WidgetImplP*  begin           () const = 0;
  virtual WidgetImplP*  end             () const = 0;
  WidgetImpl*           get_focus_child () const;
  void                  child_container (ContainerImpl  *child_container);
  ContainerImpl&        child_container ();
  virtual void          foreach_recursive  (const std::function<void (WidgetImpl&)> &f) override;
  virtual size_t        n_children      () = 0;
  virtual WidgetImpl*   nth_child       (size_t nth) = 0;
  bool                  has_children    ()                              { return 0 != n_children(); }
  bool                  remove          (WidgetImpl           &widget);
  bool                  remove          (WidgetImpl           *widget)  { assert_return (widget != NULL, 0); return remove (*widget); }
  void                  add             (WidgetImpl                   &widget);
  void                  add             (WidgetImpl                   *widget);
  virtual Affine        child_affine    (const WidgetImpl             &widget); /* container => widget affine */
  virtual void          point_children  (Point                   p, /* widget coordinates relative */
                                         std::vector<WidgetImplP>     &stack);
  void    screen_window_point_children  (Point                   p, /* screen_window coordinates relative */
                                         std::vector<WidgetImplP>     &stack);
  virtual ContainerImpl* as_container_impl ()                           { return this; }
  virtual void          render_recursive(RenderContext &rcontext);
  void                  debug_tree      (String indent = String());
  // ContainerIface
  virtual WidgetIfaceP create_widget    (const String &widget_identifier, const StringSeq &args) override;
  virtual void         remove_widget    (WidgetIface &child) override;
};

// == Single Child Container ==
class SingleContainerImpl : public virtual ContainerImpl {
  WidgetImplP           child_widget;
protected:
  void                  size_request_child      (Requisition &requisition, bool *hspread, bool *vspread);
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area, bool changed);
  virtual void          render                  (RenderContext&, const Rect&) {}
  WidgetImpl&           get_child               () { critical_unless (child_widget != NULL); return *child_widget; }
  virtual              ~SingleContainerImpl     ();
  virtual WidgetImplP*  begin                   () const override;
  virtual WidgetImplP*  end                     () const override;
  virtual size_t        n_children              () { return child_widget ? 1 : 0; }
  virtual WidgetImpl*   nth_child               (size_t nth) { return nth == 0 ? child_widget.get() : NULL; }
  bool                  has_visible_child       () { return child_widget && child_widget->visible(); }
  bool                  has_drawable_child      () { return child_widget && child_widget->drawable(); }
  virtual void          add_child               (WidgetImpl   &widget);
  virtual void          remove_child            (WidgetImpl   &widget);
  explicit              SingleContainerImpl     ();
};

// == AnchorInfo ==
struct AnchorInfo {
  ResizeContainerImpl *resize_container;
  ViewportImpl        *viewport;
  WindowImpl          *window;
  constexpr AnchorInfo() : resize_container (NULL), viewport (NULL), window (NULL) {}
};

// == Resize Container ==
class ResizeContainerImpl : public virtual SingleContainerImpl {
  uint                  tunable_requisition_counter_;
  uint                  resizer_;
  AnchorInfo            anchor_info_;
  void                  idle_sizing             ();
  void                  update_anchor_info      ();
protected:
  virtual void          invalidate_parent       ();
  virtual void          hierarchy_changed       (WidgetImpl *old_toplevel);
  void                  negotiate_size          (const Allocation *carea);
  explicit              ResizeContainerImpl     ();
  virtual              ~ResizeContainerImpl     ();
public:
  bool                  requisitions_tunable    () const { return tunable_requisition_counter_ > 0; }
  AnchorInfo*           container_anchor_info   () { return &anchor_info_; }
};

// == Multi Child Container ==
class MultiContainerImpl : public virtual ContainerImpl {
  std::vector<WidgetImplP> widgets;
protected:
  virtual              ~MultiContainerImpl      ();
  virtual void          render                  (RenderContext&, const Rect&) {}
  virtual WidgetImplP*  begin                   () const override;
  virtual WidgetImplP*  end                     () const override;
  virtual size_t        n_children              () { return widgets.size(); }
  virtual WidgetImpl*   nth_child               (size_t nth) { return nth < widgets.size() ? widgets[nth].get() : NULL; }
  virtual void          add_child               (WidgetImpl   &widget);
  virtual void          remove_child            (WidgetImpl   &widget);
  void                  raise_child             (WidgetImpl   &widget);
  void                  lower_child             (WidgetImpl   &widget);
  void                  remove_all_children     ();
  explicit              MultiContainerImpl      ();
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
