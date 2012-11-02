// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <ui/utilities.hh>
#include <cairo/cairo-xlib.h>
#include <sys/poll.h>
#include "screenwindow-xaux.cc"

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvents", __VA_ARGS__)

template<class T> cairo_status_t cairo_status_from_any (T t);
template<> cairo_status_t cairo_status_from_any (cairo_status_t c)          { return c; }
template<> cairo_status_t cairo_status_from_any (cairo_rectangle_list_t *c) { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_font_options_t *c)   { return c ? cairo_font_options_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_font_face_t *c)      { return c ? cairo_font_face_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_scaled_font_t *c)    { return c ? cairo_scaled_font_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_path_t *c)           { return c ? c->status : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_t *c)                { return c ? cairo_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_device_t *c)         { return c ? cairo_device_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_surface_t *c)        { return c ? cairo_surface_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (cairo_pattern_t *c)        { return c ? cairo_pattern_status (c) : CAIRO_STATUS_NULL_POINTER; }
template<> cairo_status_t cairo_status_from_any (const cairo_region_t *c)   { return c ? cairo_region_status (c) : CAIRO_STATUS_NULL_POINTER; }

#define RAPICORN_CHECK_CAIRO_STATUS(cairox)             do {    \
    cairo_status_t ___s = cairo_status_from_any (cairox);       \
    if (___s != CAIRO_STATUS_SUCCESS)                           \
      RAPICORN_CRITICAL ("cairo status (%s): %s", #cairox,      \
                         cairo_status_to_string (___s));        \
  } while (0)
#define CHECK_CAIRO_STATUS(cairox)      RAPICORN_CHECK_CAIRO_STATUS (cairox)

namespace Rapicorn {

// == declarations ==
class X11Thread;
class X11Context;
static X11Thread*       start_x11_thread (X11Context &x11context);
static void             stop_x11_thread  (X11Thread *x11_thread);
static Mutex x11_rmutex (RECURSIVE_LOCK);

// == X11Item ==
struct X11Item {
  explicit X11Item() {}
  virtual ~X11Item() {}
};

// == X11Context ==
struct X11Context {
  X11Thread            *x11_thread;
  Display              *display;
  int                   screen;
  Visual               *visual;
  int                   depth;
  Window                root_window;
  int8                  m_shared_mem;
  map<size_t, X11Item*> x11ids;
  explicit              X11Context (const String &x11display);
  Atom                  atom       (const String &text, bool force_create = true);
  String                atom       (Atom atom) const;
  bool                  local_x11  ();
  /*dtor*/             ~X11Context ();
};

X11Context::X11Context (const String &x11display) :
  x11_thread (NULL), display (NULL), screen (0), visual (NULL), depth (0), root_window (0), m_shared_mem (-1)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  do_once { xlib_error_handler = XSetErrorHandler (x11_error); }
  const char *s = x11display.c_str();
  display = XOpenDisplay (s[0] ? s : NULL);
  XDEBUG ("XOpenDisplay(%s): %s", CQUOTE (x11display.c_str()), display ? "success" : "failed to connect");
  if (!display)
    return;
  x11_thread = start_x11_thread (*this);
  load_atom_cache (display);
  if (1) // FIXME: toggle sync
    XSynchronize (display, true);
  screen = DefaultScreen (display);
  visual = DefaultVisual (display, screen);
  depth = DefaultDepth (display, screen);
  root_window = XRootWindow (display, screen);
}

X11Context::~X11Context ()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  if (x11_thread)
    {
      x11locker.unlock();
      stop_x11_thread (x11_thread);
      x11locker.lock();
      x11_thread = NULL;
    }
  x11ids.clear();
  if (display)
    {
      XCloseDisplay (display);
      display = NULL;
    }
}

Atom
X11Context::atom (const String &text, bool force_create)
{
  // XLib caches atoms well
  return XInternAtom (display, text.c_str(), !force_create);
}

String
X11Context::atom (Atom atom) const
{
  // XLib caches atoms well
  char *res = XGetAtomName (display, atom);
  String result (res ? res : "");
  if (res)
    XFree (res);
  return result;
}

bool
X11Context::local_x11()
{
  if (m_shared_mem < 0)
    m_shared_mem = x11_check_shared_image (display, visual, depth);
  XDEBUG ("XShmCreateImage: %s", m_shared_mem ? "ok" : "failed to attach");
  return m_shared_mem;
}

// == ScreenDriverX11 ==
struct ScreenDriverX11 : ScreenDriver {
  X11Context *m_x11context;
  uint        m_open_count;
  virtual bool
  open ()
  {
    if (!m_x11context)
      {
        assert_return (m_open_count == 0, false);
        const char *s = getenv ("DISPLAY");
        String ds = s ? s : "";
        X11Context *x11context = new X11Context (ds);
        if (!x11context->display)
          {
            delete x11context;
            return false;
          }
        m_x11context = x11context;
      }
    m_open_count++;
    return true;
  }
  virtual void
  close ()
  {
    assert_return (m_open_count > 0);
    m_open_count--;
    if (!m_open_count)
      {
        X11Context *x11context = m_x11context;
        m_x11context = NULL;
        delete x11context;
      }
  }
  virtual ScreenWindow* create_screen_window (const ScreenWindow::Setup &setup,
                                              const ScreenWindow::Config &config,
                                              ScreenWindow::EventReceiver &receiver);
  ScreenDriverX11() : ScreenDriver ("X11Window", -1), m_x11context (NULL), m_open_count (0) {}
};
static ScreenDriverX11 screen_driver_x11;

