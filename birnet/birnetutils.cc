/* Birnet
 * Copyright (C) 2005-2006 Tim Janik
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
#include <glib.h>
#include "birnetutils.hh"
#include "birnetthread.hh"
#include "birnetmsg.hh"
#include "birnetcpu.hh"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <cxxabi.h>
#include <signal.h>

#ifndef _
#define _(s)    s
#endif

namespace Birnet {

/* --- InitHooks --- */
static void    (*run_init_hooks) () = NULL;
static InitHook *init_hooks = NULL;

InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  BIRNET_ASSERT (birnet_init_settings == NULL);
  /* the above assertion guarantees single-threadedness */
  next = init_hooks;
  init_hooks = this;
  run_init_hooks = invoke_hooks;
}

void
InitHook::invoke_hooks (void)
{
  std::vector<InitHook*> hv;
  struct Sub {
    static int
    init_hook_cmp (const InitHook *const &v1,
                   const InitHook *const &v2)
    {
      return v1->priority < v2->priority ? -1 : v1->priority > v2->priority;
    }
  };
  for (InitHook *ihook = init_hooks; ihook; ihook = ihook->next)
    hv.push_back (ihook);
  stable_sort (hv.begin(), hv.end(), Sub::init_hook_cmp);
  for (std::vector<InitHook*>::iterator it = hv.begin(); it != hv.end(); it++)
    (*it)->hook();
}

/* --- initialization --- */
const InitSettings *birnet_init_settings = NULL;
static InitSettings global_init_settings = {
  false,        /* stand_alone */
};

static void
apply_settings_values (InitValue *value)
{
  while (value->value_name)
    {
      if (strcmp (value->value_name, "stand-alone") == 0)
        global_init_settings.stand_alone = init_value_bool (value);
      value++;
    }
}

void
birnet_init (int        *argcp,
             char     ***argvp,
             const char *app_name,
             InitValue   ivalues[])
{
  /* mandatory initial initialization */
  if (!g_threads_got_initialized)
    g_thread_init (NULL);

  /* update program/application name upon repeated initilization */
  char *prg_name = argcp && *argcp ? g_path_get_basename ((*argvp)[0]) : NULL;
  if (birnet_init_settings != NULL)
    {
      if (prg_name && !g_get_prgname ())
        g_set_prgname (prg_name);
      g_free (prg_name);
      if (app_name && !g_get_application_name())
        g_set_application_name (app_name);
      return;   /* simply ignore repeated initializations */
    }

  /* normal initialization */
  birnet_init_settings = &global_init_settings;
  if (ivalues)
    apply_settings_values (ivalues);
  if (prg_name)
    g_set_prgname (prg_name);
  g_free (prg_name);
  if (app_name && (!g_get_application_name() || g_get_application_name() == g_get_prgname()))
    g_set_application_name (app_name);

  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand48 (tv.tv_usec + (tv.tv_sec << 16));
    srand (lrand48());
  }

  /* initialize sub systems */
  _birnet_init_cpuinfo();
  _birnet_init_threads();
  if (run_init_hooks)
    run_init_hooks();
}

bool
init_value_bool (InitValue *value)
{
  if (value->value_string)
    switch (value->value_string[0])
      {
      case 0:   // FIXME: use string_to_bool()
      case '0': case 'f': case 'F':
      case 'n': case 'N':               /* false assigments */
        return FALSE;
      default:
        return TRUE;
      }
  else
    return ABS (value->value_num) >= 0.5;
}

double
init_value_double (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return g_strtod (value->value_string, NULL);
  return value->value_num;
}

int64
init_value_int (InitValue *value)
{
  if (value->value_string && value->value_string[0])
    return strtoll (value->value_string, NULL, 0);
  return value->value_num + 0.5;
}

/* --- assertions/warnings/errors --- */
void
raise_sigtrap ()
{
  raise (SIGTRAP);
}

