/* Tests
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
//#define TEST_VERBOSE
#include <rapicorn-core/testutils.hh>
#include <rapicorn/rapicorn.hh>
using namespace Rapicorn;

#include "../../configure.h"    // for HAVE_READLINE_AND_HISTORY
#ifdef HAVE_READLINE_AND_HISTORY
#include <readline/readline.h>  // for --shell
#include <readline/history.h>   // for --shell
#endif

namespace {
using namespace Rapicorn;

static void
test_basics ()
{
  Sinfex *sinfex = Sinfex::parse_string ("21");
  ref_sink (sinfex);
  assert (sinfex != NULL);
  assert (sinfex->eval (*(Sinfex::Scope*) NULL).real() == 21);
  unref (sinfex);
}

static RAPICORN_UNUSED char*
freadline (const char *prompt,
           FILE       *stream)
{
  int sz = 2;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  malloc_string[0] = 0;
  if (prompt)
    fputs (prompt, stderr);
  char *start = malloc_string;
 continue_readline:
  if (!fgets (start, sz - (start - malloc_string), stdin))
    {
      if (start != malloc_string)
        return malloc_string;
      free (malloc_string);
      return NULL; // end of input
    }
  if (strchr (start, '\n'))
    return malloc_string;
  sz *= 2;
  char *newstring = (char*) realloc (malloc_string, sz);
  if (!newstring)
    {
      free (malloc_string);
      return NULL; // OOM during single line input
    }
  malloc_string = newstring;
  start = malloc_string + strlen (malloc_string);
  goto continue_readline;
}

struct EvalScope : public Sinfex::Scope {
  virtual Sinfex::Value
  resolve_variable (const String        &entity,
                    const String        &name)
  {
    printout ("VAR: %s.%s\n", entity.c_str(), name.c_str());
    return Sinfex::Value (0);
  }
  virtual Sinfex::Value
  call_function (const String                &entity,
                 const String                &name,
                 const vector<Sinfex::Value> &args)
  {
    printout ("FUNC: %s (", name.c_str());
    for (uint i = 0; i < args.size(); i++)
      printout ("%s%s", i ? ", " : "", args[i].tostring().c_str());
    printout (");\n");
    return Sinfex::Value (0);
  }
};

extern "C" int
main (int   argc,
      char *argv[])
{
  bool shell_mode = argc >= 2 && strcmp (argv[1], "--shell") == 0;

  if (!shell_mode)
    rapicorn_init_test (&argc, &argv);
  else
    Application::init_with_x11 (&argc, &argv, "sinfextest");

  if (shell_mode)
    {
      bool interactive_prompt = isatty (fileno (stdin));
      char *malloc_string;
#ifdef HAVE_READLINE_AND_HISTORY
      rl_instream = stdin;
      rl_readline_name = "Rapicorn"; // for inputrc conditionals
      rl_bind_key ('\t', rl_insert);
      using_history ();
      stifle_history (999);
#endif
      do
        {
#ifdef HAVE_READLINE_AND_HISTORY
          malloc_string = readline (interactive_prompt ? "sinfex> " : "");
          if (malloc_string && malloc_string[0] != 0 && !strchr (" \t\v", malloc_string[0]))
            add_history (malloc_string);
#else
          malloc_string = freadline (interactive_prompt ? "sinfex> " : "", stdin);
#endif
          if (malloc_string)
            {
              Sinfex *sinfex = Sinfex::parse_string (malloc_string);
              ref_sink (sinfex);
              free (malloc_string);
              EvalScope scope;
              Sinfex::Value v = sinfex->eval (scope);
              String s = v.tostring();
              if (v.isreal())
                {
                  char buffer[128];
                  snprintf (buffer, 128, "%.15g", v.real());
                  s = buffer;
                }
              printf ("= %s\n", s.c_str());
              unref (sinfex);
            }
        }
      while (malloc_string);
      if (interactive_prompt)
        fprintf (stderr, "\n"); // newline after last prompt
      return 0;
    }

  Test::add ("Sinfex/Basics", test_basics);

  return Test::run();
}

} // Anon