// == ScreenWindowX11 ==
struct ScreenWindowX11 : public virtual ScreenWindow, public virtual X11Item {
  X11Context           &x11context;
  ScreenDriverX11      &m_x11driver;
  EventReceiver        &m_receiver;
  State                 m_state;
  Color                 m_average_background;
  Window                m_window;
  int                   m_last_motion_time, m_pending_configures, m_pending_exposes;
  bool                  m_override_redirect, m_mapped, m_crossing_focus, m_win_focus;
  EventContext          m_event_context;
  Rapicorn::Region      m_expose_region;
  cairo_surface_t      *m_expose_surface;
  explicit              ScreenWindowX11         (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, const ScreenWindow::Config &config, EventReceiver &receiver);
  virtual              ~ScreenWindowX11         ();
  virtual State         get_state               ();
  virtual void          configure               (const Config &config);
  void                  setup                   (const ScreenWindow::Setup &setup);
  virtual void          beep                    ();
  virtual void          show                    ();
  virtual void          present                 ();
  virtual bool          viewable                ();
  virtual void          blit_surface            (cairo_surface_t *surface, Rapicorn::Region region);
  virtual void          start_user_move         (uint button, double root_x, double root_y);
  virtual void          start_user_resize       (uint button, double root_x, double root_y, AnchorType edge);
  void                  enqueue_locked          (Event *event);
  bool                  process_event           (const XEvent &xevent);
  void                  property_changed        (Atom atom, bool deleted);
  void                  client_message          (const XClientMessageEvent &xevent);
  void                  blit_expose_region      ();
};

ScreenWindow*
ScreenDriverX11::create_screen_window (const ScreenWindow::Setup &setup, const ScreenWindow::Config &config, ScreenWindow::EventReceiver &receiver)
{
  assert_return (m_open_count > 0, NULL);
  return new ScreenWindowX11 (*this, setup, config, receiver);
}

ScreenWindowX11::ScreenWindowX11 (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, const ScreenWindow::Config &config, EventReceiver &receiver) :
  x11context (*x11driver.m_x11context), m_x11driver (x11driver), m_receiver (receiver), m_average_background (0xff808080),
  m_window (0), m_last_motion_time (0), m_pending_configures (0), m_pending_exposes (0),
  m_override_redirect (false), m_mapped (false), m_crossing_focus (false), m_win_focus (false), m_expose_surface (NULL)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  m_x11driver.open();
  m_state.setup.window_type = setup.window_type;
  m_override_redirect = (setup.window_type == WINDOW_TYPE_DESKTOP ||
                         setup.window_type == WINDOW_TYPE_DROPDOWN_MENU ||
                         setup.window_type == WINDOW_TYPE_POPUP_MENU ||
                         setup.window_type == WINDOW_TYPE_TOOLTIP ||
                         setup.window_type == WINDOW_TYPE_NOTIFICATION ||
                         setup.window_type == WINDOW_TYPE_COMBO ||
                         setup.window_type == WINDOW_TYPE_DND);
  XSetWindowAttributes attributes;
  attributes.event_mask        = ExposureMask | StructureNotifyMask | VisibilityChangeMask | PropertyChangeMask |
                                 FocusChangeMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask | PointerMotionHintMask |
                                 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
                                 0 * OwnerGrabButtonMask; // owner_events for automatic grabs
  attributes.background_pixel  = XWhitePixel (x11context.display, x11context.screen);
  attributes.border_pixel      = XBlackPixel (x11context.display, x11context.screen);
  attributes.override_redirect = m_override_redirect;
  attributes.win_gravity       = StaticGravity;
  attributes.bit_gravity       = StaticGravity;
  attributes.save_under        = m_override_redirect;
  attributes.backing_store     = x11context.local_x11() ? NotUseful : WhenMapped;
  unsigned long attribute_mask = CWWinGravity | CWBitGravity | CWBackingStore | CWSaveUnder |
                                 CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask;
  const int border = 0, request_width = MAX (1, config.request_width), request_height = MAX (1, config.request_height);
  // create and register window
  const ulong create_serial = NextRequest (x11context.display);
  m_window = XCreateWindow (x11context.display, x11context.root_window, 0, 0, request_width, request_height, border,
                            x11context.depth, InputOutput, x11context.visual, attribute_mask, &attributes);
  assert (m_window != 0);
  x11context.x11ids[m_window] = this;
  // adjust X hints & settings
  vector<Atom> atoms;
  atoms.push_back (x11context.atom ("WM_DELETE_WINDOW")); // request client messages instead of XKillClient
  atoms.push_back (x11context.atom ("WM_TAKE_FOCUS"));
  atoms.push_back (x11context.atom ("_NET_WM_PING"));
  XSetWMProtocols (x11context.display, m_window, atoms.data(), atoms.size());
  // FIXME: set Window hints
  // window setup
  this->setup (setup);
  configure (config);
  // configure state for this window
  {
    XConfigureEvent xev = { ConfigureNotify, create_serial, true, x11context.display, m_window, m_window,
                            0, 0, request_width, request_height, border, /*above*/ 0, m_override_redirect };
    XEvent xevent;
    xevent.xconfigure = xev;
    process_event (xevent);
  }
}

