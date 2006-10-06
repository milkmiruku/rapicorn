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
#include "birnetutilsxx.hh"
#include "birnetutils.hh"
#include "birnetthreadxx.hh"
#include "birnetcpu.h"
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <cxxabi.h>

extern "C" {

/* --- demangling --- */
gchar*
birnet_cxx_demangle (const char *mangled_identifier) /* from birnetutils.h */
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  gchar *result = g_strdup (malloced_result && !status ? malloced_result : mangled_identifier);
  if (malloced_result)
    free (malloced_result);
  return result;
}

/* --- initialization for C --- */
static void (*birnet_init_cplusplus_func) (void) = NULL;
static BirnetInitSettings default_init_settings = {
  false,        /* stand_alone */
};
BirnetInitSettings *birnet_init_settings = NULL;

static void
birnet_init_settings_values (BirnetInitValue bivalues[])
{
  BirnetInitValue *value = bivalues;
  while (value->value_name)
    {
      if (strcmp (value->value_name, "stand-alone") == 0)
        birnet_init_settings->stand_alone = birnet_init_value_bool (value);
      value++;
    }
}

void
birnet_init_extended (int            *argcp,    /* declared in birnetcore.h */
                      char         ***argvp,
                      const char     *app_name,
                      BirnetInitValue bivalues[])
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
      return;   /* ignore repeated initializations */
    }

  /* normal initialization */
  birnet_init_settings = &default_init_settings;
  if (bivalues)
    birnet_init_settings_values (bivalues);
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
  if (birnet_init_cplusplus_func)
    birnet_init_cplusplus_func();
}

} // "C"

namespace Birnet {

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

/* --- InitHooks --- */
static InitHook *init_hooks = NULL;

InitHook::InitHook (InitHookFunc _func,
                    int          _priority) :
  next (NULL), priority (_priority), hook (_func)
{
  BIRNET_ASSERT (birnet_init_settings == NULL);
  /* the above assertion guarantees single-threadedness */
  next = init_hooks;
  init_hooks = this;
  birnet_init_cplusplus_func = invoke_hooks;
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

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return      TRUE if @a file adhears to @a mode
 *
 * Perform various checks on @a file by calling birnet_file_check().
 */
bool
check (const String &file,
       const String &mode)
{
  return file_check (file.c_str(), mode.c_str());
}

/**
 * @param file1  possibly relative filename
 * @param file2  possibly relative filename
 * @return       TRUE if @a file1 and @a file2 are equal
 *
 * Check whether @a file1 and @a file2 are pointing to the same inode
 * in the same file system on the same device by calling birnet_file_equals().
 */
bool
equals (const String &file1,
        const String &file2)
{
  return file_equals (file1.c_str(), file2.c_str());
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
  Mutex                                            mutex;
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

} // Birnet
