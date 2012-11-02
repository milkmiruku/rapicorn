// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "screenwindow.hh"
#include <ui/utilities.hh>
#include <cairo/cairo-xlib.h>
#include <fcntl.h> // for wakeup pipe
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>

#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("XEvents", __VA_ARGS__)
#define XDEBUG(...)     RAPICORN_KEY_DEBUG ("X11", __VA_ARGS__)

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

namespace { // Anon
using namespace Rapicorn;

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

// == X11 Error Handling ==
static XErrorHandler xlib_error_handler = NULL;
static XErrorEvent *xlib_error_trap = NULL;
static int
x11_error (Display *error_display, XErrorEvent *error_event)
{
  if (xlib_error_trap)
    {
      *xlib_error_trap = *error_event;
      return 0;
    }
  int result = -1;
  try
    {
      atexit (abort);
      result = xlib_error_handler (error_display, error_event);
    }
  catch (...)
    {
      // ignore C++ exceptions from atexit handlers
    }
  abort();
  return result;
}

static void
x11_trap_errors (XErrorEvent *trapped_event)
{
  assert_return (xlib_error_trap == NULL);
  xlib_error_trap = trapped_event;
  trapped_event->error_code = 0;
}

static int
x11_untrap_errors()
{
  assert_return (xlib_error_trap != NULL, -1);
  const int error_code = xlib_error_trap->error_code;
  xlib_error_trap = NULL;
  return error_code;
}

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
  bool                  local_x11 ();
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
  if (1) // FIXME: toggle sync
    XSynchronize (display, true);
  screen = DefaultScreen (display);
  visual = DefaultVisual (display, screen);
  depth = DefaultDepth (display, screen);
  root_window = XRootWindow (display, screen);
}

X11Context::~X11Context ()
{
  if (x11_thread)
    {
      stop_x11_thread (x11_thread);
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
  Atom a = XInternAtom (display, text.c_str(), !force_create);
  // FIXME: optimize roundtrips
  return a;
}

String
X11Context::atom (Atom atom) const
{
  char *res = XGetAtomName (display, atom);
  // FIXME: optimize roundtrips
  String result (res ? res : "");
  if (res)
    XFree (res);
  return result;
}

bool
X11Context::local_x11()
{
  if (m_shared_mem >= 0)
    return m_shared_mem;
  m_shared_mem = 0;
  XShmSegmentInfo shminfo = { 0 /*shmseg*/, -1 /*shmid*/, (char*) -1 /*shmaddr*/, True /*readOnly*/ };
  XImage *ximage = XShmCreateImage (display, visual, depth, ZPixmap, NULL, &shminfo, 1, 1);
  if (ximage)
    {
      shminfo.shmid = shmget (IPC_PRIVATE, ximage->bytes_per_line * ximage->height, IPC_CREAT | 0600);
      if (shminfo.shmid != -1)
        {
          shminfo.shmaddr = (char*) shmat (shminfo.shmid, NULL, SHM_RDONLY);
          if (ptrdiff_t (shminfo.shmaddr) != -1)
            {
              XErrorEvent dummy = { 0, };
              x11_trap_errors (&dummy);
              Bool result = XShmAttach (display, &shminfo);
              XSync (display, False); // forces error delivery
              if (!x11_untrap_errors())
                {
                  // if we got here, shared memory works
                  m_shared_mem = result != 0;
                }
            }
          shmctl (shminfo.shmid, IPC_RMID, NULL); // delete the shm segment upon last detaching process
          // cleanup
          if (m_shared_mem)
            XShmDetach (display, &shminfo);
          if (ptrdiff_t (shminfo.shmaddr) != -1)
            shmdt (shminfo.shmaddr);
        }
      XDestroyImage (ximage);
    }
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
  virtual ScreenWindow* create_screen_window (const ScreenWindow::Setup &setup, ScreenWindow::EventReceiver &receiver);
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
  explicit              ScreenWindowX11         (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, EventReceiver &receiver);
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
  void                  blit_expose_region      ();
};

ScreenWindow*
ScreenDriverX11::create_screen_window (const ScreenWindow::Setup &setup, ScreenWindow::EventReceiver &receiver)
{
  assert_return (m_open_count > 0, NULL);
  return new ScreenWindowX11 (*this, setup, receiver);
}

ScreenWindowX11::ScreenWindowX11 (ScreenDriverX11 &x11driver, const ScreenWindow::Setup &setup, EventReceiver &receiver) :
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
  attributes.event_mask        = ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask |
                                 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                                 PointerMotionMask | PointerMotionHintMask | ButtonMotionMask |
                                 Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
  attributes.background_pixel  = XWhitePixel (x11context.display, x11context.screen);
  attributes.border_pixel      = XBlackPixel (x11context.display, x11context.screen);
  attributes.override_redirect = m_override_redirect;
  attributes.win_gravity       = StaticGravity;
  attributes.bit_gravity       = StaticGravity;
  attributes.save_under        = m_override_redirect;
  attributes.backing_store     = x11context.local_x11() ? NotUseful : WhenMapped;
  unsigned long attribute_mask = CWWinGravity | CWBitGravity | CWBackingStore | CWSaveUnder |
                                 CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask;
  const int border = 3, request_width = 33, request_height = 33;
  m_window = XCreateWindow (x11context.display, x11context.root_window, 0, 0, request_width, request_height, border,
                            x11context.depth, InputOutput, x11context.visual, attribute_mask, &attributes);
  if (m_window)
    x11context.x11ids[m_window] = this;
  printerr ("window: %lu\n", m_window);
  // window setup
  this->setup (setup);
  // FIXME:  set Window hints & focus
  // configure state for this window
  {
    XConfigureEvent xev = { ConfigureNotify, /*serial*/ 0, true, x11context.display, m_window, m_window,
                            0, 0, request_width, request_height, border, /*above*/ 0, m_override_redirect };
    XEvent xevent;
    xevent.xconfigure = xev;
    process_event (xevent);
  }
}