ScreenWindowX11::~ScreenWindowX11()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  if (m_expose_surface)
    {
      cairo_surface_destroy (m_expose_surface);
      m_expose_surface = NULL;
    }
  if (m_window)
    {
      XDestroyWindow (x11context.display, m_window);
      x11context.x11ids.erase (m_window);
      m_window = 0;
    }
  x11locker.unlock();
  m_x11driver.close();
}

void
ScreenWindowX11::show (void)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  XMapWindow (x11context.display, m_window);
}

struct PendingSensor {
  Drawable window;
  int pending_configures, pending_exposes;
};

static Bool
pending_event_sensor (Display *display, XEvent *event, XPointer arg)
{
  PendingSensor &ps = *(PendingSensor*) arg;
  if (event->xany.window == ps.window)
    switch (event->xany.type)
      {
      case Expose:              ps.pending_exposes++;           break;
      case ConfigureNotify:     ps.pending_configures++;        break;
      }
  return False;
}

static void
check_pending (Display *display, Drawable window, int *pending_configures, int *pending_exposes)
{
  XEvent dummy;
  PendingSensor ps = { window, 0, 0 };
  XCheckIfEvent (display, &dummy, pending_event_sensor, XPointer (&ps));
  *pending_configures = ps.pending_configures;
  *pending_exposes = ps.pending_exposes;
}

