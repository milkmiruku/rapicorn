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
#include <rcore/testutils.hh>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
using namespace Rapicorn;

#define throw_if_fail(expr)     do { if (RAPICORN_LIKELY (expr)) break; throw Rapicorn::AssertionError (#expr, __FILE__, __LINE__); } while (0)

static void
test_dialog_messages ()
{
  TASSERT (Msg::NONE    == Msg::lookup_type ("none"));
  TASSERT (Msg::ALWAYS  == Msg::lookup_type ("always"));
  TASSERT (Msg::ERROR   == Msg::lookup_type ("error"));
  TASSERT (Msg::WARNING == Msg::lookup_type ("warning"));
  TASSERT (Msg::SCRIPT  == Msg::lookup_type ("script"));
  TASSERT (Msg::INFO    == Msg::lookup_type ("info"));
  TASSERT (Msg::DIAG    == Msg::lookup_type ("diag"));
  TASSERT (Msg::DEBUG   == Msg::lookup_type ("debug"));
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  Msg::enable (Msg::NONE);
  Msg::disable (Msg::ALWAYS);
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::disable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == false);
  Msg::enable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::display (Msg::WARNING,
                Msg::Title ("Warning Title"),
                Msg::Text1 ("Primary warning message."),
                Msg::Text2 ("Secondary warning message."),
                Msg::Text2 ("Continuation of secondary warning message."),
                Msg::Text3 ("Message details: 1, 2, 3."),
                Msg::Text3 ("And more message details: a, b, c."),
                Msg::Check ("Show this message again."));
}
REGISTER_LOGTEST ("Message/Dialog Message Types", test_dialog_messages);

static void
bogus_func ()
{
  assert_return (0 == "assert-return-void");
}

static int
value_func (int)
{
  assert_return (0 == "assert-return+val", 0);
  return 1;
}

static void
test_logging (const char *logarg)
{
  errno = EINVAL;       // used by P* checks
  if (String ("--test-assert") == logarg)
    RAPICORN_ASSERT (0 == "test-assert");
  if (String ("--test-unreached") == logarg)
    RAPICORN_ASSERT_UNREACHED();
  if (String ("--test-fatal") == logarg)
    fatal ("execution has reached a fatal condition (\"test-fatal\")");
  if (String ("--test-TCMP") == logarg)
    TCMP (0, ==, "validate failing TCMP");
  if (String ("--test-logging") == logarg)
    {
#define FIXIT FIX##ME
      FIXIT ("nothing to fix here, FIX""ME() test");
      debug_configure ("no-fatal-warnings"); // cancel fatal-warnings, usually enforced for tests
      RAPICORN_DEBUG ("logging test selected, will trigger various warnings (debugging message)");
      RAPICORN_CRITICAL_UNLESS (0 > 1);
      errno = EINVAL;
      RAPICORN_CRITICAL_UNLESS (errno == 0);
      try {
        throw_if_fail (0 == "throw-if-fail");
      } catch (Rapicorn::AssertionError ar) {
        printerr ("Caught exception: %s\n", ar.what());
      }
      critical ("execution has reached a critical condition (\"test-critical\")");
      bogus_func();
      value_func (0);
      exit (0);
    }
}

int
main (int   argc,
      char *argv[])
{
  init_core_test (__SOURCE_COMPONENT__, &argc, argv);

  if (argc >= 2 || Test::logging())
    {
      // set testing switch 'testpid0' to enforce deterministic ouput
      debug_configure ("testpid0");
    }

  if (argc >= 2)
    test_logging (argv[1]);

  return Test::run();
}
