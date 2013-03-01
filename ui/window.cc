// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "window.hh"
#include "application.hh"
#include "factory.hh"
#include "uithread.hh"
#include <string.h> // memcpy
#include <algorithm>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("Events", __VA_ARGS__)

namespace Rapicorn {

struct ClassDoctor {
  static void widget_set_flag       (WidgetImpl &widget, uint32 flag) { widget.set_flag (flag, true); }
  static void widget_unset_flag     (WidgetImpl &widget, uint32 flag) { widget.unset_flag (flag); }
  static void set_window_heritage (WindowImpl &window, Heritage *heritage) { window.heritage (heritage); }
  static Heritage*
  window_heritage (WindowImpl &window, ColorSchemeType cst)
  {
    return Heritage::create_heritage (window, window, cst);
  }
};

WindowImpl&
WindowIface::impl ()
{
  WindowImpl *wimpl = dynamic_cast<WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

const WindowImpl&
WindowIface::impl () const
{
  const WindowImpl *wimpl = dynamic_cast<const WindowImpl*> (this);
  if (!wimpl)
    throw std::bad_cast();
  return *wimpl;
}

const PropertyList&
WindowImpl::_property_list ()
{
  static Property *properties[] = {
    MakeProperty (WindowImpl, title,   _("Title"),   _("User visible title to be displayed in the window title bar"), "rw"),
  };
  static const PropertyList property_list (properties, ViewportImpl::_property_list());
  return property_list;
}

String
WindowImpl::title () const
{
  return config_.title;
}

void
WindowImpl::title (const String &window_title)
{
  if (config_.title != window_title)
    {
      config_.title = window_title;
      if (screen_window_)
        screen_window_->configure (config_, false);
    }
}

void
WindowImpl::set_parent (ContainerImpl *parent)
{
  if (parent)
    critical ("setting parent on toplevel Window widget to: %p (%s)", parent, parent->typeid_name().c_str());
  return ContainerImpl::set_parent (parent);
}

bool
WindowImpl::custom_command (const String &command_name, const StringSeq &command_args)
{
  assert_return (commands_emission_ == NULL, false);
  last_command_ = command_name;
  commands_emission_ = sig_commands.emission (command_name, command_args);
  return true;
}

bool
WindowImpl::command_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return commands_emission_ && commands_emission_->pending();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      commands_emission_->dispatch();                   // invoke signal handlers
      bool handled = false;
      if (commands_emission_->has_value())
        handled = commands_emission_->get_value();      // value returned from signal handler
      if (handled || commands_emission_->done())
        {
          if (!handled)                                 // all handlers returned false
            critical ("Command unhandled: %s", last_command_.c_str());
          Signal_commands::Emission *emi = commands_emission_;
          commands_emission_ = NULL;
          delete emi;
          last_command_ = "";
        }
      unref (this);
      return true;
    }
  else if (state.phase == state.DESTROY)
    {
      if (commands_emission_)
        {
          Signal_commands::Emission *emi = commands_emission_;
          commands_emission_ = NULL;
          delete emi;
          last_command_ = "";
        }
    }
  return false;
}

struct CurrentFocus {
  WidgetImpl *focus_widget;
  size_t    uncross_id;
  CurrentFocus (WidgetImpl *f = NULL, size_t i = 0) : focus_widget (f), uncross_id (i) {}
};
static DataKey<CurrentFocus> focus_widget_key;

WidgetImpl*
WindowImpl::get_focus () const
{
  return get_data (&focus_widget_key).focus_widget;
}

void
WindowImpl::uncross_focus (WidgetImpl &fwidget)
{
  CurrentFocus cfocus = get_data (&focus_widget_key);
  assert_return (&fwidget == cfocus.focus_widget);
  if (cfocus.uncross_id)
    {
      set_data (&focus_widget_key, CurrentFocus (cfocus.focus_widget, 0)); // reset cfocus.uncross_id
      cross_unlink (fwidget, cfocus.uncross_id);
      WidgetImpl *widget = &fwidget;
      while (widget)
        {
          ClassDoctor::widget_unset_flag (*widget, FOCUS_CHAIN);
          ContainerImpl *fc = widget->parent();
          if (fc)
            fc->set_focus_child (NULL);
          widget = fc;
        }
      cfocus = get_data (&focus_widget_key);
      assert_return (&fwidget == cfocus.focus_widget && cfocus.uncross_id == 0);
      delete_data (&focus_widget_key);
    }
}