bool
ScreenWindowX11::process_event (const XEvent &xevent)
{
  m_event_context.synthesized = xevent.xany.send_event;
  const char ss = m_event_context.synthesized ? 'S' : 's';
  bool consumed = false;
  switch (xevent.type)
    {
    case CreateNotify: {
      const XCreateWindowEvent &xev = xevent.xcreatewindow;
      EDEBUG ("Creat: %c=%lu w=%lu a=%+d%+d%+dx%d b=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.border_width);
      consumed = true;
      break; }
    case ConfigureNotify: {
      const XConfigureEvent &xev = xevent.xconfigure;
      if (xev.window != m_window)
        break;
      EDEBUG ("Confg: %c=%lu w=%lu a=%+d%+d%+dx%d b=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.border_width);
      if (m_state.width != xev.width || m_state.height != xev.height)
        {
          m_state.width = xev.width;
          m_state.height = xev.height;
          if (m_expose_surface)
            {
              cairo_surface_destroy (m_expose_surface);
              m_expose_surface = NULL;
            }
          m_expose_region.clear();
        }
      if (m_pending_configures)
        m_pending_configures--;
      if (!m_pending_configures)
        check_pending (x11context.display, m_window, &m_pending_configures, &m_pending_exposes);
      enqueue_locked (create_event_win_size (m_event_context, m_state.width, m_state.height, m_pending_configures > 0));
      consumed = true;
      break; }
    case MapNotify: {
      const XMapEvent &xev = xevent.xmap;
      if (xev.window != m_window)
        break;
      EDEBUG ("Map  : %c=%lu w=%lu e=%lu", ss, xev.serial, xev.window, xev.event);
      m_mapped = true;
      consumed = true;
      break; }
    case UnmapNotify: {
      const XUnmapEvent &xev = xevent.xunmap;
      if (xev.window != m_window)
        break;
      EDEBUG ("Unmap: %c=%lu w=%lu e=%lu", ss, xev.serial, xev.window, xev.event);
      m_mapped = false;
      enqueue_locked (create_event_cancellation (m_event_context));
      consumed = true;
      break; }
    case ReparentNotify:  {
      const XReparentEvent &xev = xevent.xreparent;
      EDEBUG ("Rprnt: %c=%lu w=%lu p=%lu @=%+d%+d ovr=%d", ss, xev.serial, xev.window, xev.parent, xev.x, xev.y, xev.override_redirect);
      consumed = true;
      break; }
    case VisibilityNotify: {
      const XVisibilityEvent &xev = xevent.xvisibility;
      EDEBUG ("Visbl: %c=%lu w=%lu notify=%s", ss, xev.serial, xev.window, visibility_state (xev.state));
      consumed = true;
      break; }
    case PropertyNotify: {
      const XPropertyEvent &xev = xevent.xproperty;
      const bool deleted = xev.state == PropertyDelete;
      EDEBUG ("Prop%c: %c=%lu w=%lu prop=%s", deleted ? 'D' : 'C', ss, xev.serial, xev.window, x11context.atom (xev.atom).c_str());
      m_event_context.time = xev.time;
      property_changed (xev.atom, deleted);
      consumed = true;
      break; }
    case Expose: {
      const XExposeEvent &xev = xevent.xexpose;
      std::vector<Rect> rectangles;
      m_expose_region.add (Rect (Point (xev.x, xev.y), xev.width, xev.height));
      if (m_pending_exposes)
        m_pending_exposes--;
      if (!m_pending_exposes && xev.count == 0)
        check_pending (x11context.display, m_window, &m_pending_configures, &m_pending_exposes);
      String hint = m_pending_exposes ? " (E+)" : m_expose_surface ? "" : " (nodata)";
      EDEBUG ("Expos: %c=%lu w=%lu a=%+d%+d%+dx%d c=%d%s", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.count, hint.c_str());
      if (!m_pending_exposes && xev.count == 0)
        blit_expose_region();
      consumed = true;
      break; }
    case KeyPress: case KeyRelease: {
      const XKeyEvent &xev = xevent.xkey;
      const char  *kind = xevent.type == KeyPress ? "dn" : "up";
      const KeySym ksym = XKeycodeToKeysym (x11context.display, xev.keycode, 0);
      const char  *kstr = XKeysymToString (ksym);
      EDEBUG ("KEY%s: %c=%lu w=%lu c=%lu p=%+d%+d k=%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, kstr);
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_key (xevent.type == KeyPress ? KEY_PRESS : KEY_RELEASE, m_event_context, KeyValue (ksym), kstr));
      consumed = true;
      break; }
    case ButtonPress: case ButtonRelease: {
      const XButtonEvent &xev = xevent.xbutton;
      const char  *kind = xevent.type == ButtonPress ? "dn" : "up";
      if (xev.window != m_window)
        break;
      EDEBUG ("BUT%s: %c=%lu w=%lu c=%lu p=%+d%+d b=%d", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.button);
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      if (xevent.type == ButtonPress)
        switch (xev.button)
          {
          case 4:  enqueue_locked (create_event_scroll (SCROLL_UP, m_event_context));                break;
          case 5:  enqueue_locked (create_event_scroll (SCROLL_DOWN, m_event_context));              break;
          case 6:  enqueue_locked (create_event_scroll (SCROLL_LEFT, m_event_context));              break;
          case 7:  enqueue_locked (create_event_scroll (SCROLL_RIGHT, m_event_context));             break;
          default: enqueue_locked (create_event_button (BUTTON_PRESS, m_event_context, xev.button)); break;
          }
      else // ButtonRelease
        switch (xev.button)
          {
          case 4: case 5: case 6: case 7: break; // scrolling
          default: enqueue_locked (create_event_button (BUTTON_RELEASE, m_event_context, xev.button)); break;
          }
      consumed = true;
      break; }
    case MotionNotify: {
      const XMotionEvent &xev = xevent.xmotion;
      if (xev.window != m_window)
        break;
      if (xev.is_hint)
        {
          int nevents = 0;
          XTimeCoord *xcoords = XGetMotionEvents (x11context.display, m_window, m_last_motion_time + 1, xev.time - 1, &nevents);
          if (xcoords)
            {
              for (int i = 0; i < nevents; ++i)
                if (xcoords[i].x == 0)
                  {
                    /* xserver-xorg-1:7.6+7ubuntu7 + libx11-6:amd64-2:1.4.4-2ubuntu1 may generate buggy motion
                     * history events with x=0 for X220t trackpoints.
                     */
                    EDEBUG ("  ...: S=%lu w=%lu c=%lu p=%+d%+d (discarding)", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
                else
                  {
                    m_event_context.time = xcoords[i].time; m_event_context.x = xcoords[i].x; m_event_context.y = xcoords[i].y;
                    enqueue_locked (create_event_mouse (MOUSE_MOVE, m_event_context));
                    EDEBUG ("  ...: S=%lu w=%lu c=%lu p=%+d%+d", xev.serial, xev.window, xev.subwindow, xcoords[i].x, xcoords[i].y);
                  }
              XFree (xcoords);
            }
        }
      EDEBUG ("Mtion: %c=%lu w=%lu c=%lu p=%+d%+d%s", ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y, xev.is_hint ? " (hint)" : "");
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_mouse (MOUSE_MOVE, m_event_context));
      m_last_motion_time = xev.time;
      consumed = true;
      break; }
    case EnterNotify: case LeaveNotify: {
      const XCrossingEvent &xev = xevent.xcrossing;
      const EventType etype = xevent.type == EnterNotify ? MOUSE_ENTER : MOUSE_LEAVE;
      const char *kind = xevent.type == EnterNotify ? "Enter" : "Leave";
      EDEBUG ("%s: %c=%lu w=%lu c=%lu p=%+d%+d notify=%s+%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y,
              notify_mode (xev.mode), notify_detail (xev.detail));
      m_event_context.time = xev.time; m_event_context.x = xev.x; m_event_context.y = xev.y; m_event_context.modifiers = ModifierState (xev.state);
      enqueue_locked (create_event_mouse (xev.detail == NotifyInferior ? MOUSE_MOVE : etype, m_event_context));
      if (xev.detail != NotifyInferior)
        {
          m_crossing_focus = xev.focus;
          m_last_motion_time = xev.time;
        }
      consumed = true;
      break; }
    case FocusIn: case FocusOut: {
      const XFocusChangeEvent &xev = xevent.xfocus;
      const char *kind = xevent.type == FocusIn ? "FocIn" : "FcOut";
      EDEBUG ("%s: %c=%lu w=%lu notify=%s+%s", kind, ss, xev.serial, xev.window, notify_mode (xev.mode), notify_detail (xev.detail));
      if (!(xev.detail == NotifyInferior ||     // subwindow focus changed
            xev.detail == NotifyPointer ||      // pointer focus changed
            xev.detail == NotifyPointerRoot ||  // root focus changed
            xev.detail == NotifyDetailNone))    // root focus is discarded
        {
          const bool last_focus = m_win_focus;
          m_win_focus = xevent.type == FocusIn;
          if (last_focus != m_win_focus)
            enqueue_locked (create_event_focus (m_win_focus ? FOCUS_IN : FOCUS_OUT, m_event_context));
        }
      consumed = true;
      break; }
    case ClientMessage: {
      const XClientMessageEvent &xev = xevent.xclient;
      if (debug_enabled()) // avoid atom() round-trips
        {
          const Atom mtype = xev.message_type == x11context.atom ("WM_PROTOCOLS") ? xev.data.l[0] : xev.message_type;
          EDEBUG ("ClMsg: %c=%lu w=%lu t=%s f=%u", ss, xev.serial, xev.window, x11context.atom (mtype).c_str(), xev.format);
        }
      client_message (xev);
      consumed = true;
      break; }
    case DestroyNotify: {
      const XDestroyWindowEvent &xev = xevent.xdestroywindow;
      if (xev.window != m_window)
        break;
      EDEBUG ("Destr: %c=%lu w=%lu", ss, xev.serial, xev.window);
      x11context.x11ids.erase (m_window);
      m_window = 0;
      consumed = true;
      break; }
    default: ;
    }
  return consumed;
}

void
ScreenWindowX11::property_changed (Atom atom, bool deleted)
{
  const String atom_name = x11context.atom (atom);
  State old_state = m_state;
  if (debug_enabled() && (atom_name == "WM_NAME" || atom_name == "_NET_WM_NAME"))
    {
      String text = x11_get_string_property (x11context.display, m_window, atom);
      EDEBUG ("State: %s: %s", atom_name.c_str(), text.c_str());
    }
  else if (atom_name == "WM_STATE")
    {
      vector<uint32> datav = x11_get_property_data<uint32> (x11context.display, m_window, atom);
      if (datav.size())
        m_state.window_flags = Flags ((m_state.window_flags & ~ICONIFY) | (datav[0] == IconicState ? ICONIFY : 0));
    }
  else if (atom_name == "_NET_WM_STATE")
    {
      vector<uint32> datav = x11_get_property_data<uint32> (x11context.display, m_window, atom);
      uint32 f = 0;
      for (size_t i = 0; i < datav.size(); i++)
        if      (datav[i] == x11context.atom ("_NET_WM_STATE_MODAL"))           f += MODAL;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_STICKY"))          f += STICKY;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_MAXIMIZED_VERT"))  f += VMAXIMIZED;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_MAXIMIZED_HORZ"))  f += HMAXIMIZED;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_SHADED"))          f += SHADED;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_SKIP_TASKBAR"))    f += SKIP_TASKBAR;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_SKIP_PAGER"))      f += SKIP_PAGER;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_HIDDEN"))          f += HIDDEN;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_FULLSCREEN"))      f += FULLSCREEN;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_ABOVE"))           f += ABOVE_ALL;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_BELOW"))           f += BELOW_ALL;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_DEMANDS_ATTENTION")) f += ATTENTION;
        else if (datav[i] == x11context.atom ("_NET_WM_STATE_FOCUSED"))         f += FOCUS_DECO;
      m_state.window_flags = Flags ((m_state.window_flags & ~_WM_STATE_MASK) | f);
    }
  else if (atom_name == "_MOTIF_WM_HINTS")
    {
      Mwm funcs = Mwm (FUNC_CLOSE | FUNC_MINIMIZE | FUNC_MAXIMIZE), deco = DECOR_ALL;
      get_mwm_hints (x11context.display, m_window, &funcs, &deco);
      uint32 f = 0;
      if (funcs & FUNC_CLOSE)                                                   f += DELETABLE;
      if (funcs & FUNC_MINIMIZE || deco & DECOR_MINIMIZE)                       f += MINIMIZABLE;
      if (funcs & FUNC_MAXIMIZE || deco & DECOR_MAXIMIZE)                       f += MAXIMIZABLE;
      if (deco & (DECOR_ALL | DECOR_BORDER | DECOR_RESIZEH | DECOR_TITLE))      f += DECORATED;
      m_state.window_flags = Flags ((m_state.window_flags & ~_DECO_MASK) | f);
    }
  if (debug_enabled())
    {
      if (old_state.window_flags != m_state.window_flags)
        EDEBUG ("State: flags=%s", flags_name (m_state.window_flags).c_str());
    }
}

