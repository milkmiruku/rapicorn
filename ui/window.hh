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
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <rcore/rapicornsignal.hh>
#include <ui/primitives.hh>

namespace Rapicorn {

class Root;
class WinPtr : virtual public Deletable {
  Root         &m_root;
  WinPtr&       operator=       (const WinPtr   &window);
protected:
  explicit      WinPtr          (Root           &root);
public:
  /* reference ops */
  void          ref             () const;       // m_root.ref();
  Root&         root            () const;       // rapicorn_thread_entered()
  void          unref           () const;       // m_root.unref();
  /* window ops */
  bool          viewable        ();
  void          show            ();
  bool          closed          ();
  void          close           ();
  /* class */
  /*Copy*/      WinPtr          (const WinPtr &window);
  virtual      ~WinPtr          ();
  static WinPtr*from_locatable_id (uint64 objid);
  /* callbacks */
  typedef Signals::Slot2<bool,          const String&, const StringVector&> CmdSlot;
  typedef Signals::Slot3<bool, WinPtr&, const String&, const StringVector&> CmdSlotW;
  class Commands {
    WinPtr  &window;
    explicit Commands (WinPtr &w);
    friend   class WinPtr;
  public:
    void operator+= (const CmdSlot  &s);
    void operator-= (const CmdSlot  &s);
    void operator+= (const CmdSlotW &s);
    void operator-= (const CmdSlotW &s);
    void operator+= (bool (*callback) (const String&, const StringVector&));
    void operator-= (bool (*callback) (const String&, const StringVector&));
    void operator+= (bool (*callback) (WinPtr&, const String&, const StringVector&));
    void operator-= (bool (*callback) (WinPtr&, const String&, const StringVector&));
  };
  Commands      commands;
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