void
WindowImpl::set_focus (WidgetImpl *widget)
{
  CurrentFocus cfocus = get_data (&focus_widget_key);
  if (widget == cfocus.focus_widget)
    return;
  if (cfocus.focus_widget)
    uncross_focus (*cfocus.focus_widget);
  if (!widget)
    return;
  // set new focus
  assert_return (widget->has_ancestor (*this));
  cfocus.focus_widget = widget;
  cfocus.uncross_id = cross_link (*cfocus.focus_widget, Aida::slot (*this, &WindowImpl::uncross_focus));
  set_data (&focus_widget_key, cfocus);
  while (widget)
    {
      ClassDoctor::widget_set_flag (*widget, FOCUS_CHAIN);
      ContainerImpl *fc = widget->parent();
      if (fc)
        fc->set_focus_child (widget);
      widget = fc;
    }
}

cairo_surface_t*
WindowImpl::create_snapshot (const Rect &subarea)
{
  const Allocation area = allocation();
  Region region = area;
  region.intersect (subarea);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, subarea.width, subarea.height);
  critical_unless (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS);
  cairo_surface_set_device_offset (surface, -subarea.x, -subarea.y);
  cairo_t *cr = cairo_create (surface);
  critical_unless (CAIRO_STATUS_SUCCESS == cairo_status (cr));
  render_into (cr, region);
  cairo_destroy (cr);
  return surface;
}

namespace WindowTrail {
static Mutex               wmutex;
static vector<WindowImpl*> windows;
static vector<WindowImpl*> wlist  ()               { ScopedLock<Mutex> slock (wmutex); return windows; }
static void wenter (WindowImpl *wi) { ScopedLock<Mutex> slock (wmutex); windows.push_back (wi); }
static void wleave (WindowImpl *wi)
{
  ScopedLock<Mutex> slock (wmutex);
  auto it = find (windows.begin(), windows.end(), wi);
  assert_return (it != windows.end());
  windows.erase (it);
};
} // WindowTrail

void
WindowImpl::forcefully_close_all ()
{
  vector<WindowImpl*> wl = WindowTrail::wlist();
  for (auto it : wl)
    it->close();
}

WindowImpl::WindowImpl() :
  loop_ (*ref_sink (uithread_main_loop()->new_slave())),
  screen_window_ (NULL), commands_emission_ (NULL),
  notify_displayed_id_ (0), entered_ (false), auto_close_ (false), pending_win_size_ (false), pending_expose_ (true)
{
  const_cast<AnchorInfo*> (force_anchor_info())->window = this;
  WindowTrail::wenter (this);
  Heritage *hr = ClassDoctor::window_heritage (*this, color_scheme());
  ref_sink (hr);
  ClassDoctor::set_window_heritage (*this, hr);
  unref (hr);
  set_flag (PARENT_SENSITIVE, true);
  set_flag (PARENT_UNVIEWABLE, false);
  /* create event loop (auto-starts) */
  loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::event_dispatcher), EventLoop::PRIORITY_NORMAL);
  loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::resizing_dispatcher), PRIORITY_RESIZE);
  loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::drawing_dispatcher), EventLoop::PRIORITY_UPDATE);
  loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
  loop_.flag_primary (false);
  ApplicationImpl::the().add_window (*this);
  change_flags_silently (ANCHORED, true);       /* window is always anchored */
}

void
WindowImpl::dispose ()
{
  ApplicationImpl::the().remove_window (*this);
}

WindowImpl::~WindowImpl()
{
  WindowTrail::wleave (this);
  if (notify_displayed_id_)
    {
      loop_.try_remove (notify_displayed_id_);
      notify_displayed_id_ = 0;
    }
  if (screen_window_)
    {
      screen_window_->destroy();
      screen_window_ = NULL;
    }
  /* make sure all children are removed while this is still of type WindowImpl.
   * necessary because C++ alters the object type during constructors and destructors
   */
  if (has_children())
    remove (get_child());
  /* shutdown event loop */
  loop_.kill_sources();
  /* this should be done last */
  unref (&loop_);
  const_cast<AnchorInfo*> (force_anchor_info())->window = NULL;
}