void
ScreenWindowX11::client_message (const XClientMessageEvent &xev)
{
  const Atom mtype = xev.message_type == x11context.atom ("WM_PROTOCOLS") ? xev.data.l[0] : xev.message_type;
  if      (mtype == x11context.atom ("WM_DELETE_WINDOW"))
    {
      const uint32 saved_time = m_event_context.time;
      m_event_context.time = xev.data.l[1];
      enqueue_locked (create_event_win_delete (m_event_context));
      m_event_context.time = saved_time; // avoid time warps from client messages
    }
  else if (mtype == x11context.atom ("WM_TAKE_FOCUS"))
    {
      XErrorEvent dummy = { 0, };
      x11_trap_errors (&dummy); // guard against being unmapped
      XSetInputFocus (x11context.display, m_window, RevertToPointerRoot, xev.data.l[1]);
      XSync (x11context.display, False);
      x11_untrap_errors();
    }
  else if (mtype == x11context.atom ("_NET_WM_PING"))
    {
      XEvent xevent = *(XEvent*) &xev;
      xevent.xclient.data.l[3] = xevent.xclient.data.l[4] = 0; // [0]=_PING, [1]=time, [2]=m_window
      xevent.xclient.window = x11context.root_window;
      XSendEvent (x11context.display, xevent.xclient.window, False, SubstructureNotifyMask | SubstructureRedirectMask, &xevent);
    }
}