static void
stderr_print (bool        bail_out,
              const char *prefix,
              const char *domain,
              const char *file,
              int         line,
              const char *funcname,
              const char *pmsg,
              const char *str)
{
  fflush (stdout);
  String msg (bail_out ? "\n" : "");
  if (domain)
    msg += domain + String ("-") + prefix;
  else
    msg += prefix;
  if (file)
    {
      char buffer[64];
      sprintf (buffer, "%d", line);
      msg += String (":") + file + String (":") + String (buffer);
    }
  if (funcname)
    msg += String (":") + funcname + "()";
  if (pmsg)
    msg += String (": ") + pmsg;
  if (str)
    msg += String (": ") + str;
  msg += "\n";
  if (bail_out)
    msg += "aborting...\n";
  fputs (msg.c_str(), stderr);
  fflush (stderr);
}

void
birnet_runtime_problem (char        ewran_tag,
                        const char *domain,
                        const char *file,
                        int         line,
                        const char *funcname,
                        const char *msgformat,
                        ...)
{
  va_list args;
  va_start (args, msgformat);
  birnet_runtime_problemv (ewran_tag, domain, file, line, funcname, msgformat, args);
  va_end (args);
}

void
birnet_runtime_problemv (char        ewran_tag,
                         const char *domain,
                         const char *file,
                         int         line,
                         const char *funcname,
                         const char *msgformat,
                         va_list     args)
{
  const bool noreturn_case = ewran_tag == 'E' || ewran_tag == 'A' || ewran_tag == 'N';
  char *msg = NULL;
  if (msgformat && msgformat[0])
    msg = g_strdup_vprintf (msgformat, args);
  const char *prefix, *pmsg = NULL;
  switch (ewran_tag)
    {
    case 'E':
      prefix = "ERROR";
      break;
    case 'W':
      prefix = "WARNING";
      break;
    case 'R':
      prefix = "WARNING:";
      pmsg = "Check failed";
      break;
    case 'A':
      prefix = "ERROR";
      pmsg = "Assertion failed";
      break;
    default:
    case 'N':
      prefix = "ERROR";
      pmsg = "Assertion should not be reached";
      break;
    }
  stderr_print (noreturn_case, prefix, domain, file, line, funcname, pmsg, msg);
  g_free (msg);
  if (noreturn_case)
    {
      BREAKPOINT();
      abort();
    }
}

/* --- VirtualTypeid --- */
VirtualTypeid::~VirtualTypeid ()
{ /* virtual destructor ensures vtable */ }

String
VirtualTypeid::typeid_name ()
{
  return typeid (*this).name();
}

String
VirtualTypeid::typeid_pretty_name ()
{
  return cxx_demangle (typeid (*this).name());
}

String
VirtualTypeid::cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

