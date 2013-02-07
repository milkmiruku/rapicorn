// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "adjustment.hh"
#include "factory.hh"

namespace Rapicorn {

Adjustment::Adjustment() :
  sig_value_changed (Aida::slot (*this, &Adjustment::value_changed)),
  sig_range_changed (Aida::slot (*this, &Adjustment::range_changed))
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
  double l = lower(), u = upper() - page(), ar = u - l;
  return ar > 0 ? (value() - l) / ar : 0.0;
}

void
Adjustment::nvalue (double newval)
{
  double l = lower(), u = upper() - page(), ar = u - l;
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
  return fabs (upper() - page() - lower());
}

double
Adjustment::abs_length ()
{
  double p = page();
  double ar = fabs (upper() - lower());
  return ar > 0 ? p / ar : 0.0;
}

bool
Adjustment::move_flipped (MoveType movet)
{
  switch (movet)
    {
    case MOVE_PAGE_FORWARD:     return move (MOVE_PAGE_BACKWARD);
    case MOVE_STEP_FORWARD:     return move (MOVE_STEP_BACKWARD);
    case MOVE_STEP_BACKWARD:    return move (MOVE_STEP_FORWARD);
    case MOVE_PAGE_BACKWARD:    return move (MOVE_PAGE_FORWARD);
    case MOVE_NONE:             ;
    }
  return false;
}

bool
Adjustment::move (MoveType move)
{
  switch (move)
    {
    case MOVE_PAGE_FORWARD:
      value (value() + page_increment());
      return true;
    case MOVE_STEP_FORWARD:
      value (value() + step_increment());
      return true;
    case MOVE_STEP_BACKWARD:
      value (value() - step_increment());
      return true;
    case MOVE_PAGE_BACKWARD:
      value (value() - page_increment());
      return true;
    case MOVE_NONE: ;
    }
  return false;
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

struct AdjustmentMemorizedState {
  double value, lower, upper, step_increment, page_increment, page;
};
static DataKey<AdjustmentMemorizedState> memorized_state_key;

class AdjustmentSimpleImpl : public virtual Adjustment, public virtual DataListContainer {
  double m_value, m_lower, m_upper, m_step_increment, m_page_increment, m_page;
  uint   m_freeze_count;
public:
  AdjustmentSimpleImpl() :
    m_value (0), m_lower (0), m_upper (100),
    m_step_increment (1), m_page_increment (10), m_page (0),
    m_freeze_count (0)
  {}
  /* value */
  virtual double        value	        ()                      { return m_value; }
  virtual void
  value (double newval)
  {
    double old_value = m_value;
    if (isnan (newval))
      {
        critical ("Adjustment::value(): invalid value: %g", newval);
        newval = 0;
      }
    m_value = CLAMP (newval, m_lower, m_upper - m_page);
    if (old_value != m_value && !m_freeze_count)
      sig_value_changed.emit ();
  }
  /* range */
  virtual bool                  frozen          () const                { return m_freeze_count > 0; }
  virtual double                lower	        () const                { return m_lower; }
  virtual void                  lower           (double newval)         { assert_return (m_freeze_count); m_lower = newval; }
  virtual double                upper	        () const                { return m_upper; }
  virtual void		        upper	        (double newval)         { assert_return (m_freeze_count); m_upper = newval; }
  virtual double	        step_increment	() const                { return m_step_increment; }
  virtual void		        step_increment	(double newval)         { assert_return (m_freeze_count); m_step_increment = newval; }
  virtual double	        page_increment	() const                { return m_page_increment; }
  virtual void		        page_increment	(double newval)         { assert_return (m_freeze_count); m_page_increment = newval; }
  virtual double	        page	        () const                { return m_page; }
  virtual void		        page	        (double newval)         { assert_return (m_freeze_count); m_page = newval; }
  virtual void
  freeze ()
  {
    if (!m_freeze_count)
      {
        AdjustmentMemorizedState m;
        m.value = m_value;
        m.lower = m_lower;
        m.upper = m_upper;
        m.step_increment = m_step_increment;
        m.page_increment = m_page_increment;
        m.page = m_page;
        set_data (&memorized_state_key, m);
      }
    m_freeze_count++;
  }
  virtual void
  constrain ()
  {
    assert_return (m_freeze_count);
    if (m_lower > m_upper)
      m_lower = m_upper = (m_lower + m_upper) / 2;
    m_page = CLAMP (m_page, 0, m_upper - m_lower);
    m_page_increment = MAX (m_page_increment, 0);
    if (m_page > 0)
      m_page_increment = MIN (m_page_increment, m_page);
    m_step_increment = MAX (0, m_step_increment);
    if (m_page_increment > 0)
      m_step_increment = MIN (m_step_increment, m_page_increment);
    else if (m_page > 0)
      m_step_increment = MIN (m_step_increment, m_page);
    m_value = CLAMP (m_value, m_lower, m_upper - m_page);
  }
  virtual void
  thaw ()
  {
    assert_return (m_freeze_count);
    if (m_freeze_count == 1)
      constrain();
    m_freeze_count--;
    if (!m_freeze_count)
      {
        AdjustmentMemorizedState m = swap_data (&memorized_state_key);
        if (m.lower != m_lower || m.upper != m_upper || m.page != m_page ||
            m.step_increment != m_step_increment || m.page_increment != m_page_increment)
          sig_range_changed.emit();
        if (m.value != m_value)
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
