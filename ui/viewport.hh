// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_VIEWPORT_HH__
#define __RAPICORN_VIEWPORT_HH__

#include <ui/container.hh>
#include <ui/displaywindow.hh>

namespace Rapicorn {

class ViewportImpl : public virtual ResizeContainerImpl {
  Region                expose_region_;        // maintained in child coord space
  int                   xoffset_, yoffset_;
  void                  collapse_expose_region  ();
protected:
  virtual Affine        child_affine            (const WidgetImpl &widget);
  const Region&         peek_expose_region      () const { return expose_region_; }
  void                  discard_expose_region   () { expose_region_.clear(); }
  bool                  exposes_pending         () const { return !expose_region_.empty(); }
  virtual void          render_recursive        (RenderContext &rcontext);
  virtual void          render                  (RenderContext &rcontext, const Rect &rect);
  void                  scroll_offsets          (int deltax, int deltay);
  void                  do_scrolled             ();
  int                   scroll_offset_x         () const { return xoffset_; }
  int                   scroll_offset_y         () const { return yoffset_; }
public:
  Aida::Signal<void ()> sig_scrolled;
  void                  expose_child_region     (const Region &region);
  Allocation            child_viewport          ();
  explicit              ViewportImpl            ();
  virtual              ~ViewportImpl            ();
};

} // Rapicorn

#endif  /* __RAPICORN_VIEWPORT_HH__ */
