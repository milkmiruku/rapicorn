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
#include "properties.hh"

namespace Rapicorn {

static inline String
canonify (String s)
{
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '-'))
      s[i] = '-';
  return s;
}

Property::Property (const char *cident, const char *clabel, const char *cblurb, const char *chints) :
  ident (cident),
  label (clabel ? strdup (clabel) : NULL),
  blurb (cblurb ? strdup (cblurb) : NULL),
  hints (chints ? strdup (chints) : NULL)
{
  assert (ident != NULL);
  ident = strdup (canonify (ident).c_str());
}

Property::~Property()
{
  if (ident)
    free (const_cast<char*> (ident));
  if (label)
    free (label);
  if (blurb)
    free (blurb);
  if (hints)
    free (hints);
}

} // Rapicorn
