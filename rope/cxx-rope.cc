/* Rapicorn C++ Remote Object Programming Extension
 * Copyright (C) 2010 Tim Janik
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
#include "cxx-rope.hh"
using namespace Rapicorn;
#include "cxx-client.hh"        // generated client API
#include <ui/rope.hh>

namespace { // Anonymous

static void
cxxrope_init_dispatcher (const String         &appname,
                         const vector<String> &cmdline_args)
{
  int cpu = Thread::Self::affinity (1);
  uint64 app_id = rope_thread_start (appname, cmdline_args, cpu);
  if (app_id == 0)
    throw_error ("failed to initialize rapicorn thread");
  printout ("APPURL: 0x%016llx\n", app_id);
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  RapicornInitValue ivalues[] = { { NULL } };
  rapicorn_init_core (&argc, &argv, NULL, ivalues);
  vector<String> cmdline_args;
  for (int i = 1; i < argc; i++)
    cmdline_args.push_back (argv[i]);
  cxxrope_init_dispatcher (argv[0], cmdline_args);
  return 0;
}

// generated client implementation
#define PLIC_CALL_REMOTE        rope_thread_call
#include "cxx-client.cc"