static Rect
cairo_image_surface_coverage (cairo_surface_t *surface)
{
  int w = cairo_image_surface_get_width (surface);
  int h = cairo_image_surface_get_height (surface);
  double x_offset = 0, y_offset = 0;
  cairo_surface_get_device_offset (surface, &x_offset, &y_offset);
  return Rect (Point (x_offset, y_offset), w, h);
}

void
ScreenWindowX11::blit_surface (cairo_surface_t *surface, Rapicorn::Region region)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  CHECK_CAIRO_STATUS (surface);
  if (!m_window)
    return;
  const Rect fullwindow = Rect (0, 0, m_state.width, m_state.height);
  if (region.count_rects() == 1 && fullwindow == region.extents() && fullwindow == cairo_image_surface_coverage (surface))
    {
      // special case, surface matches exactly the entire window
      if (m_expose_surface)
        cairo_surface_destroy (m_expose_surface);
      m_expose_surface = cairo_surface_reference (surface);
    }
  else
    {
      if (!m_expose_surface)
        m_expose_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, m_state.width, m_state.height);
      cairo_t *cr = cairo_create (m_expose_surface);
      // clip to region
      vector<Rect> rects;
      region.list_rects (rects);
      for (size_t i = 0; i < rects.size(); i++)
        cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      cairo_clip (cr);
      // render onto m_expose_surface
      cairo_set_source_surface (cr, surface, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_paint (cr);
      // cleanup
      cairo_destroy (cr);
    }
  // redraw expose region
  m_expose_region.add (region);
  blit_expose_region();
}

void
ScreenWindowX11::blit_expose_region()
{
  if (!m_expose_surface)
    {
      m_expose_region.clear();
      return;
    }
  CHECK_CAIRO_STATUS (m_expose_surface);
  const unsigned long blit_serial = XNextRequest (x11context.display) - 1;
  // surface for drawing on the X11 window
  cairo_surface_t *xsurface = cairo_xlib_surface_create (x11context.display, m_window, x11context.visual, m_state.width, m_state.height);
  CHECK_CAIRO_STATUS (xsurface);
  // cairo context
  cairo_t *xcr = cairo_create (xsurface);
  CHECK_CAIRO_STATUS (xcr);
  // clip to m_expose_region
  vector<Rect> rects;
  m_expose_region.list_rects (rects);
  uint coverage = 0;
  for (size_t i = 0; i < rects.size(); i++)
    {
      cairo_rectangle (xcr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      coverage += rects[i].width * rects[i].height;
    }
  cairo_clip (xcr);
  // paint m_expose_region
  cairo_set_source_surface (xcr, m_expose_surface, 0, 0);
  cairo_set_operator (xcr, CAIRO_OPERATOR_OVER);
  cairo_paint (xcr);
  CHECK_CAIRO_STATUS (xcr);
  XFlush (x11context.display);
  // debugging info
  EDEBUG ("BlitS: S=%lu nrects=%zu coverage=%.1f%%", blit_serial, rects.size(), coverage * 100.0 / (m_state.width * m_state.height));
  // cleanup
  m_expose_region.clear();
  cairo_destroy (xcr);
  cairo_surface_destroy (xsurface);
}

void
ScreenWindowX11::enqueue_locked (Event *event)
{
  // ScopedLock<Mutex> x11locker (x11_rmutex);
  m_receiver.enqueue_async (event);
}

ScreenWindow::State
ScreenWindowX11::get_state ()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  return m_state;
}

