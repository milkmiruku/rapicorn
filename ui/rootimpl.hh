/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __RAPICORN_ROOT_IMPL_HH__
#define __RAPICORN_ROOT_IMPL_HH__

#include <ui/root.hh>
#include <ui/containerimpl.hh>
#include <ui/viewp0rt.hh>

namespace Rapicorn {

class RootImpl : public virtual Root,
                 public virtual SingleContainerImpl,
                 public virtual Wind0w,
                 public virtual Viewp0rt::EventReceiver
{
  EventLoop            &m_loop;
  EventLoop::Source    *m_source;
  Mutex                 m_async_mutex;
  std::list<Event*>     m_async_event_queue;
  Region                m_expose_region;
  Viewp0rt             *m_viewp0rt;
  uint                  m_tunable_requisition_counter : 24;
  uint                  m_entered : 1;
  uint                  m_auto_close : 1;
  EventContext          m_last_event_context;
  vector<Item*>         m_last_entered_children;
  Viewp0rt::Config      m_config;
public:
  explicit              RootImpl                                ();
protected:
  virtual void          dispose                                 ();
private:
  /*Des*/               ~RootImpl                               ();
  virtual void          dispose_item                            (Item                   &item);
  virtual bool          self_visible                            () const;
  /* misc */
  vector<Item*>         item_difference                         (const vector<Item*>    &clist, /* preserves order of clist */
                                                                 const vector<Item*>    &cminus);
  /* sizing */
  virtual void          size_request                            (Requisition            &requisition);
  using                 Item::size_request;
  virtual void          size_allocate                           (Allocation              area);
  virtual bool          tunable_requisitions                    ();
  void                  resize_all                              (Allocation             *new_area);
  virtual void          do_invalidate                           ();
  virtual void          beep                                    ();
  /* rendering */
  virtual void          render                                  (Display                &display);
  using                 Item::render;
  void                  collapse_expose_region                  ();
  virtual void          expose_root_region                      (const Region           &region);
  virtual void          copy_area                               (const Rect             &src,
                                                                 const Point            &dest);
  void                  expose_now                              ();
  virtual void          draw_now                                ();
  /* grab handling */
  virtual void          remove_grab_item                        (Item                   &child);
  void                  grab_stack_changed                      ();
  virtual void          add_grab                                (Item                   &child,
                                                                 bool                    unconfined);
  virtual void          remove_grab                             (Item                   &child);
  virtual Item*         get_grab                                (bool                   *unconfined = NULL);
  /* main loop */
  virtual bool          viewable                                ();
  virtual void          create_viewp0rt                         ();
  virtual bool          has_viewp0rt                            ();
  virtual void          destroy_viewp0rt                        ();
  void                  idle_show                               ();
  virtual Wind0w&       wind0w                                  ();
  virtual bool          custom_command                          (const String       &command_name,
                                                                 const StringList   &command_args);
  virtual bool          prepare                                 (uint64                  current_time_usecs,
                                                                 int64                  *timeout_usecs_p);
  virtual bool          check                                   (uint64                  current_time_usecs);
  virtual bool          dispatch                                ();
  virtual void          enable_auto_close                       ();
  virtual EventLoop*    get_loop                                ();
  /* Wind0w */
  virtual Root&         root                                    ();
  virtual void          show                                    ();
  virtual bool          closed                                  ();
  virtual void          close                                   ();
  virtual bool          synthesize_enter                        (double xalign = 0.5,
                                                                 double yalign = 0.5);
  virtual bool          synthesize_leave                        ();
  virtual bool          synthesize_click                        (Item  &item,
                                                                 int    button,
                                                                 double xalign = 0.5,
                                                                 double yalign = 0.5);
  virtual bool            synthesize_delete                     ();
  /* event handling */
  virtual void          enqueue_async                           (Event                  *event);
  virtual void          cancel_item_events                      (Item                   *item);
  using                 Root::cancel_item_events;
  bool                  dispatch_mouse_movement                 (const Event            &event);
  bool                  dispatch_event_to_pierced_or_grab       (const Event            &event);
  bool                  dispatch_button_press                   (const EventButton      &bevent);
  bool                  dispatch_button_release                 (const EventButton      &bevent);
  bool                  dispatch_cancel_event                   (const Event            &event);
  bool                  dispatch_enter_event                    (const EventMouse       &mevent);
  bool                  dispatch_move_event                     (const EventMouse       &mevent);
  bool                  dispatch_leave_event                    (const EventMouse       &mevent);
  bool                  dispatch_button_event                   (const Event            &event);
  bool                  dispatch_focus_event                    (const EventFocus       &fevent);
  bool                  move_focus_dir                          (FocusDirType            focus_dir);
  bool                  dispatch_key_event                      (const Event            &event);
  bool                  dispatch_scroll_event                   (const EventScroll      &sevent);
  bool                  dispatch_win_size_event                 (const Event            &event);
  bool                  dispatch_win_draw_event                 (const Event            &event);
  bool                  dispatch_win_delete_event               (const Event            &event);
  virtual bool          dispatch_event                          (const Event            &event);
  bool                  has_pending_win_size                    ();
  /* --- GrabEntry --- */
  struct GrabEntry {
    Item *item;
    bool  unconfined;
    explicit            GrabEntry (Item *i, bool uc) : item (i), unconfined (uc) {}
  };
  vector<GrabEntry>     m_grab_stack;
  /* --- ButtonState --- */
  struct ButtonState {
    Item               *item;
    uint                button;
    explicit            ButtonState     (Item *i, uint b) : item (i), button (b) {}
    explicit            ButtonState     () : item (NULL), button (0) {}
    bool                operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.item < bs2.item || (bs1.item == bs2.item &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> m_button_state_map;
  /* --- EventLoop Source --- */
  class RootSource : public EventLoop::Source {
    RootImpl &root;
  public:
    explicit
    RootSource  (RootImpl &_root) :
      root (_root)
    {
      bool entered = rapicorn_thread_entered();
      if (!entered)
        rapicorn_thread_enter();
      assert (root.m_source == NULL);
      root.m_source = this;
      if (!entered)
        rapicorn_thread_leave();
    }
    virtual
    ~RootSource ()
    {
      bool entered = rapicorn_thread_entered();
      if (!entered)
        rapicorn_thread_enter();
      assert (root.m_source != this);
      if (!entered)
        rapicorn_thread_leave();
    }
    virtual bool prepare    (uint64 current_time_usecs,
                             int64 *timeout_usecs_p)         { assert (rapicorn_thread_entered()); return root.prepare (current_time_usecs, timeout_usecs_p); }
    virtual bool check      (uint64 current_time_usecs)      { assert (rapicorn_thread_entered()); return root.check (current_time_usecs); }
    virtual bool dispatch   ()                               { assert (rapicorn_thread_entered()); return root.dispatch(); }
    virtual void
    destroy ()
    {
      assert (rapicorn_thread_entered());
      assert (root.m_source == this);
      root.m_source = NULL;
      root.destroy_viewp0rt();
    }
  };
};


} // Rapicorn

#endif  /* __RAPICORN_ROOT_IMPL_HH__ */
