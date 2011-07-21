/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_TESTUTILS_HH__
#define __RAPICORN_TESTUTILS_HH__

#include <rcore/rcore.hh>

// Test Macros
#define TTITLE(...)             Rapicorn::Test::test_output (3, __VA_ARGS__)
#define TSTART(...)             Rapicorn::Test::test_output (4, __VA_ARGS__)
#define TDONE()                 Rapicorn::Test::test_output (5, "%s", "")
#define TOUT(...)               Rapicorn::Test::test_output (0, __VA_ARGS__)
#define TMSG(...)               Rapicorn::Test::test_output (1, __VA_ARGS__)
#define TINFO(...)              Rapicorn::Test::test_output (2, __VA_ARGS__)
#define TWARN(...)              Rapicorn::Test::test_output (6, __VA_ARGS__)
#define TRUN(name, func)        ({ TSTART (name); func(); TDONE(); })
#define TCMP(a,cmp,b)           TCMP_implf (a,cmp,b)
#define TCMPHEX(a,cmp,b)        TCMP_implx (a,cmp,b)
#define TCMPSIGNED(a,cmp,b)     TCMP_impls (a,cmp,b)
#define TCHECK(code)            TCHECK_impl (code)
#define TASSERT(code)           TCHECK_impl (code)
#define TOK()           do {} while (0) // printerr (".")
#define TCHECK_impl(code)       do { if (code) TOK(); else      \
      Rapicorn::Logging::message ("ABORT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                                  "assertion failed: %s", #code);       \
  } while (0)
#define TCMP_implf(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  double __tassert_va = a; double __tassert_vb = b;             \
  Rapicorn::Logging::message ("ABORT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                              "assertion failed: %s %s %s: %.17g %s %.17g", \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)
#define TCMP_implx(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  uint64 __tassert_va = a; uint64 __tassert_vb = b;             \
  Rapicorn::Logging::message ("ABORT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                              "assertion failed: %s %s %s: 0x%08Lx %s 0x%08Lx", \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)
#define TCMP_impls(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  int64 __tassert_va = a; int64 __tassert_vb = b;               \
  Rapicorn::Logging::message ("ABORT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
    "assertion failed: %s %s %s: %lld %s %lld",                         \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)

namespace Rapicorn {

void init_core_test (const String &app_ident, int *argcp, char **argv, const StringVector &args = StringVector());

namespace Test {

/**
 * Class for profiling benchmark tests.
 * UseCase: Benchmarking function implementations, e.g. to compare sorting implementations.
 */
class Timer {
  const double   m_deadline;
  vector<double> m_samples;
  double         m_test_duration;
  int64          m_n_runs;
  int64          loops_needed () const;
  void           reset        ();
  void           submit       (double elapsed, int64 repetitions);
  static double  bench_time   ();
public:
  /// Create a Timer() instance, specifying an optional upper bound for test durations.
  explicit       Timer        (double deadline_in_secs = 0);
  virtual       ~Timer        ();
  int64          n_runs       () const { return m_n_runs; }             ///< Number of benchmark runs executed
  double         test_elapsed () const { return m_test_duration; }      ///< Seconds spent in benchmark()
  double         min_elapsed  () const;         ///< Minimum time benchmarked for a @a callee() call.
  double         max_elapsed  () const;         ///< Maximum time benchmarked for a @a callee() call.
  template<typename Callee>
  double         benchmark    (Callee callee);
};

/**
 * @param callee        A callable function or object.
 * Method to benchmark the execution time of @a callee.
 * @returns Minimum runtime in seconds,
 */
template<typename Callee> double
Timer::benchmark (Callee callee)
{
  reset();
  for (int64 runs = loops_needed(); runs; runs = loops_needed())
    {
      int64 n = runs;
      const double start = bench_time();
      while (RAPICORN_LIKELY (n--))
        callee();
      const double stop = bench_time();
      submit (stop - start, runs);
    }
  return min_elapsed();
}

// === test maintenance ===
int     run             (void);         ///< Run all registered tests.
bool    verbose         (void);         ///< Indicates whether tests should run verbosely.
bool    logging         (void);         ///< Indicates whether only logging tests should be run.
bool    slow            (void);         ///< Indicates whether only slow tests should be run.
bool    ui_test         (void);         ///< Indicates execution of ui-thread tests.

void    test_output     (int kind, const char *format, ...) RAPICORN_PRINTF (2, 3);

void    add_internal    (const String &testname,
                         void        (*test_func) (void*),
                         void         *data);
void    add             (const String &funcname,
                         void (*test_func) (void));
template<typename D>
void    add             (const String &testname,
                         void        (*test_func) (D*),
                         D            *data)
{
  add_internal (testname, (void(*)(void*)) test_func, (void*) data);
}

class RegisterTest {
  static void add_test (char kind, const String &testname, void (*test_func) (void*), void *data);
public:
  RegisterTest (const char k, const String &testname, void (*test_func) (void))
  { add_test (k, testname, (void(*)(void*)) test_func, NULL); }
  RegisterTest (const char k, const String &testname, void (*test_func) (ptrdiff_t), ptrdiff_t data)
  { add_test (k, testname, (void(*)(void*)) test_func, (void*) data); }
  template<typename D>
  RegisterTest (const char k, const String &testname, void (*test_func) (D*), D *data)
  { add_test (k, testname, (void(*)(void*)) test_func, (void*) data); }
  typedef void (*TestTrigger)  (void (*runner) (void));
  static void test_set_trigger (TestTrigger func);
};

/// Register a standard test function for execution as unit test.
#define REGISTER_TEST(name, ...)     static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('t', name, __VA_ARGS__)

/// Register a slow test function for execution as during slow unit testing.
#define REGISTER_SLOWTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('s', name, __VA_ARGS__)

/// Register a logging test function for output recording and verification.
#define REGISTER_LOGTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('l', name, __VA_ARGS__)

// == Deterministic random numbers for tests ===
char    rand_bit                (void);                                 ///< Return a random bit.
int32   rand_int                (void);                                 ///< Return random int.
int32   rand_int_range          (int32 begin, int32 end);               ///< Return random int within range.
double  test_rand_double        (void);                                 ///< Return random double.
double  test_rand_double_range  (double range_start, double range_end); ///< Return random double within range.

} // Test
} // Rapicorn

#endif /* __RAPICORN_TESTUTILS_HH__ */