void
WindowImpl::resize_window (const Allocation *new_area)
{
  const uint64 start = timestamp_realtime();
  assert_return (requisitions_tunable() == false);
  Requisition rsize;
  ScreenWindow::State state;
  bool allocated = false;

  // negotiate sizes (new_area==NULL) and ensures window is allocated
  negotiate_size (new_area);
  pending_expose_ = true;
  if (new_area)
    goto done;  // only called for reallocating
  rsize = requisition();

  // grow screen window if needed
  if (!screen_window_)
    goto done;
  state = screen_window_->get_state();
  if (state.width <= 0 || state.height <= 0 || rsize.width > state.width || rsize.height > state.height)
    {
      config_.request_width = rsize.width;
      config_.request_height = rsize.height;
      pending_win_size_ = true;
      discard_expose_region(); // we request a new WIN_SIZE event in configure
      screen_window_->configure (config_, true);
      goto done;
    }
  // screen window size is good, allocate it
  if (rsize.width != state.width || rsize.height != state.height)
    {
      Allocation area = Allocation (0, 0, state.width, state.height);
      negotiate_size (&area);
      allocated = true;
    }
 done:
  const uint64 stop = timestamp_realtime();
  Allocation area = new_area ? *new_area : allocated ? Allocation (0, 0, state.width, state.height) : Allocation (0, 0, rsize.width, rsize.height);
  EDEBUG ("RESIZE: request=%s allocate=%s elapsed=%.3fms",
          new_area ? "-" : string_printf ("%.0fx%.0f", rsize.width, rsize.height).c_str(),
          string_printf ("%.0fx%.0f", area.width, area.height).c_str(),
          (stop - start) / 1000.0);
}

void
WindowImpl::do_invalidate ()
{
  ViewportImpl::do_invalidate();
  // we just need to make sure to be woken up, since flags are set appropriately already
  loop_.wakeup();
}

void
WindowImpl::beep()
{
  if (screen_window_)
    screen_window_->beep();
}

vector<WidgetImpl*>
WindowImpl::widget_difference (const vector<WidgetImpl*> &clist, /* preserves order of clist */
                             const vector<WidgetImpl*> &cminus)
{
  map<WidgetImpl*,bool> mminus;
  for (uint i = 0; i < cminus.size(); i++)
    mminus[cminus[i]] = true;
  vector<WidgetImpl*> result;
  for (uint i = 0; i < clist.size(); i++)
    if (!mminus[clist[i]])
      result.push_back (clist[i]);
  return result;
}

