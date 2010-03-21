/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#ifndef __RAPICORN_APPLICATION_HH__
#define __RAPICORN_APPLICATION_HH__

#include <ui/utilities.hh>
#include <ui/primitives.hh>
#include <ui/window.hh>
#include <stdlib.h>

namespace Rapicorn {

class Application {
protected:
  explicit              Application     ();
  RAPICORN_PRIVATE_CLASS_COPY (Application);
  static Application    self;           // singleton
public:
  /* --- public API --- */
  /* startup */
  void           init_with_x11   (int           *argcp,
                                  char        ***argvp,
                                  const char    *app_name);
  String         auto_path       (const String  &file_name,
                                  const String  &binary_path,
                                  bool           search_vpath = true);
  void           auto_load       (const String  &i18n_domain,
                                  const String  &file_name,
                                  const String  &binary_path);
  static void           pixstream       (const String  &pix_name,
                                         const uint8   *static_const_pixstream);
  /* windows */
  Window         create_window   (const String            &window_identifier,
                                  const std::list<String> &arguments = std::list<String>(),
                                  const std::list<String> &env_variables = std::list<String>());
  vector<Window> list_windows    ();
  /* execution */
  uint           execute_loops   ();
  void           exit            (uint   code = 0);
  /* notifications */
  Signal<Application, void ()> sig_missing_primary;
  /* global mutex */
  struct ApplicationMutex {
    static void lock    () { rapicorn_thread_enter (); }
    static bool trylock () { return rapicorn_thread_try_enter (); }
    static void unlock  () { rapicorn_thread_leave (); }
  };
  static ApplicationMutex mutex;  // singleton
};

extern Application &App;

} // Rapicorn

#endif  /* __RAPICORN_APPLICATION_HH__ */