/* --- file utils --- */
namespace Path {

const String
dirname (const String &path)
{
  const char *filename = path.c_str();
  const char *base = strrchr (filename, BIRNET_DIR_SEPARATOR);
  if (!base)
    return ".";
  while (*base == BIRNET_DIR_SEPARATOR && base > filename)
    base--;
  return String (filename, base - filename + 1);
}

const String
basename (const String &path)
{
  const char *filename = path.c_str();
  const char *base = strrchr (filename, BIRNET_DIR_SEPARATOR);
  if (!base)
    return filename;
  return String (base + 1);
}

bool
isabs (const String &path)
{
  return g_path_is_absolute (path.c_str());
}

const String
skip_root (const String &path)
{
  const char *frag = g_path_skip_root (path.c_str());
  return frag;
}

const String
join (const String &frag0, const String &frag1,
      const String &frag2, const String &frag3,
      const String &frag4, const String &frag5,
      const String &frag6, const String &frag7,
      const String &frag8, const String &frag9,
      const String &frag10, const String &frag11,
      const String &frag12, const String &frag13,
      const String &frag14, const String &frag15)
{
  char *cpath = g_build_path (BIRNET_DIR_SEPARATOR_S, frag0.c_str(),
                              frag1.c_str(), frag2.c_str(), frag3.c_str(), frag4.c_str(), 
                              frag5.c_str(), frag6.c_str(), frag7.c_str(), frag8.c_str(),
                              frag9.c_str(), frag10.c_str(), frag11.c_str(), frag12.c_str(),
                              frag13.c_str(), frag14.c_str(), frag15.c_str(), NULL);
  String path (cpath);
  g_free (cpath);
  return path;
}

static int
errno_check_file (const char *file_name,
                  const char *mode)
{
  uint access_mask = 0, nac = 0;
  
  if (strchr (mode, 'e'))       /* exists */
    nac++, access_mask |= F_OK;
  if (strchr (mode, 'r'))       /* readable */
    nac++, access_mask |= R_OK;
  if (strchr (mode, 'w'))       /* writable */
    nac++, access_mask |= W_OK;
  bool check_exec = strchr (mode, 'x') != NULL;
  if (check_exec)               /* executable */
    nac++, access_mask |= X_OK;
  
  /* on some POSIX systems, X_OK may succeed for root without any
   * executable bits set, so we also check via stat() below.
   */
  if (nac && access (file_name, access_mask) < 0)
    return -errno;
  
  bool check_file = strchr (mode, 'f') != NULL;     /* open as file */
  bool check_dir  = strchr (mode, 'd') != NULL;     /* open as directory */
  bool check_link = strchr (mode, 'l') != NULL;     /* open as link */
  bool check_char = strchr (mode, 'c') != NULL;     /* open as character device */
  bool check_block = strchr (mode, 'b') != NULL;    /* open as block device */
  bool check_pipe = strchr (mode, 'p') != NULL;     /* open as pipe */
  bool check_socket = strchr (mode, 's') != NULL;   /* open as socket */
  
  if (check_exec || check_file || check_dir || check_link || check_char || check_block || check_pipe || check_socket)
    {
      struct stat st;
      
      if (check_link)
        {
          if (lstat (file_name, &st) < 0)
            return -errno;
        }
      else if (stat (file_name, &st) < 0)
        return -errno;
      
      if (0)
        g_printerr ("file-check(\"%s\",\"%s\"): %s%s%s%s%s%s%s\n",
                    file_name, mode,
                    S_ISREG (st.st_mode) ? "f" : "",
                    S_ISDIR (st.st_mode) ? "d" : "",
                    S_ISLNK (st.st_mode) ? "l" : "",
                    S_ISCHR (st.st_mode) ? "c" : "",
                    S_ISBLK (st.st_mode) ? "b" : "",
                    S_ISFIFO (st.st_mode) ? "p" : "",
                    S_ISSOCK (st.st_mode) ? "s" : "");
      
      if (S_ISDIR (st.st_mode) && (check_file || check_link || check_char || check_block || check_pipe))
        return -EISDIR;
      if (check_file && !S_ISREG (st.st_mode))
        return -EINVAL;
      if (check_dir && !S_ISDIR (st.st_mode))
        return -ENOTDIR;
      if (check_link && !S_ISLNK (st.st_mode))
        return -EINVAL;
      if (check_char && !S_ISCHR (st.st_mode))
        return -ENODEV;
      if (check_block && !S_ISBLK (st.st_mode))
        return -ENOTBLK;
      if (check_pipe && !S_ISFIFO (st.st_mode))
        return -ENXIO;
      if (check_socket && !S_ISSOCK (st.st_mode))
        return -ENOTSOCK;
      if (check_exec && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return -EACCES; /* for root executable, any +x bit is good enough */
    }
  
  return 0;
}

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return      true if @a file adhears to @a mode
 *
 * Perform various checks on @a file and return whether all
 * checks passed. On failure, errno is set appropriately, and
 * FALSE is returned. Available features to be checked for are:
 * @itemize
 * @item e - @a file must exist
 * @item r - @a file must be readable
 * @item w - @a file must be writable
 * @item x - @a file must be executable
 * @item f - @a file must be a regular file
 * @item d - @a file must be a directory
 * @item l - @a file must be a symbolic link
 * @item c - @a file must be a character device
 * @item b - @a file must be a block device
 * @item p - @a file must be a named pipe
 * @item s - @a file must be a socket.
 * @done
 */
bool
check (const String &file,
       const String &mode)
{
  int err = file.size() && mode.size() ? errno_check_file (file.c_str(), mode.c_str()) : -EFAULT;
  errno = err < 0 ? -err : 0;
  return errno == 0;
}

/**
 * @param file1  possibly relative filename
 * @param file2  possibly relative filename
 * @return       TRUE if @a file1 and @a file2 are equal
 *
 * Check whether @a file1 and @a file2 are pointing to the same inode
 * in the same file system on the same device.
 */
bool
equals (const String &file1,
        const String &file2)
{
  if (!file1.size() || !file2.size())
    return file1.size() == file2.size();
  struct stat st1 = { 0, }, st2 = { 0, };
  int err1 = 0, err2 = 0;
  errno = 0;
  if (stat (file1.c_str(), &st1) < 0 && stat (file1.c_str(), &st1) < 0)
    err1 = errno;
  errno = 0;
  if (stat (file2.c_str(), &st2) < 0 && stat (file2.c_str(), &st2) < 0)
    err2 = errno;
  if (err1 || err2)
    return err1 == err2;
  return (st1.st_dev  == st2.st_dev &&
          st1.st_ino  == st2.st_ino &&
          st1.st_rdev == st2.st_rdev);
}

} // Path

/* --- Deletable --- */
Deletable::~Deletable ()
{
  invoke_deletion_hooks();
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was added
 *
 * Adds the deletion hook to @a deletable if it is non NULL.
 * The deletion hook is asserted to be so far uninstalled.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_add_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->add_deletion_hook (this);
      return true;
    }
  return false;
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was removed
 *
 * Removes the deletion hook from @a deletable if it is non NULL.
 * The deletion hook is asserted to be installed on @a deletable.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_remove_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->remove_deletion_hook (this);
      return true;
    }
  return false;
}