bool
WindowImpl::dispatch_mouse_movement (const Event &event)
{
  last_event_context_ = event;
  vector<WidgetImpl*> pierced;
  /* figure all entered children */
  bool unconfined;
  WidgetImpl *grab_widget = get_grab (&unconfined);
  if (grab_widget)
    {
      if (unconfined or grab_widget->screen_window_point (Point (event.x, event.y)))
        {
          pierced.push_back (ref (grab_widget));        /* grab-widget receives all mouse events */
          ContainerImpl *container = grab_widget->interface<ContainerImpl*>();
          if (container)                              /* deliver to hovered grab-widget children as well */
            container->screen_window_point_children (Point (event.x, event.y), pierced);
        }
    }
  else if (drawable())
    {
      pierced.push_back (ref (this)); /* window receives all mouse events */
      if (entered_)
        screen_window_point_children (Point (event.x, event.y), pierced);
    }
  /* send leave events */
  vector<WidgetImpl*> left_children = widget_difference (last_entered_children_, pierced);
  EventMouse *leave_event = create_event_mouse (MOUSE_LEAVE, EventContext (event));
  for (vector<WidgetImpl*>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
    (*it)->process_event (*leave_event);
  delete leave_event;
  /* send enter events */
  vector<WidgetImpl*> entered_children = widget_difference (pierced, last_entered_children_);
  EventMouse *enter_event = create_event_mouse (MOUSE_ENTER, EventContext (event));
  for (vector<WidgetImpl*>::reverse_iterator it = entered_children.rbegin(); it != entered_children.rend(); it++)
    (*it)->process_event (*enter_event);
  delete enter_event;
  /* send actual move event */
  bool handled = false;
  EventMouse *move_event = create_event_mouse (MOUSE_MOVE, EventContext (event));
  for (vector<WidgetImpl*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled && (*it)->sensitive())
      handled = (*it)->process_event (*move_event);
  delete move_event;
  /* cleanup */
  for (vector<WidgetImpl*>::reverse_iterator it = last_entered_children_.rbegin(); it != last_entered_children_.rend(); it++)
    (*it)->unref();
  last_entered_children_ = pierced;
  return handled;
}

bool
WindowImpl::dispatch_event_to_pierced_or_grab (const Event &event)
{
  vector<WidgetImpl*> pierced;
  /* figure all entered children */
  WidgetImpl *grab_widget = get_grab();
  if (grab_widget)
    pierced.push_back (ref (grab_widget));
  else if (drawable())
    {
      pierced.push_back (ref (this)); /* window receives all events */
      screen_window_point_children (Point (event.x, event.y), pierced);
    }
  /* send actual event */
  bool handled = false;
  for (vector<WidgetImpl*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    {
      if (!handled && (*it)->sensitive())
        handled = (*it)->process_event (event);
      (*it)->unref();
    }
  return handled;
}

bool
WindowImpl::dispatch_button_press (const EventButton &bevent)
{
  uint press_count = bevent.type - BUTTON_PRESS + 1;
  assert (press_count >= 1 && press_count <= 3);
  /* figure all entered children */
  const vector<WidgetImpl*> &pierced = last_entered_children_;
  /* send actual event */
  bool handled = false;
  for (vector<WidgetImpl*>::const_reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
    if (!handled && (*it)->sensitive())
      {
        ButtonState bs (*it, bevent.button);
        if (button_state_map_[bs] == 0)                /* no press delivered for <button> on <widget> yet */
          {
            button_state_map_[bs] = press_count;       /* record single press */
            handled = (*it)->process_event (bevent);    // modifies last_entered_children_ + this
          }
      }
  return handled;
}

bool
WindowImpl::dispatch_button_release (const EventButton &bevent)
{
  bool handled = false;
 restart:
  for (map<ButtonState,uint>::iterator it = button_state_map_.begin();
       it != button_state_map_.end(); it++)
    {
      const ButtonState bs = it->first;
      // uint press_count = it->second;
      if (bs.button == bevent.button)
        {
#if 0 // FIXME
          if (press_count == 3)
            bevent.type = BUTTON_3RELEASE;
          else if (press_count == 2)
            bevent.type = BUTTON_2RELEASE;
#endif
          button_state_map_.erase (it);
          handled |= bs.widget->process_event (bevent); // modifies button_state_map_ + this
          goto restart; // restart bs.button search
        }
    }
  // bevent.type = BUTTON_RELEASE;
  return handled;
}

void
WindowImpl::cancel_widget_events (WidgetImpl *widget)
{
  /* cancel enter events */
  for (int i = last_entered_children_.size(); i > 0;)
    {
      WidgetImpl *current = last_entered_children_[--i]; /* walk backwards */
      if (widget == current || !widget)
        {
          EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context_);
          current->process_event (*mevent);
          delete mevent;
          current->unref();
          last_entered_children_.erase (last_entered_children_.begin() + i);
        }
    }
  /* cancel button press events */
  while (button_state_map_.begin() != button_state_map_.end())
    {
      map<ButtonState,uint>::iterator it = button_state_map_.begin();
      const ButtonState bs = it->first;
      button_state_map_.erase (it);
      if (bs.widget == widget || !widget)
        {
          EventButton *bevent = create_event_button (BUTTON_CANCELED, last_event_context_, bs.button);
          bs.widget->process_event (*bevent); // modifies button_state_map_ + this
          delete bevent;
        }
    }
}

bool
WindowImpl::dispatch_cancel_event (const Event &event)
{
  cancel_widget_events (NULL);
  return false;
}

bool
WindowImpl::dispatch_enter_event (const EventMouse &mevent)
{
  entered_ = true;
  dispatch_mouse_movement (mevent);
  return false;
}

bool
WindowImpl::dispatch_move_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  return false;
}

bool
WindowImpl::dispatch_leave_event (const EventMouse &mevent)
{
  dispatch_mouse_movement (mevent);
  entered_ = false;
  if (get_grab ())
    ; /* leave events in grab mode are automatically calculated */
  else
    {
      /* send leave events */
      while (last_entered_children_.size())
        {
          WidgetImpl *widget = last_entered_children_.back();
          last_entered_children_.pop_back();
          widget->process_event (mevent);
          widget->unref();
        }
    }
  return false;
}

bool
WindowImpl::dispatch_button_event (const Event &event)
{
  bool handled = false;
  const EventButton *bevent = dynamic_cast<const EventButton*> (&event);
  if (bevent)
    {
      dispatch_mouse_movement (*bevent);
      if (bevent->type >= BUTTON_PRESS && bevent->type <= BUTTON_3PRESS)
        handled = dispatch_button_press (*bevent);
      else
        handled = dispatch_button_release (*bevent);
      dispatch_mouse_movement (*bevent);
    }
  return handled;
}

bool
WindowImpl::dispatch_focus_event (const EventFocus &fevent)
{
  bool handled = false;
  // dispatch_event_to_pierced_or_grab (*fevent);
  dispatch_mouse_movement (fevent);
  return handled;
}

bool
WindowImpl::move_focus_dir (FocusDirType focus_dir)
{
  WidgetImpl *new_focus = NULL, *old_focus = get_focus();
  if (old_focus)
    ref (old_focus);

  switch (focus_dir)
    {
    case FOCUS_UP:   case FOCUS_DOWN:
    case FOCUS_LEFT: case FOCUS_RIGHT:
      new_focus = old_focus;
      break;
    default: ;
    }
  if (focus_dir && !move_focus (focus_dir))
    {
      if (new_focus && new_focus->get_window() != this)
        new_focus = NULL;
      if (new_focus)
        new_focus->grab_focus();
      else
        set_focus (NULL);
      if (old_focus == new_focus)
        return false; // should have moved focus but failed
    }
  if (old_focus)
    unref (old_focus);
  return true;
}

bool
WindowImpl::dispatch_key_event (const Event &event)
{
  bool handled = false;
  dispatch_mouse_movement (event);
  WidgetImpl *widget = get_focus();
  if (widget && widget->process_screen_window_event (event))
    return true;
  const EventKey *kevent = dynamic_cast<const EventKey*> (&event);
  if (kevent && kevent->type == KEY_PRESS)
    {
      FocusDirType fdir = key_value_to_focus_dir (kevent->key);
      ActivateKeyType activate = key_value_to_activation (kevent->key);
      if (!handled && fdir)
        {
          if (!move_focus_dir (fdir))
            notify_key_error();
          handled = true;
        }
      if (!handled && (activate == ACTIVATE_FOCUS || activate == ACTIVATE_DEFAULT))
        {
          WidgetImpl *focus_widget = get_focus();
          if (focus_widget && focus_widget->sensitive())
            {
              if (!focus_widget->activate())
                notify_key_error();
              handled = true;
            }
        }
      if (0)
        {
          WidgetImpl *grab_widget = get_grab();
          grab_widget = grab_widget ? grab_widget : this;
          handled = grab_widget->process_event (*kevent);
        }
    }
  return handled;
}

bool
WindowImpl::dispatch_scroll_event (const EventScroll &sevent)
{
  bool handled = false;
  if (sevent.type == SCROLL_UP || sevent.type == SCROLL_RIGHT ||
      sevent.type == SCROLL_DOWN || sevent.type == SCROLL_LEFT)
    {
      dispatch_mouse_movement (sevent);
      handled = dispatch_event_to_pierced_or_grab (sevent);
    }
  return handled;
}
bool
WindowImpl::dispatch_win_size_event (const Event &event)
{
  bool handled = false;
  const EventWinSize *wevent = dynamic_cast<const EventWinSize*> (&event);
  if (wevent)
    {
      pending_win_size_ = wevent->intermediate || has_queued_win_size();
      EDEBUG ("%s: %.0fx%.0f intermediate=%d pending=%d", string_from_event_type (event.type),
              wevent->width, wevent->height, wevent->intermediate, pending_win_size_);
      const Allocation area = allocation();
      if (wevent->width != area.width || wevent->height != area.height)
        {
          Allocation new_area = Allocation (0, 0, wevent->width, wevent->height);
          if (!pending_win_size_)
            {
#if 0 // excessive resizing costs
              Allocation zzz = new_area;
              resize_window (&zzz);
              for (int i = 0; i < 37; i++)
                {
                  zzz.width += 1;
                  resize_window (&zzz);
                  zzz.height += 1;
                  resize_window (&zzz);
                }
#endif
              resize_window (&new_area);
            }
          else
            discard_expose_region(); // we'll get more WIN_SIZE events
        }
      if (!pending_win_size_ && pending_expose_)
        {
          expose();
          pending_expose_ = false;
        }
      handled = true;
    }
  return handled;
}

bool
WindowImpl::dispatch_win_delete_event (const Event &event)
{
  bool handled = false;
  const EventWinDelete *devent = dynamic_cast<const EventWinDelete*> (&event);
  if (devent)
    handled = dispatch_win_destroy();
  return handled;
}

bool
WindowImpl::dispatch_win_destroy ()
{
  destroy_screen_window();
  dispose();
  return true;
}

void
WindowImpl::notify_displayed()
{
  // emit updates at exec_update() priority, so other high priority handlers run first (exec_now)
  sig_displayed.emit();
}

void
WindowImpl::draw_child (WidgetImpl &child)
{
  // FIXME: this should be optimized to just redraw the child in question
  WindowImpl *child_window = child.get_window();
  assert_return (child_window == this);
  draw_now();
}

void
WindowImpl::draw_now ()
{
  if (screen_window_)
    {
      const uint64 start = timestamp_realtime();
      Rect area = allocation();
      assert_return (area.x == 0 && area.y == 0);
      // determine invalidated rendering region
      Region region = area;
      region.intersect (peek_expose_region());
      discard_expose_region();
      // rendering rectangle
      Rect rrect = region.extents();
      const int x1 = ifloor (rrect.x), y1 = ifloor (rrect.y), x2 = iceil (rrect.x + rrect.width), y2 = iceil (rrect.y + rrect.height);
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, x2 - x1, y2 - y1);
      cairo_surface_set_device_offset (surface, -x1, -y1);
      critical_unless (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS);
      cairo_t *cr = cairo_create (surface);
      critical_unless (CAIRO_STATUS_SUCCESS == cairo_status (cr));
      render_into (cr, region);
      screen_window_->blit_surface (surface, region);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);
      if (!notify_displayed_id_)
        notify_displayed_id_ = loop_.exec_update (Aida::slot (*this, &WindowImpl::notify_displayed));
      const uint64 stop = timestamp_realtime();
      EDEBUG ("RENDER: %+d%+d%+dx%d coverage=%.1f%% elapsed=%.3fms",
              x1, y1, x2 - x1, y2 - y1, ((x2 - x1) * (y2 - y1)) * 100.0 / (area.width*area.height),
              (stop - start) / 1000.0);
    }
  else
    discard_expose_region(); // nuke stale exposes
}