static void
cairo_set_source_color (cairo_t *cr, Color c)
{
  cairo_set_source_rgba (cr, c.red() / 255., c.green() / 255., c.blue() / 255., c.alpha() / 255.);
}

static Pixmap
create_checkerboard_pixmap (Display *display, Visual *visual, Drawable drawable, uint depth,
                            int tile_size, Color c1, Color c2)
{
  const int bw = tile_size * 2, bh = tile_size * 2;
  Pixmap xpixmap = XCreatePixmap (display, drawable, bw, bh, depth);
  cairo_surface_t *xsurface = cairo_xlib_surface_create (display, xpixmap, visual, bw, bh);
  CHECK_CAIRO_STATUS (xsurface);
  cairo_t *cr = cairo_create (xsurface);
  cairo_set_source_color (cr, c1);
  cairo_rectangle (cr,         0,         0, tile_size, tile_size);
  cairo_rectangle (cr, tile_size, tile_size, tile_size, tile_size);
  cairo_fill (cr);
  cairo_set_source_color (cr, c2);
  cairo_rectangle (cr,         0, tile_size, tile_size, tile_size);
  cairo_rectangle (cr, tile_size,         0, tile_size, tile_size);
  cairo_fill (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (xsurface);
  return xpixmap;
}

void
ScreenWindowX11::setup (const ScreenWindow::Setup &setup)
{
  // window is not yet mapped
  m_state.setup.window_type = setup.window_type;
  // _NET_WM_STATE
  m_state.setup.request_flags = setup.request_flags;
  vector<unsigned long> longs;
  const uint64 f = m_state.setup.request_flags;
  if (f & MODAL)        longs.push_back (x11context.atom ("_NET_WM_STATE_MODAL"));
  if (f & STICKY)       longs.push_back (x11context.atom ("_NET_WM_STATE_STICKY"));
  if (f & VMAXIMIZED)   longs.push_back (x11context.atom ("_NET_WM_STATE_MAXIMIZED_VERT"));
  if (f & HMAXIMIZED)   longs.push_back (x11context.atom ("_NET_WM_STATE_MAXIMIZED_HORZ"));
  if (f & SHADED)       longs.push_back (x11context.atom ("_NET_WM_STATE_SHADED"));
  if (f & SKIP_TASKBAR) longs.push_back (x11context.atom ("_NET_WM_STATE_SKIP_TASKBAR"));
  if (f & SKIP_PAGER)   longs.push_back (x11context.atom ("_NET_WM_STATE_SKIP_PAGER"));
  if (f & FULLSCREEN)   longs.push_back (x11context.atom ("_NET_WM_STATE_FULLSCREEN"));
  if (f & ABOVE_ALL)    longs.push_back (x11context.atom ("_NET_WM_STATE_ABOVE"));
  if (f & BELOW_ALL)    longs.push_back (x11context.atom ("_NET_WM_STATE_BELOW"));
  if (f & ATTENTION)    longs.push_back (x11context.atom ("_NET_WM_STATE_DEMANDS_ATTENTION"));
  XChangeProperty (x11context.display, m_window, x11context.atom ("_NET_WM_STATE"),
                   XA_ATOM, 32, PropModeReplace, (uint8*) longs.data(), longs.size());
  // WM_HINTS
  XWMHints wmhints = { InputHint | StateHint, False, NormalState, 0, 0, 0, 0, 0, 0, };
  if (f & ATTENTION)
    wmhints.flags |= XUrgencyHint;
  if (f & (HIDDEN | ICONIFY))
    wmhints.initial_state = IconicState;
  XSetWMHints (x11context.display, m_window, &wmhints);
  // _MOTIF_WM_HINTS
  uint32 funcs = FUNC_RESIZE | FUNC_MOVE, deco = 0;
  if (f & DECORATED)    { deco |= DECOR_BORDER | DECOR_RESIZEH | DECOR_TITLE | DECOR_MENU; }
  if (f & MINIMIZABLE)  { funcs |= FUNC_MINIMIZE; deco |= DECOR_MINIMIZE; }
  if (f & MAXIMIZABLE)  { funcs |= FUNC_MAXIMIZE; deco |= DECOR_MAXIMIZE; }
  if (f & DELETABLE)    { funcs |= FUNC_CLOSE; deco |= DECOR_CLOSE; }
  adjust_mwm_hints (x11context.display, m_window, Mwm (funcs), Mwm (deco));
  // session role
  m_state.setup.session_role = setup.session_role;
  set_text_property (x11context.display, m_window, x11context.atom ("WM_WINDOW_ROLE"),
                     XStringStyle, setup.session_role, DELETE_EMPTY);   // ICCCM
  // background
  m_state.setup.bg_average = setup.bg_average;
  Color c1 = setup.bg_average, c2 = setup.bg_average;
  if (devel_enabled())
    {
      c1.tint (0, 0.96, 0.96);
      c2.tint (0, 1.03, 1.03);
    }
  Pixmap xpixmap = create_checkerboard_pixmap (x11context.display, x11context.visual, m_window, x11context.depth, 8, c1, c2);
  XSetWindowBackgroundPixmap (x11context.display, m_window, xpixmap); // m_state.setup.bg_average ?
  XFreePixmap (x11context.display, xpixmap);
}

void
ScreenWindowX11::configure (const Config &config)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  // WM size & gravity hints
  XSizeHints szhint = { PWinGravity, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, { 0, 0 }, 0, 0, StaticGravity };
  XSetWMNormalHints (x11context.display, m_window, &szhint);
  // title
  if (config.title != m_state.config.title)
    {
      set_text_property (x11context.display, m_window, x11context.atom ("WM_NAME"), XStdICCTextStyle, config.title);      // ICCCM
      set_text_property (x11context.display, m_window, x11context.atom ("_NET_WM_NAME"), XUTF8StringStyle, config.title); // EWMH
      m_state.config.title = config.title;
    }
  // iconified title
  if (config.alias != m_state.config.alias)
    {
      set_text_property (x11context.display, m_window, x11context.atom ("WM_ICON_NAME"), XStdICCTextStyle, config.alias, DELETE_EMPTY);
      set_text_property (x11context.display, m_window, x11context.atom ("_NET_WM_ICON_NAME"), XUTF8StringStyle, config.alias, DELETE_EMPTY);
      m_state.config.alias = config.alias;
    }
  enqueue_locked (create_event_win_size (m_event_context, m_state.width, m_state.height, m_pending_configures > 0));
}

