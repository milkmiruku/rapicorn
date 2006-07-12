/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "adjustment.hh"
#include "factory.hh"

namespace Rapicorn {

Adjustment::Adjustment() :
  sig_value_changed (*this, &Adjustment::value_changed),
  sig_range_changed (*this, &Adjustment::range_changed)
{}

Adjustment::~Adjustment()
{}

double
Adjustment::flipped_value()
{
  double v = value() - lower();
  return upper() - page() - v;
}

void
Adjustment::flipped_value (double newval)
{
  double l = lower();
  double u = upper() - page();
  newval = CLAMP (newval, l, u);
  double v = newval - l;
  value (u - v);
}

double
Adjustment::nvalue ()
{
  double l = lower(), u = upper(), ar = u - l;
  return ar > 0 ? (value() - l) / ar : 0.0;
}

void
Adjustment::nvalue (double newval)
{
  double l = lower(), u = upper(), ar = u - l;
  value (l + newval * ar);
}

double
Adjustment::flipped_nvalue ()
{
  return 1.0 - nvalue();
}

void
Adjustment::flipped_nvalue (double newval)
{
  nvalue (1.0 - newval);
}

void
Adjustment::value_changed()
{}

void
Adjustment::range_changed()
{}

double
Adjustment::abs_range ()
{
  return fabs (upper() - lower());
}

String
Adjustment::string ()
{
  String s = string_printf ("Adjustment(%g,[%f,%f],istep=%+f,pstep=%+f,page=%f%s)",
                            value(), lower(), upper(),
                            step_increment(), page_increment(), page(),
                            frozen() ? ",frozen" : "");
  return s;
}

class AdjustmentSimpleImpl : public virtual Adjustment {
  double m_value, m_lower, m_upper, m_step_increment, m_page_increment, m_page;
  uint   m_freeze_count;
public:
  AdjustmentSimpleImpl() :
    m_value (0), m_lower (0), m_upper (1),
    m_step_increment (1.0 / 100), m_page_increment (1.0 / 10), m_page (0),
    m_freeze_count (0)
  {}
  /* value */
  virtual double        value	        ()                      { return m_value; }
  virtual void
  value (double newval)
  {
    double old_value = m_value;
    m_value = CLAMP (newval, m_lower, m_upper);
    if (old_value != m_value && !m_freeze_count)
      sig_value_changed.emit ();
  }
  /* range */
  virtual bool                  frozen          () const                { return m_freeze_count > 0; }
  virtual void                  freeze          ()                      { m_freeze_count++; }
  virtual double                lower	        () const                { return m_lower; }
  virtual void                  lower           (double newval)         { return_if_fail (m_freeze_count); m_lower = newval; }
  virtual double                upper	        () const                { return m_upper; }
  virtual void		        upper	        (double newval)         { return_if_fail (m_freeze_count); m_upper = newval; }
  virtual double	        step_increment	() const                { return m_step_increment; }
  virtual void		        step_increment	(double newval)         { return_if_fail (m_freeze_count); m_step_increment = newval; }
  virtual double	        page_increment	() const                { return m_page_increment; }
  virtual void		        page_increment	(double newval)         { return_if_fail (m_freeze_count); m_page_increment = newval; }
  virtual double	        page	        () const                { return m_page; }
  virtual void		        page	        (double newval)         { return_if_fail (m_freeze_count); m_page = newval; }
  virtual void
  thaw ()
  {
    return_if_fail (m_freeze_count);
    m_freeze_count--;
    if (!m_freeze_count)
      {
        double old_value = m_value;
        double old_lower = m_lower;
        double old_upper = m_upper;
        double old_page  = m_page;
        if (m_lower > m_upper)
          m_lower = m_upper = (m_lower + m_upper) / 2;
        m_page = CLAMP (m_page, 0, m_upper - m_lower);
        m_value = CLAMP (m_value, m_lower, m_upper);
        if (old_lower != m_lower || old_upper != m_upper || old_page != m_page) //FIXME: check change since freeze()
          sig_range_changed.emit();
        if (old_value != m_value)
          sig_value_changed.emit ();
      }
  }
};

Adjustment*
Adjustment::create (double  value,
                    double  lower,
                    double  upper,
                    double  step_increment,
                    double  page_increment,
                    double  page_size)
{
  AdjustmentSimpleImpl *adj = new AdjustmentSimpleImpl();
  adj->freeze();
  adj->lower (lower);
  adj->upper (upper);
  adj->step_increment (step_increment);
  adj->page_increment (page_increment);
  adj->page (page_size);
  adj->thaw();
  return adj;
}

} // Rapicorn