void
WindowImpl::render (RenderContext &rcontext, const Rect &rect)
{
  // paint background
  Color col = background();
  cairo_t *cr = cairo_context (rcontext, rect);
  cairo_set_source_rgba (cr, col.red1(), col.green1(), col.blue1(), col.alpha1());
  vector<Rect> rects;
  rendering_region (rcontext).list_rects (rects);
  for (size_t i = 0; i < rects.size(); i++)
    cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
  cairo_clip (cr);
  cairo_paint (cr);
  ViewportImpl::render (rcontext, rect);
}

void
WindowImpl::remove_grab_widget (WidgetImpl &child)
{
  bool stack_changed = false;
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget == &child)
      {
        grab_stack_.erase (grab_stack_.begin() + i);
        stack_changed = true;
      }
  if (stack_changed)
    grab_stack_changed();
}

void
WindowImpl::grab_stack_changed()
{
  // FIXME: use idle handler for event synthesis
  EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context_);
  dispatch_mouse_movement (*mevent);
  /* synthesize neccessary leaves after grabbing */
  if (!grab_stack_.size() && !entered_)
    dispatch_event (*mevent);
  delete mevent;
}

void
WindowImpl::add_grab (WidgetImpl *child,
                      bool      unconfined)
{
  assert_return (child != NULL);
  add_grab (*child, unconfined);
}