bool
ScreenWindowX11::viewable (void)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  return m_mapped;
}

void
ScreenWindowX11::beep()
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  XBell (x11context.display, 0);
}

void
ScreenWindowX11::present () { /*FIXME*/ }

void
ScreenWindowX11::start_user_move (uint button, double root_x, double root_y) { /*FIXME*/ }

void
ScreenWindowX11::start_user_resize (uint button, double root_x, double root_y, AnchorType edge) { /*FIXME*/ }

// == X11 thread ==
class X11Thread {
  X11Context &x11context;
  EventFd     m_wakeup;
  Atomic<int> m_running;
  std::thread m_thread_handle;
  bool
  filter_event (const XEvent &xevent)
  {
    switch (xevent.type)
      {
      case MappingNotify:
        XRefreshKeyboardMapping (const_cast<XMappingEvent*> (&xevent.xmapping));
        return true;
      default:
        return false;
      }
  }
  void
  process_x11()
  {
    while (XPending (x11context.display))
      {
        XEvent xevent = { 0, };
        XNextEvent (x11context.display, &xevent);
        bool consumed = filter_event (xevent);
        X11Item *xitem = x11context.x11ids[xevent.xany.window];
        if (xitem)
          {
            ScreenWindowX11 *scw = dynamic_cast<ScreenWindowX11*> (xitem);
            if (scw)
              consumed = scw->process_event (xevent);
          }
        if (!consumed)
          {
            const char ss = xevent.xany.send_event ? 'S' : 's';
            EDEBUG ("Spare: %c=%lu event_type=%d w=%lu", ss, xevent.xany.serial, xevent.type, xevent.xany.window);
          }
      }
  }
  void
  run()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    for (;;)
      {
        process_x11();
        struct pollfd pfds[] = {
          { m_wakeup.inputfd(), PollFD::IN, 0 },
          { ConnectionNumber (x11context.display), PollFD::IN, 0 },
        };
        int presult;
        do
          {
            x11locker.unlock();
            presult = poll (pfds, ARRAY_SIZE (pfds), -1);
            x11locker.lock();
          }
        while (presult < 0 && (errno == EAGAIN || errno == EINTR));
        if (pfds[0].revents != 0)
          {
            m_wakeup.flush();
            if (!m_running.load())
              break;
            critical ("X11Thread: seen wakeup without stop-notification, m_running=%d (addr=%p)", int (m_running), &m_running);
          }
        if (pfds[1].revents != 0)
          process_x11();
      }
  }
public:
  X11Thread (X11Context &ix11context) :
    x11context (ix11context), m_running (0)
  {
    int err = m_wakeup.open();
    if (err)
      fatal ("failed to open wakeup file descriptor: %s", strerror (err));
  }
  void
  start()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    assert_return (std::thread::id() == m_thread_handle.get_id());
    m_running.store (true);
    m_thread_handle = std::thread (&X11Thread::run, this);
  }
  void
  stop()
  {
    assert_return (m_thread_handle.joinable());
    m_running.store (false);
    m_wakeup.wakeup();
    m_thread_handle.join();
  }
  ~X11Thread()
  {
    assert_return (!m_thread_handle.joinable());
  }
};

static X11Thread*
start_x11_thread (X11Context &x11context)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  X11Thread *x11_thread = new X11Thread (x11context);
  x11_thread->start();
  return x11_thread;
}

static void
stop_x11_thread (X11Thread *x11_thread)
{
  assert_return (x11_thread != NULL);
  x11_thread->stop();
  ScopedLock<Mutex> x11locker (x11_rmutex);
  delete x11_thread;
}

} // Rapicorn
