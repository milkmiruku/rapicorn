/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_BUTTONS_HH__
#define __RAPICORN_BUTTONS_HH__

#include <rapicorn/utilities.hh>
#include <rapicorn/enumdefs.hh>

namespace Rapicorn {

class ButtonView : public virtual Convertible {
protected:
  explicit      ButtonView ();
  virtual bool  pressed    ();
  virtual void  do_clicked ();
public:
  Signal<ButtonView,void()> sig_clicked;
};

} // Rapicorn

#endif  /* __RAPICORN_BUTTONS_HH__ */