void
WindowImpl::add_grab (WidgetImpl &child,
                      bool      unconfined)
{
  if (!child.has_ancestor (*this))
    throw Exception ("child is not descendant of container \"", name(), "\": ", child.name());
  /* for unconfined==true grabs, the mouse pointer is always considered to
   * be contained by the grab-widget, and only by the grab-widget. events are
   * delivered to the grab-widget and its children.
   */
  grab_stack_.push_back (GrabEntry (&child, unconfined));
  // grab_stack_changed(); // FIXME: re-enable this, once grab_stack_changed() synthesizes from idler
}

void
WindowImpl::remove_grab (WidgetImpl *child)
{
  assert_return (child != NULL);
  remove_grab (*child);
}

void
WindowImpl::remove_grab (WidgetImpl &child)
{
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget == &child)
      {
        grab_stack_.erase (grab_stack_.begin() + i);
        grab_stack_changed();
        return;
      }
  throw Exception ("no such child in grab stack: ", child.name());
}

WidgetImpl*
WindowImpl::get_grab (bool *unconfined)
{
  for (int i = grab_stack_.size() - 1; i >= 0; i--)
    if (grab_stack_[i].widget->visible())
      {
        if (unconfined)
          *unconfined = grab_stack_[i].unconfined;
        return grab_stack_[i].widget;
      }
  return NULL;
}

