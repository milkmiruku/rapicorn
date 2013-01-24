// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "testitems.hh"
#include "container.hh"
#include "painter.hh"
#include "window.hh"
#include "factory.hh"
#include "selector.hh"
#include <errno.h>

namespace Rapicorn {

TestContainer::TestContainer() :
  sig_assertion_ok (*this),
  sig_assertions_passed (*this)
{}

#define DFLTEPS (1e-8)

const PropertyList&
TestContainer::_property_list()
{
  /* not using _() here, because TestContainer is just a developer tool */
  static Property *properties[] = {
    MakeProperty (TestContainer, value,         "Value",         "Store string value for assertion",           "rw"),
    MakeProperty (TestContainer, assert_value,  "Assert Value",  "Assert a particular string value",           "rw"),
    MakeProperty (TestContainer, assert_left,   "Assert-Left",   "Assert positioning of the left item edge",   -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_right,  "Assert-Right",  "Assert positioning of the right item edge",  -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_bottom, "Assert-Bottom", "Assert positioning of the bottom item edge", -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_top,    "Assert-Top",    "Assert positioning of the top item edge",    -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_width,  "Assert-Width",  "Assert amount of the item width",            -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, assert_height, "Assert-Height", "Assert amount of the item height",           -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestContainer, epsilon,       "Epsilon",       "Epsilon within which assertions must hold",  0,         +DBL_MAX, 0.01, "rw"),
    MakeProperty (TestContainer, paint_allocation, "Paint Allocation", "Fill allocation rectangle with a solid color",  "rw"),
    MakeProperty (TestContainer, fatal_asserts, "Fatal-Asserts", "Handle assertion failures as fatal errors",  "rw"),
    MakeProperty (TestContainer, accu,          "Accumulator",   "Store string value and keep history",        "rw"),
    MakeProperty (TestContainer, accu_history,  "Accu-History",  "Concatenated accumulator history",           "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

static uint test_containers_rendered = 0;

uint
TestContainer::seen_test_items ()
{
  return test_containers_rendered;
}

static const Selector::CustomPseudoRegistry _test_pass ("test-pass", "The TestContainer:test-pass(arg) pseudo selector matches if 'arg' is a true boolean value.");

class TestContainerImpl : public virtual SingleContainerImpl, public virtual TestContainer {
  String m_value, m_assert_value;
  String m_accu, m_accu_history;
  double m_assert_left, m_assert_right;
  double m_assert_top, m_assert_bottom;
  double m_assert_width, m_assert_height;
  double m_epsilon;
  bool   m_test_container_counted;
  bool   m_fatal_asserts;
  bool   m_paint_allocation;
public:
  TestContainerImpl() :
    m_value (""), m_assert_value (""),
    m_accu (""), m_accu_history (""),
    m_assert_left (-INFINITY), m_assert_right (-INFINITY),
    m_assert_top (-INFINITY), m_assert_bottom (-INFINITY),
    m_assert_width (-INFINITY), m_assert_height (-INFINITY),
    m_epsilon (DFLTEPS), m_test_container_counted (false),
    m_fatal_asserts (false), m_paint_allocation (false)
  {}
  virtual String value           () const            { return m_value; }
  virtual void   value           (const String &val) { m_value = val; }
  virtual String assert_value    () const            { return m_assert_value; }
  virtual void   assert_value    (const String &val) { m_assert_value = val; }
  virtual double assert_left     () const        { return m_assert_left  ; }
  virtual void   assert_left     (double val)    { m_assert_left = val; invalidate(); }
  virtual double assert_right    () const        { return m_assert_right ; }
  virtual void   assert_right    (double val)    { m_assert_right = val; invalidate(); }
  virtual double assert_top      () const        { return m_assert_top   ; }
  virtual void   assert_top      (double val)    { m_assert_top = val; invalidate(); }
  virtual double assert_bottom   () const        { return m_assert_bottom; }
  virtual void   assert_bottom   (double val)    { m_assert_bottom = val; invalidate(); }
  virtual double assert_width    () const        { return m_assert_width ; }
  virtual void   assert_width    (double val)    { m_assert_width = val; invalidate(); }
  virtual double assert_height   () const        { return m_assert_height; }
  virtual void   assert_height   (double val)    { m_assert_height = val; invalidate(); }
  virtual double epsilon         () const        { return m_epsilon  ; }
  virtual void   epsilon         (double val)    { m_epsilon = val; invalidate(); }
  virtual bool   paint_allocation() const        { return m_paint_allocation; }
  virtual void   paint_allocation(bool   val)    { m_paint_allocation = val; invalidate(); }
  virtual bool   fatal_asserts   () const        { return m_fatal_asserts; }
  virtual void   fatal_asserts   (bool   val)    { m_fatal_asserts = val; invalidate(); }
  virtual String accu            () const            { return m_accu; }
  virtual void   accu            (const String &val) { m_accu = val; m_accu_history += val; }
  virtual String accu_history    () const            { return m_accu_history; }
  virtual void   accu_history    (const String &val) { m_accu_history = val; }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    SingleContainerImpl::size_request (requisition);
  }
  void
  assert_value (const char *assertion_name,
                double      value,
                double      first,
                double      second)
  {
    if (!isnormal (value))
      return;
    double rvalue;
    if (value >= 0)
      rvalue = first;
    else /* value < 0 */
      {
        rvalue = second;
        value = fabs (value);
      }
    double delta = fabs (rvalue - value);
    if (delta > m_epsilon)
      {
        if (m_fatal_asserts)
          fatal ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                 assertion_name, value, rvalue, delta, m_epsilon);
        else
          critical ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                    assertion_name, value, rvalue, delta, m_epsilon);
      }
    else
      {
        String msg = string_printf ("%s == %f", assertion_name, value);
        sig_assertion_ok.emit (msg);
      }
  }
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    if (m_paint_allocation)
      {
        IRect ia = allocation();
        cairo_t *cr = cairo_context (rcontext, rect);
        CPainter painter (cr);
        painter.draw_filled_rect (ia.x, ia.y, ia.width, ia.height, heritage()->black());
      }

    Allocation rarea = get_window()->allocation();
    double width = allocation().width, height = allocation().height;
    double x1 = allocation().x, x2 = rarea.width - x1 - width;
    double y1 = allocation().y, y2 = rarea.height - y1 - height;
    assert_value ("assert-bottom", m_assert_bottom, y1, y2);
    assert_value ("assert-right",  m_assert_right,  x1, x2);
    assert_value ("assert-top",    m_assert_top,    y1, y2);
    assert_value ("assert-left",   m_assert_left,   x1, x2);
    assert_value ("assert-width",  m_assert_width, width, width);
    assert_value ("assert-height", m_assert_height, height, height);
    if (m_assert_value != m_value)
      {
        if (m_fatal_asserts)
          fatal ("value has unexpected contents: %s (expected: %s)", m_value.c_str(), m_assert_value.c_str());
        else
          critical ("value has unexpected contents: %s (expected: %s)", m_value.c_str(), m_assert_value.c_str());
      }
    sig_assertions_passed.emit ();
    /* count containers for seen_test_containers() */
    if (!m_test_container_counted)
      {
        m_test_container_counted = true;
        test_containers_rendered++;
      }
  }
  virtual bool
  pseudo_selector (const String &ident, const String &arg, String &error)
  {
    return ident == _test_pass.ident() ? string_to_bool (arg) : false;
  }
};
static const ItemFactory<TestContainerImpl> test_container_factory ("Rapicorn::Factory::TestContainer");

