// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
#include <errno.h>

namespace { // Anon
using namespace Rapicorn;

struct LoopTester {
  static bool
  loops_pending()
  {
    return EventLoop::iterate_loops (false, false);
  }
  static void
  loops_dispatch (bool may_block)
  {
    EventLoop::iterate_loops (may_block, true);
  }
};

// === test_loop_basics ===
static bool test_callback_touched = false;
static void
test_callback()
{
  test_callback_touched = true;
}

static bool
keep_alive_callback()
{
  return true;
}

static bool
pipe_writer (PollFD &pfd)
{
  uint8 buffer[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
  int err;
  do
    err = write (pfd.fd, buffer, sizeof (buffer));
  while (err < 0 && (errno == EINTR || errno == EAGAIN));
  RAPICORN_ASSERT (err == sizeof (buffer));
  return true;
}

static int pipe_reader_seen = 0;

static bool
pipe_reader (PollFD &pfd)
{
  int counter = 1;
  while (counter <= 17)
    {
      uint8 data;
      int err;
      do
        err = read (pfd.fd, &data, 1);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
      RAPICORN_ASSERT (err == 1);
      RAPICORN_ASSERT (counter == data);
      counter++;
    }
  pipe_reader_seen++;
  if (pipe_reader_seen % 997 == 0)
    TOK();
  return true;
}

static void
test_loop_basics()
{
  const uint max_runs = 1999;
  /* basal loop tests */
  EventLoop *loop = EventLoop::create();
  TASSERT (loop);
  ref_sink (loop);
  /* oneshot test */
  TASSERT (test_callback_touched == false);
  uint tcid = loop->exec_now (slot (test_callback));
  TASSERT (tcid > 0);
  TASSERT (test_callback_touched == false);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
  TASSERT (test_callback_touched == true);
  bool tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  test_callback_touched = false;
  /* keep-alive test */
  tcid = loop->exec_now (slot (keep_alive_callback));
  for (uint counter = 0; counter < max_runs; counter++)
    if (LoopTester::loops_pending())
      LoopTester::loops_dispatch (false);
    else
      break;
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == true);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
  tremove = loop->try_remove (tcid);
  TASSERT (tremove == false);
  /* loop + pipe */
  int pipe_fds[2];
  int err = pipe (pipe_fds);
  TASSERT (err == 0);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
  loop->exec_io_handler (slot (pipe_reader), pipe_fds[0], "r");
  loop->exec_io_handler (slot (pipe_writer), pipe_fds[1], "w");
  TASSERT (pipe_reader_seen == 0);
  while (pipe_reader_seen < 4999)
    if (LoopTester::loops_pending())
      LoopTester::loops_dispatch (false);
    else
      LoopTester::loops_dispatch (true);
  TCMPSIGNED (pipe_reader_seen, ==, 4999);
  err = close (pipe_fds[1]);
  TASSERT (err == 0);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
  err = close (pipe_fds[0]);
  TASSERT (err == -1); /* should have been auto-closed by PollFDSource */
  unref (loop);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
}
REGISTER_UITHREAD_TEST ("Primitives/Test Loop Basics", test_loop_basics);

// === test_event_loop_sources ===
static uint         check_source_counter = 0;
static uint         check_source_destroyed_counter = 0;

inline uint32
quick_rand32 (void)
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

class CheckSource : public virtual EventLoop::Source {
  enum {
    INITIALIZED = 1,
    PREPARED,
    CHECKED,
    DISPATCHED,
    DESTROYED,
    FINALIZED,
    DESTRUCTED
  };
  uint          m_state;
public:
  CheckSource () :
    m_state (0)
  {
    RAPICORN_ASSERT (m_state == 0);
    m_state = INITIALIZED;
    check_source_counter++;
  }
  virtual bool
  prepare (uint64 current_time_usecs,
           int64 *timeout_usecs_p)
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = PREPARED;
    if (quick_rand32() & 0xfeedf00d)
      *timeout_usecs_p = quick_rand32() % 5000;
    return quick_rand32() & 0x00400200a;
  }
  virtual bool
  check (uint64 current_time_usecs)
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED);
    m_state = CHECKED;
    return quick_rand32() & 0xc0ffee;
  }
  virtual bool
  dispatch ()
  {
    RAPICORN_ASSERT (m_state == PREPARED ||
                   m_state == CHECKED);
    m_state = DISPATCHED;
    return (quick_rand32() % 131) != 0;
  }
  virtual void
  destroy ()
  {
    RAPICORN_ASSERT (m_state == INITIALIZED ||
                   m_state == PREPARED ||
                   m_state == CHECKED ||
                   m_state == DISPATCHED);
    m_state = DESTROYED;
    check_source_destroyed_counter++;
  }
  virtual void
  finalize ()
  {
    RAPICORN_ASSERT (m_state == DESTROYED);
    // RAPICORN_ASSERT (m_state == INITIALIZED || m_state == DESTROYED);
    EventLoop::Source::finalize();
    m_state = FINALIZED;
  }
  virtual
  ~CheckSource ()
  {
    RAPICORN_ASSERT (m_state == FINALIZED);
    m_state = DESTRUCTED;
    check_source_counter--;
  }
};