void
WindowImpl::dispose_widget (WidgetImpl &widget)
{
  remove_grab_widget (widget);
  cancel_widget_events (widget);
  ViewportImpl::dispose_widget (widget);
}

bool
WindowImpl::has_queued_win_size ()
{
  return screen_window_ && screen_window_->peek_events ([] (Event *e) { return e->type == WIN_SIZE; });
}

bool
WindowImpl::dispatch_event (const Event &event)
{
  if (!screen_window_)
    return false;       // we can only handle events on a screen_window
  EDEBUG ("%s: w=%p", string_from_event_type (event.type), this);
  switch (event.type)
    {
    case EVENT_LAST:
    case EVENT_NONE:          return false;
    case MOUSE_ENTER:         return dispatch_enter_event (event);
    case MOUSE_MOVE:
      if (screen_window_->peek_events ([] (Event *e) { return e->type == MOUSE_MOVE; }))
        return true; // coalesce multiple motion events
      else
        return dispatch_move_event (event);
    case MOUSE_LEAVE:         return dispatch_leave_event (event);
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
    case BUTTON_CANCELED:
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:     return dispatch_button_event (event);
    case FOCUS_IN:
    case FOCUS_OUT:           return dispatch_focus_event (event);
    case KEY_PRESS:
    case KEY_CANCELED:
    case KEY_RELEASE:         return dispatch_key_event (event);
    case SCROLL_UP:          // button4
    case SCROLL_DOWN:        // button5
    case SCROLL_LEFT:        // button6
    case SCROLL_RIGHT:       // button7
      ;                       return dispatch_scroll_event (event);
    case CANCEL_EVENTS:       return dispatch_cancel_event (event);
    case WIN_SIZE:            return dispatch_win_size_event (event);
    case WIN_DELETE:          return dispatch_win_delete_event (event);
    case WIN_DESTROY:         return dispatch_win_destroy();
    }
  return false;
}

bool
WindowImpl::event_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return screen_window_ && screen_window_->has_event();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      Event *event = screen_window_ ? screen_window_->pop_event() : NULL;
      if (event)
        {
          dispatch_event (*event);
          delete event;
        }
      unref (this);
      return true;
    }
  else if (state.phase == state.DESTROY)
    destroy_screen_window();
  return false;
}

bool
WindowImpl::resizing_dispatcher (const EventLoop::State &state)
{
  const bool can_resize = !pending_win_size_ && screen_window_;
  const bool need_resize = can_resize && test_any_flag (INVALID_REQUISITION | INVALID_ALLOCATION);
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return need_resize;
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      if (need_resize)
        resize_window();
      unref (this);
      return true;
    }
  return false;
}

void
WindowImpl::enable_auto_close ()
{
  auto_close_ = true;
}

bool
WindowImpl::drawing_dispatcher (const EventLoop::State &state)
{
  if (state.phase == state.PREPARE || state.phase == state.CHECK)
    return exposes_pending();
  else if (state.phase == state.DISPATCH)
    {
      ref (this);
      if (exposes_pending())
        {
          draw_now();
          if (auto_close_)
            {
              EventLoop *loop = get_loop();
              if (loop)
                {
                  loop->exec_timer (0, Aida::slot (*this, &WindowImpl::destroy_screen_window), INT_MAX);
                  auto_close_ = false;
                }
            }
        }
      unref (this);
      return true;
    }
  return false;
}

EventLoop*
WindowImpl::get_loop ()
{
  return &loop_;
}

bool
WindowImpl::viewable ()
{
  return visible() && screen_window_ && screen_window_->viewable();
}

void
WindowImpl::idle_show()
{
  if (screen_window_)
    {
      // size request & show up
      screen_window_->show();
    }
}

