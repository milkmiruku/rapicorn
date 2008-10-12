/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include "heritage.hh"

namespace Rapicorn {

static Color
adjust_color (Color  color,
              double saturation_factor,
              double value_factor)
{
  double hue, saturation, value;
  color.get_hsv (&hue, &saturation, &value);
  saturation *= saturation_factor;
  value *= value_factor;
  color.set_hsv (hue, MIN (1, saturation), MIN (1, value));
  return color;
}

static Color lighten (Color color) { return adjust_color (color, 1.0, 1.1); }
static Color darken  (Color color) { return adjust_color (color, 1.0, 0.9); }

static Color
state_color (Color     color,
             StateType state,
             ColorType ctype)
{
  /* foreground colors remain stable across certain states */
  bool background_color = true;
  switch (ctype)
    {
    case COLOR_FOREGROUND: case COLOR_FOCUS:
    case COLOR_BLACK:      case COLOR_WHITE:
    case COLOR_RED:        case COLOR_YELLOW:
    case COLOR_GREEN:      case COLOR_CYAN:
    case COLOR_BLUE:       case COLOR_MAGENTA:
      background_color = false;
    default: ;
    }
  Color c = color;
  if (state & STATE_IMPRESSED && background_color)
    c = adjust_color (c, 1.0, 0.8);
  if (state & STATE_INSENSITIVE)
    c = adjust_color (c, 0.8, 1.075);
  if (state & STATE_PRELIGHT && background_color &&
      !(state & STATE_INSENSITIVE))     /* ignore prelight if insensitive */
    c = adjust_color (c, 1.2, 1.0);
  return c;
}

static Color
colorset_normal (StateType state,
                 ColorType color_type)
{
  switch (color_type)
    {
    default:
    case COLOR_NONE:            return 0x00000000;
    case COLOR_FOREGROUND:      return 0xff000000;
    case COLOR_BACKGROUND:      return 0xffdfdcd8;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_normal (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_normal (state, COLOR_BACKGROUND));
    case COLOR_DARK:            return 0xff9f9c98;
    case COLOR_DARK_SHADOW:     return adjust_color (colorset_normal (state, COLOR_DARK), 1, 0.9); // 0xff8f8c88
    case COLOR_DARK_GLINT:      return adjust_color (colorset_normal (state, COLOR_DARK), 1, 1.1); // 0xffafaca8
    case COLOR_LIGHT:           return 0xffdfdcd8;
    case COLOR_LIGHT_SHADOW:    return adjust_color (colorset_normal (state, COLOR_LIGHT), 1, 0.93); // 0xffcfccc8
    case COLOR_LIGHT_GLINT:     return adjust_color (colorset_normal (state, COLOR_LIGHT), 1, 1.07); // 0xffefece8
    case COLOR_FOCUS:           return 0xff000060;
    case COLOR_BLACK:           return 0xff000000;
    case COLOR_WHITE:           return 0xffffffff;
    case COLOR_RED:             return 0xffff0000;
    case COLOR_YELLOW:          return 0xffffff00;
    case COLOR_GREEN:           return 0xff00ff00;
    case COLOR_CYAN:            return 0xff00ffff;
    case COLOR_BLUE:            return 0xff0000ff;
    case COLOR_MAGENTA:         return 0xffff00ff;
    }
}

static Color
colorset_selected (StateType state,
                   ColorType color_type)
{
  switch (color_type)
    {
    case COLOR_FOREGROUND:      return 0xffeeeeee;
    case COLOR_BACKGROUND:      return 0xff111188;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_selected (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_selected (state, COLOR_BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

static Color
colorset_base (StateType state,
               ColorType color_type)
{
  switch (color_type)
    {
    case COLOR_FOREGROUND:      return 0xffeeeeee;
    case COLOR_BACKGROUND:      return 0xff101010;
    case COLOR_BACKGROUND_EVEN: return lighten (colorset_base (state, COLOR_BACKGROUND));
    case COLOR_BACKGROUND_ODD:  return darken  (colorset_base (state, COLOR_BACKGROUND));
    default:                    return colorset_normal (state, color_type);
    }
}

typedef Color (*ColorFunc) (StateType, ColorType);

class ThemeHandle {
  ColorFunc cf;
public:
  explicit ThemeHandle (ColorFunc _cf) :
    cf (_cf)
  {}
  Color get_color (StateType state, ColorType ct) const { return cf (state, ct); }
};

static ThemeHandle theme_normal   (colorset_normal);
static ThemeHandle theme_selected (colorset_selected);
static ThemeHandle theme_base     (colorset_base);

Heritage::Heritage (Item &_item,
                    Root &_root) :
  m_theme (NULL), m_item (_item), m_root (_root)
{
  m_theme = &theme_normal;
}

Heritage&
Heritage::create_modified (const String &theme_name,
                           Item         &_item,
                           Root         &_root)
{
  Heritage *dest;
  if (&_item != &heritage_item() || &_root != &root() || theme_name != "normal")
    {
      dest = new Heritage (_item, _root);
      if (theme_name == "selected")
        dest->m_theme = &theme_selected;
      else if (theme_name == "base")
        dest->m_theme = &theme_base;
    }
  else
    dest = ref (this);
  return *dest;
}

Color
Heritage::get_color (StateType state,
                     ColorType color_type) const
{
  Color c;
  if (m_theme)
    c = m_theme->get_color (state, color_type);
  return state_color (c, state, color_type);
}

Color
Heritage::insensitive_ink (StateType state,
                           Color    *glintp) const
{
  /* constrcut insensitive ink from a mixture of foreground and dark_color */
  Color ink = get_color (state, COLOR_FOREGROUND);
  ink.combine (get_color (state, COLOR_DARK), 0x80);
  Color glint = get_color (state, COLOR_LIGHT);
  if (glintp)
    *glintp = glint;
  return ink;
}

Color
Heritage::resolve_color (const String  &color_name,
                         StateType      state,
                         ColorType      color_type)
{
  EnumTypeColorType ect;
  if (color_name[0] == '#')
    {
      uint32 argb = string_to_int (&color_name[1], 16);
      Color c (argb);
      /* invert alpha (transparency -> opacity) */
      c.alpha (0xff - c.alpha());
      return state_color (c, state, color_type);
    }
  const EnumClass::Value *value = ect.find_first (color_name);
  if (value)
    return get_color (state, ColorType (value->value));
  else
    {
      Color parsed_color = Color::from_name (color_name);
      if (!parsed_color) // transparent black
        return get_color (state, color_type);
      else
        return state_color (parsed_color, state, color_type);
    }
}

} // Rapicorn