static struct {
  Mutex                                         mutex;
  std::map<Deletable*,Deletable::DeletionHook*> dmap;
} deletable_maps[19]; /* use prime size for hashing, sum up to roughly 1k (use 83 for 4k) */

/**
 * @param hook  valid deletion hook
 *
 * Add an uninstalled deletion hook to the deletable.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::add_deletion_hook (DeletionHook *hook)
{
  uint32 hashv = ((gsize) (void*) this) % (sizeof (deletable_maps) / sizeof (deletable_maps[0]));
  deletable_maps[hashv].mutex.lock();
  BIRNET_ASSERT (hook);
  BIRNET_ASSERT (!hook->next);
  BIRNET_ASSERT (!hook->prev);
  std::map<Deletable*,DeletionHook*>::iterator it;
  it = deletable_maps[hashv].dmap.find (this);
  if (it != deletable_maps[hashv].dmap.end())
    {
      hook->next = it->second;
      it->second = hook;
      if (hook->next)
        hook->next->prev = hook;
    }
  else
    deletable_maps[hashv].dmap[this] = hook;
  deletable_maps[hashv].mutex.unlock();
  //g_printerr ("DELETABLE-ADD(%p,%p)\n", this, hook);
}

/**
 * @param hook  valid deletion hook
 *
 * Remove a previously added deletion hook.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::remove_deletion_hook (DeletionHook *hook)
{
  uint32 hashv = ((gsize) (void*) this) % (sizeof (deletable_maps) / sizeof (deletable_maps[0]));
  deletable_maps[hashv].mutex.lock();
  BIRNET_ASSERT (hook);
  if (hook->next)
    hook->next->prev = hook->prev;
  if (hook->prev)
    hook->prev->next = hook->next;
  else
    {
      std::map<Deletable*,DeletionHook*>::iterator it;
      it = deletable_maps[hashv].dmap.find (this);
      BIRNET_ASSERT (it != deletable_maps[hashv].dmap.end());
      BIRNET_ASSERT (it->second == hook);
      it->second = hook->next;
    }
  hook->prev = NULL;
  hook->next = NULL;
  deletable_maps[hashv].mutex.unlock();
  //g_printerr ("DELETABLE-REM(%p,%p)\n", this, hook);
}

/**
 * Invoke all deletion hooks installed on this deletable.
 */