void
WindowImpl::create_screen_window ()
{
  if (!finalizing())
    {
      if (!screen_window_)
        {
          resize_window(); // ensure initial size requisition
          ScreenDriver *sdriver = ScreenDriver::retrieve_screen_driver ("auto");
          if (sdriver)
            {
              ScreenWindow::Setup setup;
              setup.window_type = WINDOW_TYPE_NORMAL;
              uint64 flags = ScreenWindow::ACCEPT_FOCUS | ScreenWindow::DELETABLE |
                             ScreenWindow::DECORATED | ScreenWindow::MINIMIZABLE | ScreenWindow::MAXIMIZABLE;
              setup.request_flags = ScreenWindow::Flags (flags);
              String prg = program_ident();
              if (prg.empty())
                prg = program_file();
              setup.session_role = "RAPICORN:" + prg;
              if (!name().empty())
                setup.session_role += ":" + name();
              setup.bg_average = background();
              const Requisition rsize = requisition();
              config_.request_width = rsize.width;
              config_.request_height = rsize.height;
              if (config_.title.empty())
                {
                  // user_warning (this->user_source(), "window title is unset");
                  config_.title = setup.session_role;
                }
              if (config_.alias.empty())
                config_.alias = program_alias();
              pending_win_size_ = true;
              screen_window_ = sdriver->create_screen_window (setup, config_);
              screen_window_->set_event_wakeup ([this] () { loop_.wakeup(); /* thread safe */ });
            }
          else
            fatal ("failed to find and open any screen driver");
        }
      RAPICORN_ASSERT (screen_window_ != NULL);
      loop_.flag_primary (true); // FIXME: depends on WM-managable
      EventLoop::VoidSlot sl = Aida::slot (*this, &WindowImpl::idle_show);
      loop_.exec_now (sl);
    }
}

bool
WindowImpl::has_screen_window ()
{
  return !!screen_window_;
}

void
WindowImpl::destroy_screen_window ()
{
  if (!screen_window_)
    return; // during destruction, ref_count == 0
  ref (this);
  screen_window_->destroy();
  screen_window_ = NULL;
  loop_.flag_primary (false);
  loop_.kill_sources();
  // reset widget state where needed
  cancel_widget_events (NULL);
  if (!finalizing())
    {
      loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::event_dispatcher), EventLoop::PRIORITY_NORMAL);
      loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::resizing_dispatcher), PRIORITY_RESIZE);
      loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::drawing_dispatcher), EventLoop::PRIORITY_UPDATE);
      loop_.exec_dispatcher (Aida::slot (*this, &WindowImpl::command_dispatcher), EventLoop::PRIORITY_NOW);
    }
  unref (this);
}

void
WindowImpl::show ()
{
  create_screen_window();
}

bool
WindowImpl::closed ()
{
  return !has_screen_window();
}

void
WindowImpl::close ()
{
  destroy_screen_window();
}

bool
WindowImpl::snapshot (const String &pngname)
{
  cairo_surface_t *isurface = this->create_snapshot (allocation());
  cairo_status_t wstatus = cairo_surface_write_to_png (isurface, pngname.c_str());
  cairo_surface_destroy (isurface);
  String err = CAIRO_STATUS_SUCCESS == wstatus ? "ok" : cairo_status_to_string (wstatus);
  DEBUG ("WindowImpl:snapshot:%s: failed to create \"%s\": %s", name().c_str(), pngname.c_str(), err.c_str());
  return CAIRO_STATUS_SUCCESS == wstatus;
}

bool
WindowImpl::synthesize_enter (double xalign,
                              double yalign)
{
  if (!has_screen_window())
    return false;
  const Allocation &area = allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = point_to_screen_window (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  screen_window_->push_event (create_event_mouse (MOUSE_ENTER, ec));
  return true;
}

bool
WindowImpl::synthesize_leave ()
{
  if (!has_screen_window())
    return false;
  EventContext ec;
  screen_window_->push_event (create_event_mouse (MOUSE_LEAVE, ec));
  return true;
}

bool
WindowImpl::synthesize_click (WidgetIface &widgeti,
                              int        button,
                              double     xalign,
                              double     yalign)
{
  WidgetImpl &widget = *dynamic_cast<WidgetImpl*> (&widgeti);
  if (!has_screen_window() || !&widget)
    return false;
  const Allocation &area = widget.allocation();
  Point p (area.x + xalign * (max (1, area.width) - 1),
           area.y + yalign * (max (1, area.height) - 1));
  p = widget.point_to_screen_window (p);
  EventContext ec;
  ec.x = p.x;
  ec.y = p.y;
  screen_window_->push_event (create_event_button (BUTTON_RELEASE, ec, button));
  screen_window_->push_event (create_event_button (BUTTON_PRESS, ec, button));
  return true;
}

bool
WindowImpl::synthesize_delete ()
{
  if (!has_screen_window())
    return false;
  EventContext ec;
  screen_window_->push_event (create_event_win_delete (ec));
  return true;
}

static const WidgetFactory<WindowImpl> window_factory ("Rapicorn::Factory::Window");

} // Rapicorn