static CheckSource *check_sources[997] = { NULL, };

static void
test_event_loop_sources()
{
  const uint max_runs = 999;
  EventLoop *loop = EventLoop::create();
  TASSERT (loop);
  ref_sink (loop);
  while (LoopTester::loops_pending())
    LoopTester::loops_dispatch (false);
  /* source state checks */
  TASSERT (check_source_counter == 0);
  const uint nsrc = quick_rand32() % (1 + ARRAY_SIZE (check_sources));
  for (uint i = 0; i < nsrc; i++)
    {
      check_sources[i] = new CheckSource();
      ref (check_sources[i]);
      loop->add_source (check_sources[i], quick_rand32());
    }
  TASSERT (check_source_counter == nsrc);
  TASSERT (check_source_destroyed_counter == 0);
  for (uint counter = 0; counter < max_runs; counter++)
    {
      if (LoopTester::loops_pending())
        LoopTester::loops_dispatch (false);
      else
        break;
      if (counter % 347 == 0)
        TOK();
    }
  TASSERT (check_source_counter == nsrc);
  loop->kill_sources();
  TCMP (check_source_destroyed_counter, ==, nsrc); /* checks execution of enough destroy() handlers */
  TASSERT (check_source_counter == nsrc);
  for (uint i = 0; i < nsrc; i++)
    unref (check_sources[i]);
  TASSERT (check_source_counter == 0);
  unref (loop);
}
REGISTER_UITHREAD_TEST ("Primitives/Test Event Loop Sources", test_event_loop_sources);

static void
test_affine()
{
  AffineIdentity id;
  Affine a;
  TASSERT (a == id);
  TASSERT (a.determinant() == 1);
  TASSERT (a.is_identity());
  AffineTranslate at (1, 1);
  TASSERT (at.point (3, 5) == Point (4, 6));
  TASSERT (at.ipoint (4, 6) == Point (3, 5));
}
REGISTER_UITHREAD_TEST ("Primitives/Test Affine", test_affine);

