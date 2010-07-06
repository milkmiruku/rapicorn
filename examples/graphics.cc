/* Rapicorn Examples
 * Copyright (C) 2005 Tim Janik
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
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
drawable_draw (Display  &display,
               Drawable &drawable)
{
  Plane &plane = display.create_plane();
  Painter painter (plane);
  Rect area = drawable.allocation();
  Color fg = 0xff000000;
  double lthickness = 2.25;
  painter.draw_simple_line (area.x + 15, area.y + 15, area.x + 35, area.y + 35, lthickness, fg);
  painter.draw_simple_line (area.x + 35, area.y + 35, area.x + 50, area.y + 20, lthickness, fg);
  painter.draw_simple_line (area.x + 50, area.y + 20, area.x + 75, area.y + 90, lthickness, fg);
  painter.draw_simple_line (area.x + 75, area.y + 90, area.x + 230, area.y + 93, lthickness, fg);
  painter.draw_simple_line (area.x + 75, area.y + 120, area.x + 230, area.y + 110, lthickness * 0.5, fg);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its gtk backend */
  app.init_with_x11 (&argc, &argv, "Graphics");
  /* initialization acquired global Rapicorn mutex */

  /* load GUI definition file, relative to argv[0] */
  app.auto_load ("RapicornTest", "graphics.xml", argv[0]);

  /* create root item */
  Window &window = *app.create_window ("graphics-dialog");

  /* hook up drawable test */
  Root &root = window.root();
  Drawable &drawable = root.interface<Drawable&>();
  drawable.sig_draw += slot (&drawable_draw, drawable);

  window.show();

  app.execute_loops();

  return 0;
}

} // anon