void
Deletable::invoke_deletion_hooks()
{
  uint32 hashv = ((gsize) (void*) this) % (sizeof (deletable_maps) / sizeof (deletable_maps[0]));
  while (TRUE)
    {
      /* lookup hook list */
      deletable_maps[hashv].mutex.lock();
      std::map<Deletable*,DeletionHook*>::iterator it;
      DeletionHook *hooks;
      it = deletable_maps[hashv].dmap.find (this);
      if (it != deletable_maps[hashv].dmap.end())
        {
          hooks = it->second;
          deletable_maps[hashv].dmap.erase (it);
        }
      else
        hooks = NULL;
      deletable_maps[hashv].mutex.unlock();
      /* we're done if all hooks have been procesed */
      if (!hooks)
        break;
      /* process hooks */
      while (hooks)
        {
          DeletionHook *hook = hooks;
          hooks = hook->next;
          if (hooks)
            hooks->prev = NULL;
          hook->prev = NULL;
          hook->next = NULL;
          //g_printerr ("DELETABLE-DIS(%p,%p)\n", this, hook);
          hook->deletable_dispose (*this);
        }
    }
}

/* --- ReferenceCountImpl --- */
void
ReferenceCountImpl::ref_diag (const char *msg) const
{
  fprintf (stderr, "%s: this=%p ref_count=%d floating=%d", msg ? msg : "ReferenceCountImpl", this, ref_count(), floating());
}

void
ReferenceCountImpl::finalize ()
{}

void
ReferenceCountImpl::delete_this ()
{
  delete this;
}

ReferenceCountImpl::~ReferenceCountImpl ()
{
  BIRNET_ASSERT (ref_count() == 0);
}

/* --- DataList --- */
DataList::NodeBase::~NodeBase ()
{}

void
DataList::set_data (NodeBase *node)
{
  /* delete old node */
  NodeBase *it = rip_data (node->key);
  if (it)
    delete it;
  /* prepend node */
  node->next = nodes;
  nodes = node;
}

DataList::NodeBase*
DataList::get_data (DataKey<void> *key) const
{
  NodeBase *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      return it;
  return NULL;
}

DataList::NodeBase*
DataList::rip_data (DataKey<void> *key)
{
  NodeBase *last = NULL, *it;
  for (it = nodes; it; last = it, it = last->next)
    if (it->key == key)
      {
        /* unlink existing node */
        if (last)
          last->next = it->next;
        else
          nodes = it->next;
        it->next = NULL;
        return it;
      }
  return NULL;
}

void
DataList::clear_like_destructor()
{
  while (nodes)
    {
      NodeBase *it = nodes;
      nodes = it->next;
      it->next = NULL;
      delete it;
    }
}

DataList::~DataList()
{
  clear_like_destructor();
}

/* --- url handling --- */
bool
url_test_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* system browser launchers */
    { "sensible-browser",       NULL,           "", "" },
    { "x-www-browser",          NULL,           "", "" },
    { "htmlview",               NULL,           "", "" },
    /* portable browser launchers */
    { "xdg-open",               NULL,           "", "" },
#if 1
    /* desktop browser launchers */
    { "gnome-open",             NULL,           "", "" },
    { "kfmclient",              "openURL",      "", "" },
    { "gnome-moz-remote",       "--newwin"      "", "" },
    /* specific browser programs */
    { "firefox",                NULL,           "", "" },
    { "mozilla-firefox",        NULL,           "", "" },
    { "mozilla",                NULL,           "", "" },
    { "opera",                  "-newwindow",   "", "" },
    { "konqueror",              NULL,           "", "" },
#endif
    /* above, we give system browser launchers precedence over xdg-open
     * (especially the debian sensible-browser script), because xdg-open
     * tends to exhibit bugs in desktop browser launchers still (e.g.
     * gnome-open not honouring the users browser setting for file:///
     * urls).
     */
  };
  uint i;
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        bool success = g_spawn_async (NULL, /* cwd */
                                      args,
                                      NULL, /* envp */
                                      G_SPAWN_SEARCH_PATH,
                                      NULL, /* child_setup() */
                                      NULL, /* user_data */
                                      NULL, /* child_pid */
                                      &error);
        g_free (string);
        // g_printerr ("show \"%s\": %s: %s\n", url, args[0], error ? error->message : "Ok");
        g_clear_error (&error);
        if (success)
          return TRUE;
        www_browsers[i].disabled = true;
      }
  /* reset disabled states if no browser could be found */
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

