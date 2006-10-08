/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_USER_CONFIG_HH_
#define __RAPICORN_USER_CONFIG_HH_

#include <birnet/birnet.hh>

namespace Rapicorn {

/* === i18n domain === */
#define RAPICORN_I18N_DOMAIN	"rapicorn"
/* make sure to call bindtextdomain (RAPICORN_I18N_DOMAIN, path_to_translation_directory); */

/* === Gtk+ backend === */
#define RAPICORN_WITH_GTK       (1)

/* === Pango font rendering === */
#define RAPICORN_WITH_PANGO     (1)

} // Rapicorn

#endif  /* __RAPICORN_USER_CONFIG_HH_ */
