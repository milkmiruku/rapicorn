/* Rapicorn
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
#include "viewport.hh"
#include <list>
using namespace std;

namespace Rapicorn {

/* --- Viewport::FactoryBase --- */
static std::list<Viewport::FactoryBase*> viewport_backends;
void
Viewport::FactoryBase::register_backend (FactoryBase &factory)
{
  viewport_backends.push_back (&factory);
}

/* --- Viewport --- */
Viewport*
Viewport::create_viewport (const String            &backend_name,
                           WindowType               viewport_type,
                           Viewport::EventReceiver &receiver)
{
  std::list<Viewport::FactoryBase*>::iterator it;
  for (it = viewport_backends.begin(); it != viewport_backends.end(); it++)
    if (backend_name == (*it)->m_name)
      return (*it)->create_viewport (viewport_type, receiver);
  if (backend_name == "auto")
    {
      /* cruel approximation of automatic selection logic */
      it = viewport_backends.begin();
      if (it != viewport_backends.end())
        return (*it)->create_viewport (viewport_type, receiver);
    }
  return NULL;
}

} // Rapicorn
