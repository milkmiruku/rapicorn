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
#include "cmdlib.hh"

namespace Rapicorn {


static void
item_println (Item         &item,
            const String &args)
{
  printout ("%s\n", args.c_str());
}

static struct {
  void      (*cmd) (Item&, const String&);
  const char *name;
} item_cmds[] = {
  { item_println,         "Item::println" },
};

static void
window_close (Window       &window,
              const String &args)
{
  window.close();
}

static struct {
  void      (*cmd) (Window&, const String&);
  const char *name;
} window_cmds[] = {
  { window_close,       "Window::close" },
};

static void
application_close (Item         &item,
                   const String &args)
{
  printout ("app.close()\n");
}

static struct {
  void      (*cmd) (Item&, const String&);
  const char *name;
} application_cmds[] = {
  { application_close,  "Application::close" },
};

bool
command_lib_exec (Item         &item,
                  const String &cmd_name,
                  const String &args)
{
  for (uint ui = 0; ui < ARRAY_SIZE (item_cmds); ui++)
    if (item_cmds[ui].name == cmd_name)
      {
        item_cmds[ui].cmd (item, args);
        return true;
      }
  Root *root = item.get_root();
  if (root)
    {
      Window window = root->window();
      for (uint ui = 0; ui < ARRAY_SIZE (window_cmds); ui++)
        if (window_cmds[ui].name == cmd_name)
          {
            window_cmds[ui].cmd (window, args);
            return true;
          }
    }
  for (uint ui = 0; ui < ARRAY_SIZE (application_cmds); ui++)
    if (application_cmds[ui].name == cmd_name)
      {
        application_cmds[ui].cmd (item, args);
        return true;
      }
  return false;
}

static bool
parse_arg (const String &input,
           uint         *pos,
           String       *arg)
{
  uint a0 = *pos, level = 0;
  bool sq = false, dq = false, be = false, done = false;
  while (*pos < input.size())
    {
      switch (input[*pos])
        {
        case '\\':
          if (!be && (sq || dq))
            {
              be = true;
              (*pos)++;
              continue;
            }
          break;
        case '\'':
          if (!dq && (!sq || !be))
            sq = !sq;
          break;
        case '"':
          if (!sq && (!dq || !be))
            dq = !dq;
          break;
        case '(':
          if (!sq && !dq)
            level++;
          break;
        case ',':
          if (!sq && !dq && !level)
            done = true;
          break;
        case ')':
          if (!sq && !dq)
            {
              if (level)
                level--;
              else
                done = true;
            }
          break;
        default: ;
        }
      if (done)
        break;
      be = false;
      (*pos)++;
    }
  if (sq)
    {
      diag ("unclosed single-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (dq)
    {
      diag ("unclosed double-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (level)
    {
      diag ("unmatched parenthesis in command arg: %s", input.c_str());
      return false;
    }
  if (be)
    {
      diag ("invalid command arg: %s", input.c_str());
      return false;
    }
  if (!done)
    {
      diag ("unclosed command arg list: %s", input.c_str());
      return false;
    }
  *arg = input.substr (a0, *pos - a0);
  return true;
}

bool
command_scan (const String &input,
              String       *cmd_name,
              StringVector *args)
{
  const char *spaces = " \t\v\f\n\r";
  const char *ident0 = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char *identn = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  *cmd_name = "";
  args->resize (0);
  uint i = 0;
  /* skip leading spaces */
  while (i < input.size() && strchr (spaces, input[i]))
    i++;
  /* parse command name */
  const uint c0 = i;
  if (i < input.size() && strchr (ident0, input[i]))
    i++;
  while (i < input.size() && strchr (identn, input[i]))
    i++;
  const uint cl = i;
  /* skip intermediate spaces */
  while (i < input.size() && strchr (spaces, input[i]))
    i++;
  /* check command name */
  if (cl <= c0)
    {
      diag ("invalid command name: %s", input.c_str());
      return false;
    }
  *cmd_name = input.substr (c0, cl - c0);
  /* parse args */
  if (i < input.size() && input[i] == '(')
    {
      i++; // skip '('
      String arg;
      if (!parse_arg (input, &i, &arg))
        return false; // invalid arg syntax
      args->push_back (arg);
      while (i < input.size() && input[i] == ',')
        {
          i++;
          if (!parse_arg (input, &i, &arg))
            return false; // invalid arg syntax
          args->push_back (arg);
        }
      if (i >= input.size() || input[i] != ')')
        {
          diag ("missing closing parenthesis in command: %s", input.c_str());
          return false;
        }
      i++; // skip ')'
    }
  /* skip trailing spaces */
  while (i < input.size() && strchr (spaces, input[i]))
    i++;
  if (i < input.size() && input[i] != 0)
    {
      diag ("encountered junk after command: %s", input.c_str());
      return false;
    }
  return true;
}

} // Rapicorn