const PropertyList&
TestBox::_property_list()
{
  static Property *properties[] = {
    MakeProperty (TestBox, snapshot_file, _("Snapshot File Name"), _("PNG image file name to write snapshot to"), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

class TestBoxImpl : public virtual SingleContainerImpl, public virtual TestBox {
  String m_snapshot_file;
  uint   m_handler_id;
protected:
  virtual String snapshot_file () const                 { return m_snapshot_file; }
  virtual void   snapshot_file (const String &val)      { m_snapshot_file = val; invalidate(); }
  ~TestBoxImpl()
  {
    if (m_handler_id)
      {
        remove_exec (m_handler_id);
        m_handler_id = 0;
      }
  }
  void
  make_snapshot ()
  {
    WindowImpl *witem = get_window();
    if (m_snapshot_file != "" && witem)
      {
        cairo_surface_t *isurface = witem->create_snapshot (allocation());
        cairo_status_t wstatus = cairo_surface_write_to_png (isurface, m_snapshot_file.c_str());
        cairo_surface_destroy (isurface);
        String err = CAIRO_STATUS_SUCCESS == wstatus ? "ok" : cairo_status_to_string (wstatus);
        printerr ("%s: wrote %s: %s\n", name().c_str(), m_snapshot_file.c_str(), err.c_str());
      }
    if (m_handler_id)
      {
        remove_exec (m_handler_id);
        m_handler_id = 0;
      }
  }
public:
  explicit TestBoxImpl() :
    m_handler_id (0)
  {}
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    if (!m_handler_id)
      {
        WindowImpl *witem = get_window();
        if (witem)
          {
            EventLoop *loop = witem->get_loop();
            if (loop)
              m_handler_id = loop->exec_now (Aida::slot (*this, &TestBoxImpl::make_snapshot));
          }
      }
  }
};
static const ItemFactory<TestBoxImpl> test_box_factory ("Rapicorn::Factory::TestBox");

class IdlTestItemImpl : public virtual ItemImpl, public virtual IdlTestItemIface {
  bool m_bool; int m_int; double m_float; std::string m_string; TestEnum m_enum;
  Requisition m_rec; StringSeq m_seq; IdlTestItemIface *m_self;
  virtual bool          bool_prop () const                      { return m_bool; }
  virtual void          bool_prop (bool b)                      { m_bool = b; }
  virtual int           int_prop () const                       { return m_int; }
  virtual void          int_prop (int i)                        { m_int = i; }
  virtual double        float_prop () const                     { return m_float; }
  virtual void          float_prop (double f)                   { m_float = f; }
  virtual std::string   string_prop () const                    { return m_string; }
  virtual void          string_prop (const std::string &s)      { m_string = s; }
  virtual TestEnum      enum_prop () const                      { return m_enum; }
  virtual void          enum_prop (TestEnum v)                  { m_enum = v; }
  virtual Requisition   record_prop () const                    { return m_rec; }
  virtual void          record_prop (const Requisition &r)      { m_rec = r; }
  virtual StringSeq     sequence_prop () const                  { return m_seq; }
  virtual void          sequence_prop (const StringSeq &s)      { m_seq = s; }
  virtual IdlTestItemIface* self_prop () const                  { return m_self; }
  virtual void          self_prop (IdlTestItemIface *s)         { m_self = s; }
  virtual void          size_request (Requisition &req)         { req = Requisition (12, 12); }
  virtual void          size_allocate (Allocation area, bool changed) {}
  virtual void          render (RenderContext &rcontext, const Rect &rect) {}
  virtual const PropertyList& _property_list () { return RAPICORN_AIDA_PROPERTY_CHAIN (ItemImpl::_property_list(), IdlTestItemIface::_property_list()); }
};
static const ItemFactory<IdlTestItemImpl> test_item_factory ("Rapicorn::Factory::IdlTestItem");

} // Rapicorn
