/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#include "testitem.hh"
#include "itemimpl.hh"
#include "painter.hh"
#include "root.hh"

namespace Rapicorn {

TestItem::TestItem() :
  sig_assertion_ok (*this),
  sig_assertions_passed (*this)
{}

#define DFLTEPS (1e-8)

const PropertyList&
TestItem::list_properties()
{
  /* not using _() here, because TestItem is just a developer tool */
  static Property *properties[] = {
    MakeProperty (TestItem, epsilon,       "Epsilon",       "Epsilon within which assertions must hold",  DFLTEPS,   0,         +DBL_MAX, 0.01, "rw"),
    MakeProperty (TestItem, assert_left,   "Assert-Left",   "Assert positioning of the left item edge",   -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, assert_right,  "Assert-Right",  "Assert positioning of the right item edge",  -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, assert_bottom, "Assert-Bottom", "Assert positioning of the bottom item edge", -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, assert_top,    "Assert-Top",    "Assert positioning of the top item edge",    -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, assert_width,  "Assert-Width",  "Assert amount of the item width",            -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, assert_height, "Assert-Height", "Assert amount of the item height",           -INFINITY, -INFINITY, +DBL_MAX, 3, "rw"),
    MakeProperty (TestItem, fatal_asserts, "Fatal-Asserts", "Handle assertion failures as fatal errors",  false, "rw"),
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

static uint test_items_rendered = 0;

uint
TestItem::seen_test_items ()
{
  assert (rapicorn_thread_entered());
  return test_items_rendered;
}

class TestItemImpl : public virtual TestItem, public virtual ItemImpl {
  double m_assert_left, m_assert_right;
  double m_assert_top, m_assert_bottom;
  double m_assert_width, m_assert_height;
  double m_epsilon;
  bool   m_test_item_counted;
  bool   m_fatal_asserts;
public:
  TestItemImpl() :
    m_assert_left (-INFINITY), m_assert_right (-INFINITY),
    m_assert_top (-INFINITY), m_assert_bottom (-INFINITY),
    m_assert_width (-INFINITY), m_assert_height (-INFINITY),
    m_epsilon (DFLTEPS), m_test_item_counted (false),
    m_fatal_asserts (false)
  {}
  virtual double epsilon         () const        { return m_epsilon  ; }
  virtual void   epsilon         (double val)    { m_epsilon = val; invalidate(); }
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
  virtual bool   fatal_asserts   () const        { return m_fatal_asserts; }
  virtual void   fatal_asserts   (bool   val)    { m_fatal_asserts = val; invalidate(); }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    requisition.width = 7;
    requisition.height = 7;
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
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
          error ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                 assertion_name, value, rvalue, delta, m_epsilon);
        else
          warning ("similarity exceeded: %s=%f real-value=%f delta=%f (epsilon=%g)",
                   assertion_name, value, rvalue, delta, m_epsilon);
      }
    else
      {
        String msg = string_printf ("%s == %f", assertion_name, value);
        sig_assertion_ok.emit (msg);
      }
  }
  virtual void
  render (Display &display)
  {
    assert (rapicorn_thread_entered());
    IRect ia = allocation();
    Plane &plane = display.create_plane();
    Painter painter (plane);
    painter.draw_filled_rect (ia.x, ia.y, ia.width, ia.height, black());
    Allocation rarea = get_root()->allocation();
    double width = allocation().width, height = allocation().height;
    double x1 = allocation().x, x2 = rarea.width - x1 - width;
    double y1 = allocation().y, y2 = rarea.height - y1 - height;
    assert_value ("assert-bottom", m_assert_bottom, y1, y2);
    assert_value ("assert-right",  m_assert_right,  x1, x2);
    assert_value ("assert-top",    m_assert_top,    y1, y2);
    assert_value ("assert-left",   m_assert_left,   x1, x2);
    assert_value ("assert-width",  m_assert_width, width, width);
    assert_value ("assert-height", m_assert_height, height, height);
    sig_assertions_passed.emit ();
    /* count items for seen_test_items() */
    if (!m_test_item_counted)
      {
        m_test_item_counted = true;
        test_items_rendered++;
      }
  }
};
static const ItemFactory<TestItemImpl> test_item_factory ("Rapicorn::Factory::TestItem");

} // Rapicorn
