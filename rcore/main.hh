// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_MAIN_HH__
#define __RAPICORN_MAIN_HH__

#include <rcore/utilities.hh>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

namespace Rapicorn {

// == initialization ==
void    init_core               (const String       &app_ident,
                                 int                *argcp,
                                 char              **argv,
                                 const StringVector &args = StringVector());
class InitSettings {
  static const InitSettings &is;
protected:
  uint test_codes_;
  bool autonomous_;
public:
  static bool  autonomous()    { return is.autonomous_; } ///< self-contained runtime, no rcfiles, boot scripts, etc
  static uint  test_codes()    { return is.test_codes_; } // internal test flags
};

bool    arg_parse_option        (uint         argc,
                                 char       **argv,
                                 size_t      *i,
                                 const char  *arg);
bool    arg_parse_string_option (uint         argc,
                                 char       **argv,
                                 size_t      *i,
                                 const char  *arg,
                                 const char **strp);
int     arg_parse_collapse      (int         *argcp,
                                 char       **argv);
String  rapicorn_version        ();
String  rapicorn_buildid        ();

// == locale ==

/// Class to push a specific locale_t for the scope of its lifetime.
class ScopedLocale {
  locale_t      locale_;
  /*copy*/      ScopedLocale (const ScopedLocale&) = delete;
  ScopedLocale& operator=    (const ScopedLocale&) = delete;
protected:
  explicit      ScopedLocale (locale_t scope_locale);
public:
  // explicit   ScopedLocale (const String &locale_name = ""); // not supported
  /*dtor*/     ~ScopedLocale ();
};

/// Class to push the POSIX/C locale_t (UTF-8) for the scope of its lifetime.
class ScopedPosixLocale : public ScopedLocale {
public:
  explicit        ScopedPosixLocale ();
  static locale_t posix_locale      (); ///< Retrieve the (UTF-8) POSIX/C locale_t.
};

// == process info ==
String       program_file       ();
String       program_alias      ();
String       program_ident      ();
String       program_cwd        ();

// == initialization hooks ==
class InitHook {
  typedef void (*InitHookFunc) (const StringVector &args);
  InitHook    *next;
  InitHookFunc hook;
  const String name_;
  RAPICORN_CLASS_NON_COPYABLE (InitHook);
protected:
  static void  invoke_hooks (const String&, int*, char**, const StringVector&);
public:
  String       name () const { return name_; }
  StringVector main_args () const;
  explicit     InitHook (const String &fname, InitHookFunc func);
};

} // Rapicorn

#endif /* __RAPICORN_MAIN_HH__ */