ScreenWindowX11::~ScreenWindowX11()
{
  if (m_window)
    x11context.x11ids.erase (m_window);
  m_x11driver.close();
}

void
ScreenWindowX11::show (void)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  XMapWindow (x11context.display, m_window);
}

static const char*
notify_mode (int notify_type)
{
  switch (notify_type)
    {
    case NotifyNormal:          return "Normal";
    case NotifyGrab:            return "Grab";
    case NotifyUngrab:          return "Ungrab";
    case NotifyWhileGrabbed:    return "WhileGrabbed";
    default:                    return "Unknown";
    }
}

static const char*
notify_detail (int notify_type)
{
  switch (notify_type)
    {
    case NotifyAncestor:         return "Ancestor";
    case NotifyVirtual:          return "Virtual";
    case NotifyInferior:         return "Inferior";
    case NotifyNonlinear:        return "Nonlinear";
    case NotifyNonlinearVirtual: return "NonlinearVirtual";
    case NotifyPointer:          return "NotifyPointer";
    case NotifyPointerRoot:      return "NotifyPointerRoot";
    case NotifyDetailNone:       return "NotifyDetailNone";
    default:                     return "Unknown";
    }
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
      if (xev.event != xev.window)
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
      if (xev.event != xev.window)
        break;
      EDEBUG ("Map  : %c=%lu w=%lu", ss, xev.serial, xev.window);
      m_mapped = true;
      consumed = true;
      break; }
    case UnmapNotify: {
      const XUnmapEvent &xev = xevent.xunmap;
      if (xev.event != xev.window)
        break;
      EDEBUG ("Unmap: %c=%lu w=%lu", ss, xev.serial, xev.window);
      m_mapped = false;
      consumed = true;
      break; }
    case Expose: {
      const XExposeEvent &xev = xevent.xexpose;
      EDEBUG ("Expos: %c=%lu w=%lu a=%+d%+d%+dx%d c=%d", ss, xev.serial, xev.window, xev.x, xev.y, xev.width, xev.height, xev.count);
      std::vector<Rect> rectangles;
      m_expose_region.add (Rect (Point (xev.x, xev.y), xev.width, xev.height));
      if (m_pending_exposes)
        m_pending_exposes--;
      if (!m_pending_exposes && xev.count == 0)
        check_pending (x11context.display, m_window, &m_pending_configures, &m_pending_exposes);
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
      EDEBUG ("%s: %c=%lu w=%lu c=%lu p=%+d%+d Notify:%s+%s", kind, ss, xev.serial, xev.window, xev.subwindow, xev.x, xev.y,
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
      EDEBUG ("%s: %c=%lu w=%lu Notify:%s+%s", kind, ss, xev.serial, xev.window, notify_mode (xev.mode), notify_detail (xev.detail));
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
    default: ;
    }
  return consumed;
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

enum XPEmpty { KEEP_EMPTY, DELETE_EMPTY };

static bool
set_text_property (X11Context &x11context, Window window, const String &property, XICCEncodingStyle ecstyle,
                   const String &value, XPEmpty when_empty = KEEP_EMPTY)
{
  bool success = true;
  if (when_empty == DELETE_EMPTY && value.empty())
    XDeleteProperty (x11context.display, window, x11context.atom (property));
  else if (ecstyle == XUTF8StringStyle)
    XChangeProperty (x11context.display, window, x11context.atom (property), x11context.atom ("UTF8_STRING"), 8,
                     PropModeReplace, (uint8*) value.c_str(), value.size());
  else
    {
      char *text = const_cast<char*> (value.c_str());
      XTextProperty xtp = { 0, };
      const int result = Xutf8TextListToTextProperty (x11context.display, &text, 1, ecstyle, &xtp);
      printerr ("XUTF8CONVERT: target=%s len=%zd result=%d: %s -> %s\n", x11context.atom(xtp.encoding).c_str(), value.size(), result, text, xtp.value);
      if (result >= 0 && xtp.nitems && xtp.value)
        XChangeProperty (x11context.display, window, x11context.atom (property), xtp.encoding, xtp.format,
                         PropModeReplace, xtp.value, xtp.nitems);
      else
        success = false;
      if (xtp.value)
        XFree (xtp.value);
    }
  return success;
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
  m_state.setup.window_flags = setup.window_flags;
  if (setup.modal)
    FIXME ("modal window's unimplemented");
  m_state.setup.modal = setup.modal;
  m_state.setup.bg_average = setup.bg_average;
  set_text_property (x11context, m_window, "WM_WINDOW_ROLE", XStringStyle, setup.session_role, DELETE_EMPTY);   // ICCCM
  m_state.setup.session_role = setup.session_role;
  m_state.setup.bg_average = setup.bg_average;
  Color c1 = setup.bg_average, c2 = setup.bg_average;
  c1.tint (0, 0.96, 0.96);
  c2.tint (0, 1.03, 1.03);
  Pixmap xpixmap = create_checkerboard_pixmap (x11context.display, x11context.visual, m_window, x11context.depth, 8, c1, c2);
  XSetWindowBackgroundPixmap (x11context.display, m_window, xpixmap); // m_state.setup.bg_average ?
  XFreePixmap (x11context.display, xpixmap);
}

void
ScreenWindowX11::configure (const Config &config)
{
  ScopedLock<Mutex> x11locker (x11_rmutex);
  if (config.title != m_state.config.title)
    {
      set_text_property (x11context, m_window, "WM_NAME", XStdICCTextStyle, config.title);                      // ICCCM
      set_text_property (x11context, m_window, "_NET_WM_NAME", XUTF8StringStyle, config.title);                 // EWMH
      m_state.config.title = config.title;
    }
  enqueue_locked (create_event_win_size (m_event_context, m_state.width, m_state.height, m_pending_configures > 0));
}

bool
ScreenWindowX11::viewable (void) { /*FIXME*/ return false; }

void
ScreenWindowX11::beep() { /*FIXME*/ }

void
ScreenWindowX11::present () { /*FIXME*/ }

void
ScreenWindowX11::start_user_move (uint button, double root_x, double root_y) { /*FIXME*/ }

void
ScreenWindowX11::start_user_resize (uint button, double root_x, double root_y, AnchorType edge) { /*FIXME*/ }

// == X11 thread ==
class X11Thread {
  X11Context &x11context;
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
            EDEBUG ("Extra: %c=%lu w=%lu event_type=%d", ss, xevent.xany.serial, xevent.xany.window, xevent.type);
          }
      }
  }
  void
  run()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    const bool running = true;
    while (running)
      {
        process_x11();
        fd_set rfds;
        FD_ZERO (&rfds);
        FD_SET (ConnectionNumber (x11context.display), &rfds);
        int maxfd = ConnectionNumber (x11context.display);
        struct timeval tv;
        tv.tv_sec = 9999, tv.tv_usec = 0;
        x11locker.unlock();
        // XDEBUG ("Xpoll: nfds=%d", maxfd + 1);
        int retval = select (maxfd + 1, &rfds, NULL, NULL, &tv);
        x11locker.lock();
        if (retval < 0)
          XDEBUG ("select(%d): %s", ConnectionNumber (x11context.display), strerror (errno));
      }
  }
  std::thread m_thread_handle;
public:
  X11Thread (X11Context &ix11context) : x11context (ix11context) {}
  void
  start()
  {
    ScopedLock<Mutex> x11locker (x11_rmutex);
    assert_return (std::thread::id() == m_thread_handle.get_id());
    m_thread_handle = std::thread (&X11Thread::run, this);
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
  ScopedLock<Mutex> x11locker (x11_rmutex);
  FIXME ("stop and join X11Thread");
  assert_unreached();
}

} // Rapicorn
