/* RapicornSignal
 * Copyright (C) 2005-2006 Tim Janik
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
#include "birnetsignal.hh"

namespace Rapicorn {
namespace Signals {

/* --- TrampolineLink --- */
TrampolineLink::~TrampolineLink()
{
  if (next || prev)
    {
      next->prev = prev;
      prev->next = next;
      prev = next = NULL;
    }
}

/* --- SignalBase --- */
bool
SignalBase::EmbeddedLink::operator== (const TrampolineLink &other) const
{
  return false;
}

void
SignalBase::EmbeddedLink::delete_this ()
{
  /* not deleting, because this structure is always embedded as SignalBase::start */
}

} // Signals
} // Rapicorn
