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
#include "buttons.hh"
#include "containerimpl.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

ButtonController::ButtonController() :
  sig_check_activate (*this),
  sig_activate (*this)
{}

class ButtonControllerImpl : public virtual ButtonController {
  ButtonView *m_view;
  uint m_button;
  virtual void
  set_item (Item &item)
  {
    m_view = dynamic_cast<ButtonView*> (&item);
  }
  virtual Item&
  get_item ()
  {
    return *m_view;
  }
protected:
  virtual void
  unset_model()
  { // FIXME
  }
public:
  virtual void
  set_model (Activatable  &activatable)
  { // FIXME
  }
  virtual void
  update ()
  { // FIXME
  }
  ButtonControllerImpl () :
    m_view (NULL),
    m_button (0)
  {}
  void
  set_view (ButtonView &bview)
  {
    set_item (dynamic_cast<Item&> (bview));
  }
  virtual bool
  handle_event (const Event &event)
  {
    ButtonView &view = *m_view;
    bool handled = false, proper_release = false;
    switch (event.type)
      {
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
        if (!m_button && event.button == 1)
          {
            m_button = event.button;
            view.impressed (true);
            view.root()->add_grab (view);
            handled = true;
          }
        break;
      case BUTTON_RELEASE:
      case BUTTON_2RELEASE:
      case BUTTON_3RELEASE:
        proper_release = true;
      case BUTTON_CANCELED:
        if (m_button == event.button)
          {
            bool inbutton = view.prelight();
            view.root()->remove_grab (view);
            m_button = 0;
            view.impressed (false);
            handled = true;
            if (proper_release && inbutton)
              diag ("button-clicked: impressed=%d (event.button=%d)", view.impressed(), event.button);
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
};

class ButtonViewImpl : public virtual SingleContainerImpl, public virtual ButtonView {
public:
  ButtonViewImpl()
  {
    set_controller (*new ButtonControllerImpl); // FIXME: leak
  }
  void
  set_model (Activatable &activatable)
  {
    // FIXME
  }
  void
  set_controller (ButtonController &bcontroller)
  {
    controller (bcontroller);
  }
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
    };
    static const PropertyList property_list (properties, Container::list_properties());
    return property_list;
  }
};
static const ItemFactory<ButtonViewImpl> button_view_factory ("Rapicorn::ButtonView");

} // Rapicorn