static void
test_double_int()
{
  TINFO ("Testing dtoi32...");
  TASSERT (_dtoi32_generic (0.0) == 0);
  TASSERT (_dtoi32_generic (+0.3) == +0);
  TASSERT (_dtoi32_generic (-0.3) == -0);
  TASSERT (_dtoi32_generic (+0.7) == +1);
  TASSERT (_dtoi32_generic (-0.7) == -1);
  TASSERT (_dtoi32_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi32_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi32_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi32_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi32 (0.0) == 0);
  TASSERT (dtoi32 (+0.3) == +0);
  TASSERT (dtoi32 (-0.3) == -0);
  TASSERT (dtoi32 (+0.7) == +1);
  TASSERT (dtoi32 (-0.7) == -1);
  TASSERT (dtoi32 (+2147483646.3) == +2147483646);
  TASSERT (dtoi32 (+2147483646.7) == +2147483647);
  TASSERT (dtoi32 (-2147483646.3) == -2147483646);
  TASSERT (dtoi32 (-2147483646.7) == -2147483647);
  TASSERT (dtoi32 (-2147483647.3) == -2147483647);
  TASSERT (dtoi32 (-2147483647.7) == -2147483648LL);
  TINFO ("Testing dtoi64...");
  TASSERT (_dtoi64_generic (0.0) == 0);
  TASSERT (_dtoi64_generic (+0.3) == +0);
  TASSERT (_dtoi64_generic (-0.3) == -0);
  TASSERT (_dtoi64_generic (+0.7) == +1);
  TASSERT (_dtoi64_generic (-0.7) == -1);
  TASSERT (_dtoi64_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi64_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi64_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi64_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.7) == -2147483648LL);
  TASSERT (_dtoi64_generic (+4294967297.3) == +4294967297LL);
  TASSERT (_dtoi64_generic (+4294967297.7) == +4294967298LL);
  TASSERT (_dtoi64_generic (-4294967297.3) == -4294967297LL);
  TASSERT (_dtoi64_generic (-4294967297.7) == -4294967298LL);
  TASSERT (_dtoi64_generic (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (_dtoi64_generic (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (_dtoi64_generic (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (_dtoi64_generic (-1125899906842624.7) == -1125899906842625LL);
  TASSERT (dtoi64 (0.0) == 0);
  TASSERT (dtoi64 (+0.3) == +0);
  TASSERT (dtoi64 (-0.3) == -0);
  TASSERT (dtoi64 (+0.7) == +1);
  TASSERT (dtoi64 (-0.7) == -1);
  TASSERT (dtoi64 (+2147483646.3) == +2147483646);
  TASSERT (dtoi64 (+2147483646.7) == +2147483647);
  TASSERT (dtoi64 (-2147483646.3) == -2147483646);
  TASSERT (dtoi64 (-2147483646.7) == -2147483647);
  TASSERT (dtoi64 (-2147483647.3) == -2147483647);
  TASSERT (dtoi64 (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi64 (+4294967297.3) == +4294967297LL);
  TASSERT (dtoi64 (+4294967297.7) == +4294967298LL);
  TASSERT (dtoi64 (-4294967297.3) == -4294967297LL);
  TASSERT (dtoi64 (-4294967297.7) == -4294967298LL);
  TASSERT (dtoi64 (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (dtoi64 (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (dtoi64 (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (dtoi64 (-1125899906842624.7) == -1125899906842625LL);
  TINFO ("Testing iround...");
  TASSERT (round (0.0) == 0.0);
  TASSERT (round (+0.3) == +0.0);
  TASSERT (round (-0.3) == -0.0);
  TASSERT (round (+0.7) == +1.0);
  TASSERT (round (-0.7) == -1.0);
  TASSERT (round (+4294967297.3) == +4294967297.0);
  TASSERT (round (+4294967297.7) == +4294967298.0);
  TASSERT (round (-4294967297.3) == -4294967297.0);
  TASSERT (round (-4294967297.7) == -4294967298.0);
  TASSERT (iround (0.0) == 0);
  TASSERT (iround (+0.3) == +0);
  TASSERT (iround (-0.3) == -0);
  TASSERT (iround (+0.7) == +1);
  TASSERT (iround (-0.7) == -1);
  TASSERT (iround (+4294967297.3) == +4294967297LL);
  TASSERT (iround (+4294967297.7) == +4294967298LL);
  TASSERT (iround (-4294967297.3) == -4294967297LL);
  TASSERT (iround (-4294967297.7) == -4294967298LL);
  TASSERT (iround (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (iround (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iround (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iround (-1125899906842624.7) == -1125899906842625LL);
  TINFO ("Testing iceil...");
  TASSERT (ceil (0.0) == 0.0);
  TASSERT (ceil (+0.3) == +1.0);
  TASSERT (ceil (-0.3) == -0.0);
  TASSERT (ceil (+0.7) == +1.0);
  TASSERT (ceil (-0.7) == -0.0);
  TASSERT (ceil (+4294967297.3) == +4294967298.0);
  TASSERT (ceil (+4294967297.7) == +4294967298.0);
  TASSERT (ceil (-4294967297.3) == -4294967297.0);
  TASSERT (ceil (-4294967297.7) == -4294967297.0);
  TASSERT (iceil (0.0) == 0);
  TASSERT (iceil (+0.3) == +1);
  TASSERT (iceil (-0.3) == -0);
  TASSERT (iceil (+0.7) == +1);
  TASSERT (iceil (-0.7) == -0);
  TASSERT (iceil (+4294967297.3) == +4294967298LL);
  TASSERT (iceil (+4294967297.7) == +4294967298LL);
  TASSERT (iceil (-4294967297.3) == -4294967297LL);
  TASSERT (iceil (-4294967297.7) == -4294967297LL);
  TASSERT (iceil (+1125899906842624.3) == +1125899906842625LL);
  TASSERT (iceil (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iceil (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iceil (-1125899906842624.7) == -1125899906842624LL);
  TINFO ("Testing ifloor...");
  TASSERT (floor (0.0) == 0.0);
  TASSERT (floor (+0.3) == +0.0);
  TASSERT (floor (-0.3) == -1.0);
  TASSERT (floor (+0.7) == +0.0);
  TASSERT (floor (-0.7) == -1.0);
  TASSERT (floor (+4294967297.3) == +4294967297.0);
  TASSERT (floor (+4294967297.7) == +4294967297.0);
  TASSERT (floor (-4294967297.3) == -4294967298.0);
  TASSERT (floor (-4294967297.7) == -4294967298.0);
  TASSERT (ifloor (0.0) == 0);
  TASSERT (ifloor (+0.3) == +0);
  TASSERT (ifloor (-0.3) == -1);
  TASSERT (ifloor (+0.7) == +0);
  TASSERT (ifloor (-0.7) == -1);
  TASSERT (ifloor (+4294967297.3) == +4294967297LL);
  TASSERT (ifloor (+4294967297.7) == +4294967297LL);
  TASSERT (ifloor (-4294967297.3) == -4294967298LL);
  TASSERT (ifloor (-4294967297.7) == -4294967298LL);
  TASSERT (ifloor (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (ifloor (+1125899906842624.7) == +1125899906842624LL);
  TASSERT (ifloor (-1125899906842624.3) == -1125899906842625LL);
  TASSERT (ifloor (-1125899906842624.7) == -1125899906842625LL);
}
REGISTER_UITHREAD_TEST ("Primitives/Test Double <=> Int Conversion", test_double_int);

/* --- hsv --- */
static void
test_hsv_rgb()
{
  /* assert that set_hsv(get_hsv(v))==v for all v */
  const int step = Test::slow() ? 1 : 10;
  for (uint r = 0; r < 256; r += step)
    {
      for (uint g = 0; g < 256; g += step)
        for (uint b = 0; b < 256; b += step)
          {
            Color c (r, g, b, 0xff);
            double hue, saturation, value;
            c.get_hsv (&hue, &saturation, &value);
            if (r > g && r > b)
              RAPICORN_ASSERT (hue < 60 || hue > 300);
            else if (g > r && g > b)
              RAPICORN_ASSERT (hue > 60 || hue < 180);
            else if (b > g && b > r)
              RAPICORN_ASSERT (hue > 180 || hue < 300);
            Color d (0xff75c3a9);
            d.set_hsv (hue, saturation, value);
            if (c.red()   != d.red() ||
                c.green() != d.green() ||
                c.blue()  != d.blue() ||
                c.alpha() != d.alpha())
              fatal ("color difference after hsv: %s != %s (hue=%f saturation=%f value=%f)\n",
                     c.string().c_str(), d.string().c_str(), hue, saturation, value);
          }
      if (r % 5 == 0)
        TOK();
    }
}
REGISTER_UITHREAD_TEST ("Primitives/Test Hsv <=> Rgb Conversion", test_hsv_rgb);
REGISTER_UITHREAD_SLOWTEST ("Primitives/Test Hsv <=> Rgb Conversion", test_hsv_rgb);

} // anon