static void
browser_launch_warning (const char *url)
{
  Msg::display (Msg::WARNING,
                Msg::Title (_("Launch Web Browser")),
                Msg::Text1 (_("Failed to launch a web browser executable")),
                Msg::Text2 (_("No suitable web browser executable could be found to be executed and to display the URL: %s"), url),
                Msg::Check (_("Show messages about web browser launch problems")));
}

void
url_show (const char *url)
{
  bool success = url_test_show (url);
  if (!success)
    browser_launch_warning (url);
}

static void
unlink_file_name (gpointer data)
{
  char *file_name = (char*) data;
  while (unlink (file_name) < 0 && errno == EINTR);
  g_free (file_name);
}

static const gchar*
url_create_redirect (const char    *url,
                     const char    *url_title,
                     const char    *cookie)
{
  const char *ver = "0.5";
  gchar *tname = NULL;
  gint fd = -1;
  while (fd < 0)
    {
      g_free (tname);
      tname = g_strdup_printf ("/tmp/Url%08X%04X.html", (int) lrand48(), getpid());
      fd = open (tname, O_WRONLY | O_CREAT | O_EXCL, 00600);
      if (fd < 0 && errno != EEXIST)
        {
          g_free (tname);
          return NULL;
        }
    }
  char *text = g_strdup_printf ("<!DOCTYPE HTML SYSTEM>\n"
                                "<html><head>\n"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
                                "<meta http-equiv=\"refresh\" content=\"0; URL=%s\">\n"
                                "<meta http-equiv=\"set-cookie\" content=\"%s\">\n"
                                "<title>%s</title>\n"
                                "</head><body>\n"
                                "<h1>%s</h1>\n"
                                "<b>Document Redirection</b><br>\n"
                                "Your browser is being redirected.\n"
                                "If it does not support automatic redirections, try <a href=\"%s\">%s</a>.\n"
                                "<hr>\n"
                                "<address>BirnetUrl/%s file redirect</address>\n"
                                "</body></html>\n",
                                url, cookie, url_title, url_title, url, url, ver);
  int w, c, l = strlen (text);
  do
    w = write (fd, text, l);
  while (w < 0 && errno == EINTR);
  g_free (text);
  do
    c = close (fd);
  while (c < 0 && errno == EINTR);
  if (w != l || c < 0)
    {
      while (unlink (tname) < 0 && errno == EINTR)
        {}
      g_free (tname);
      return NULL;
    }
  cleanup_add (60 * 1000, unlink_file_name, tname); /* free tname */
  return tname;
}

bool
url_test_show_with_cookie (const char *url,
                           const char *url_title,
                           const char *cookie)
{
  const char *redirect = url_create_redirect (url, url_title, cookie);
  if (redirect)
    return url_test_show (redirect);
  else
    return url_test_show (url);
}

void
url_show_with_cookie (const char *url,
                      const char *url_title,
                      const char *cookie)
{
  bool success = url_test_show_with_cookie (url, url_title, cookie);
  if (!success)
    browser_launch_warning (url);
}

/* --- cleanups --- */
typedef struct {
  uint           id;
  GDestroyNotify handler;
  void          *data;
} Cleanup;

static Mutex cleanup_mutex;
static GSList *cleanup_list = NULL;

static void
cleanup_exec_Lm (Cleanup *cleanup)
{
  cleanup_list = g_slist_remove (cleanup_list, cleanup);
  g_source_remove (cleanup->id);
  GDestroyNotify handler = cleanup->handler;
  void *data = cleanup->data;
  g_free (cleanup);
  cleanup_mutex.unlock();
  handler (data);
  cleanup_mutex.lock();
}

