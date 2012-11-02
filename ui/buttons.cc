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
#include "buttons.hh"
#include "container.hh"
#include "painter.hh"
#include "factory.hh"
#include "window.hh"
#include <unistd.h>

namespace Rapicorn {

ButtonAreaImpl::ButtonAreaImpl() :
  m_button (0), m_repeater (0),
  m_click_type (CLICK_ON_RELEASE),
  m_focus_frame (NULL),
  sig_check_activate (*this),
  sig_activate (*this)
{}

const PropertyList&
ButtonAreaImpl::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ButtonAreaImpl, on_click,   _("On CLick"),   _("Command on button1 click"), "rw"),
    MakeProperty (ButtonAreaImpl, on_click2,  _("On CLick2"),  _("Command on button2 click"), "rw"),
    MakeProperty (ButtonAreaImpl, on_click3,  _("On CLick3"),  _("Command on button3 click"), "rw"),
    MakeProperty (ButtonAreaImpl, click_type, _("CLick Type"), _("Click event generation type"), "rw"),
  };
  static const PropertyList property_list (properties, SingleContainerImpl::list_properties());
  return property_list;
}

void
ButtonAreaImpl::dump_private_data (TestStream &tstream)
{
  tstream.dump_intern ("m_button", m_button);
  tstream.dump_intern ("m_repeater", m_repeater);
}

bool
ButtonAreaImpl::activate_button_command (int button)
{
  if (button >= 1 && button <= 3 && m_on_click[button - 1] != "")
    {
      exec_command (m_on_click[button - 1]);
      return TRUE;
    }
  else
    return FALSE;
}

bool
ButtonAreaImpl::activate_command()
{
  return activate_button_command (m_button);
}

void
ButtonAreaImpl::activate_click (int       button,
                                EventType etype)
{
  bool need_repeat = etype == BUTTON_PRESS && (m_click_type == CLICK_KEY_REPEAT || m_click_type == CLICK_SLOW_REPEAT || m_click_type == CLICK_FAST_REPEAT);
  bool need_click = need_repeat;
  need_click |= etype == BUTTON_PRESS && m_click_type == CLICK_ON_PRESS;
  need_click |= etype == BUTTON_RELEASE && m_click_type == CLICK_ON_RELEASE;
  bool can_exec = need_click && activate_button_command (button);
  need_repeat &= can_exec;
  if (need_repeat && !m_repeater)
    {
      if (m_click_type == CLICK_FAST_REPEAT)
        m_repeater = exec_fast_repeater (slot (*this, &ButtonAreaImpl::activate_command));
      else if (m_click_type == CLICK_SLOW_REPEAT)
        m_repeater = exec_slow_repeater (slot (*this, &ButtonAreaImpl::activate_command));
      else if (m_click_type == CLICK_KEY_REPEAT)
        m_repeater = exec_key_repeater (slot (*this, &ButtonAreaImpl::activate_command));
    }
  else if (!need_repeat && m_repeater)
    {
      remove_exec (m_repeater);
      m_repeater = 0;
    }
}

bool
ButtonAreaImpl::can_focus () const
{
  return m_focus_frame != NULL;
}

bool
ButtonAreaImpl::register_focus_frame (FocusFrame &frame)
{
  if (!m_focus_frame)
    m_focus_frame = &frame;
  return m_focus_frame == &frame;
}

void
ButtonAreaImpl::unregister_focus_frame (FocusFrame &frame)
{
  if (m_focus_frame == &frame)
    m_focus_frame = NULL;
}

void
ButtonAreaImpl::reset (ResetMode mode)
{
  remove_exec (m_repeater);
  m_repeater = 0;
  m_button = 0;
}

bool
ButtonAreaImpl::handle_event (const Event &event)
{
  ButtonAreaImpl &view = *this;
  bool handled = false, proper_release = false;
  switch (event.type)
    {
      const EventButton *bevent;
    case MOUSE_ENTER:
      view.impressed (m_button != 0);
      view.prelight (true);
      break;
    case MOUSE_LEAVE:
      view.prelight (false);
      view.impressed (m_button != 0);
      break;
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (!m_button and bevent->button >= 1 and bevent->button <= 3 and
          m_on_click[bevent->button - 1] != "")
        {
          bool inbutton = view.prelight();
          m_button = bevent->button;
          view.impressed (true);
          if (inbutton && can_focus())
            grab_focus();
          view.get_window()->add_grab (*this);
          activate_click (bevent->button, inbutton ? BUTTON_PRESS : BUTTON_CANCELED);
          handled = true;
        }
      break;
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:
      proper_release = true;
    case BUTTON_CANCELED:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (m_button == bevent->button)
        {
          bool inbutton = view.prelight();
          view.get_window()->remove_grab (*this);
          m_button = 0;
          // activation may recurse here
          activate_click (bevent->button, inbutton && proper_release ? BUTTON_RELEASE : BUTTON_CANCELED);
          view.impressed (false);
          handled = true;
        }
      break;
    case KEY_PRESS:
    case KEY_RELEASE:
      break;
    case FOCUS_IN:
    case FOCUS_OUT:
      break;
    default: break;
    }
  return handled;
}

static const ItemFactory<ButtonAreaImpl> button_area_factory ("Rapicorn::Factory::ButtonArea");

} // Rapicorn
