/* Rapicorn
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
#include "arrangementimpl.hh"
#include "factory.hh"

namespace Rapicorn {

const PropertyList&
Arrangement::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Arrangement, origin,         _("Origin"),            _("The coordinate origin to be displayed by the arrangement"), Point (-MAXDOUBLE, -MAXDOUBLE), Point (+MAXDOUBLE, +MAXDOUBLE), "rw"),
    MakeProperty (Arrangement, origin_hanchor, _("Horizontal Anchor"), _("Horizontal position of the origin within arrangement, 0=left, 1=right"), 0, 1, 0.1, "rw"),
    MakeProperty (Arrangement, origin_vanchor, _("Vertical Anchor"),   _("Vertical position of the origin within arrangement, 0=bottom, 1=top"), 0, 1, 0.1, "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::list_properties());
  return property_list;
}

ArrangementImpl::ArrangementImpl() :
  m_origin (0, 0),
  m_origin_hanchor (0.5),
  m_origin_vanchor (0.5),
  m_child_area()
{}

ArrangementImpl::~ArrangementImpl()
{}

Allocation
ArrangementImpl::local_child_allocation (ItemImpl &child,
                                         double    width,
                                         double    height)
{
  Requisition requisition = child.requisition();
  const PackInfo &pi = child.pack_info();
  Allocation area;
  area.width = iceil (requisition.width);
  area.height = iceil (requisition.height);
  double origin_x = width * m_origin_hanchor - m_origin.x;
  double origin_y = height * m_origin_vanchor - m_origin.y;
  area.x = iround (origin_x + pi.hposition - pi.halign * area.width);
  area.y = iround (origin_y + pi.vposition - pi.valign * area.height);
  if (width > 0 && child.hexpand())
    {
      area.x = 0;
      area.width = iround (width);
    }
  if (height > 0 && child.vexpand())
    {
      area.y = 0;
      area.height = iround (height);
    }
  return area;
}

Rect
ArrangementImpl::child_area ()
{
  Rect rect; /* empty */
  Allocation parea = allocation();
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      Allocation area = local_child_allocation (child, parea.width, parea.height);
      rect.rect_union (Rect (Point (area.x, area.y), 1, 1));
      rect.rect_union (Rect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
    }
  return rect;
}

void
ArrangementImpl::size_request (Requisition &requisition)
{
  Rect rect; /* empty */
  bool chspread = false, cvspread = false, need_origin = false;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      /* size request all children */
      Requisition rq = child.requisition();
      if (!child.allocatable())
        continue;
      chspread |= child.hspread();
      cvspread |= child.vspread();
      Allocation area = local_child_allocation (child, 0, 0);
      rect.rect_union (Rect (Point (area.x, area.y), 1, 1));
      rect.rect_union (Rect (Point (area.x + area.width - 1, area.y + area.height - 1), 1, 1));
      need_origin = true;
    }
  if (need_origin)
    rect.rect_union (Rect (Point (0, 0), 1, 1));
  double side1, side2;
  /* calculate size requisition in the west and east of anchor */
  side1 = MAX (-rect.x, 0);
  side2 = MAX (0, rect.upper_x());
  /* expand proportionally */
  side1 = side1 * 1.0 / CLAMP (origin_hanchor(), 0.5, 1);
  side2 = side2 * 1.0 / (1 - CLAMP (origin_hanchor(), 0, 0.5));
  requisition.width = MAX (side1, side2);
  /* calculate size requisition in the south and north of anchor */
  side1 = MAX (-rect.y, 0);
  side2 = MAX (0, rect.upper_y());
  /* expand proportionally */
  side1 = side1 * 1.0 / CLAMP (origin_vanchor(), 0.5, 1);
  side2 = side2 * 1.0 / (1 - CLAMP (origin_vanchor(), 0, 0.5));
  requisition.height = MAX (side1, side2);
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
ArrangementImpl::size_allocate (Allocation area)
{
  allocation (area);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      if (!child.allocatable())
        continue;
      Allocation carea = local_child_allocation (child, area.width, area.height);
      carea.x += area.x;
      carea.y += area.y;
      /* allocate child */
      child.set_allocation (carea);
    }
}

static const ItemFactory<ArrangementImpl> arrangement_factory ("Rapicorn::Factory::Arrangement");

} // Rapicorn