/**
 * Force all cleanup handlers (see birnet_cleanup_add()) to be immediately
 * executed. This function should be called at program exit to execute
 * cleanup handlers which have timeouts that have not yet expired.
 */
void
cleanup_force_handlers (void)
{
  cleanup_mutex.lock();
  while (cleanup_list)
    cleanup_exec_Lm ((Cleanup*) cleanup_list->data);
  cleanup_mutex.unlock();
}

static gboolean
cleanup_exec (gpointer data)
{
  cleanup_mutex.lock();
  cleanup_exec_Lm ((Cleanup*) data);
  cleanup_mutex.unlock();
  return FALSE;
}

/**
 * @param timeout_ms    timeout in milliseconds
 * @param handler       cleanup handler to run
 * @param data          cleanup handler data
 *
 * Register a cleanup handler, the @a handler is guaranteed to be run
 * asyncronously (i.e. not from within cleanup_add()). The cleanup
 * handler will be called as soon as @a timeout_ms has elapsed or
 * cleanup_force_handlers() is called.
 */
uint
cleanup_add (guint          timeout_ms,
             GDestroyNotify handler,
             void          *data)
{
  Cleanup *cleanup = g_new0 (Cleanup, 1);
  cleanup->handler = handler;
  cleanup->data = data;
  cleanup->id = g_timeout_add (timeout_ms, cleanup_exec, cleanup);
  cleanup_mutex.lock();
  cleanup_list = g_slist_prepend (cleanup_list, cleanup);
  cleanup_mutex.unlock();
  return cleanup->id;
}

/* --- string utils --- */
void
memset4 (guint32        *mem,
         guint32         filler,
         guint           length)
{
  BIRNET_STATIC_ASSERT (sizeof (*mem) == 4);
  BIRNET_STATIC_ASSERT (sizeof (filler) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) mem, filler, length);
}

/* --- memory utils --- */
void*
malloc_aligned (gsize	  total_size,
                gsize	  alignment,
                guint8	**free_pointer)
{
  uint8 *aligned_mem = (uint8*) g_malloc (total_size);
  *free_pointer = aligned_mem;
  if (!alignment || !(ptrdiff_t) aligned_mem % alignment)
    return aligned_mem;
  g_free (aligned_mem);
  aligned_mem = (uint8*) g_malloc (total_size + alignment - 1);
  *free_pointer = aligned_mem;
  if ((ptrdiff_t) aligned_mem % alignment)
    aligned_mem += alignment - (ptrdiff_t) aligned_mem % alignment;
  return aligned_mem;
}

/* --- zintern support --- */
#include <zlib.h>

/**
 * @param decompressed_size exact size of the decompressed data to be returned
 * @param cdata             compressed data block
 * @param cdata_size        exact size of the compressed data block
 * @returns                 decompressed data block or NULL in low memory situations
 *
 * Decompress the data from @a cdata of length @a cdata_size into a newly
 * allocated block of size @a decompressed_size which is returned.
 * The returned block needs to be freed with g_free().
 * This function is intended to decompress data which has been compressed
 * with the birnet-zintern utility, so no errors should occour during
 * decompression.
 * Consequently, if any error occours during decompression or if the resulting
 * data block is of a size other than @a decompressed_size, the program will
 * abort with an appropriate error message.
 * If not enough memory could be allocated for decompression, NULL is returned.
 */
uint8*
zintern_decompress (unsigned int          decompressed_size,
                    const unsigned char  *cdata,
                    unsigned int          cdata_size)
{
  uLongf dlen = decompressed_size;
  uint64 len = dlen + 1;
  uint8 *text = (uint8*) g_try_malloc (len);
  if (!text)
    return NULL;        /* handle ENOMEM gracefully */
  
  int64 result = uncompress (text, &dlen, cdata, cdata_size);
  const char *err;
  switch (result)
    {
    case Z_OK:
      if (dlen == decompressed_size)
        {
          err = NULL;
          break;
        }
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      g_free (text);
      return NULL;      /* handle ENOMEM gracefully */
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    g_error ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);
  
  text[dlen] = 0;
  return text;          /* success */
}

} // Birnet
