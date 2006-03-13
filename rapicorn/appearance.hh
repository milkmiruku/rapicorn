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
#ifndef __RAPICORN_APPEARANCE_HH__
#define __RAPICORN_APPEARANCE_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class Appearance;

class ColorScheme {
public:
  /* base colors */
  virtual Color make_color      (ColorType ct) const = 0;
  Color         foreground      () const	{ return make_color (COLOR_FOREGROUND); }
  Color         background      () const	{ return make_color (COLOR_BACKGROUND); }
  Color         light_glint     () const	{ return make_color (COLOR_LIGHT_GLINT); }
  Color         light_shadow    () const	{ return make_color (COLOR_LIGHT_SHADOW); }
  Color         dark_glint      () const	{ return make_color (COLOR_DARK_GLINT); }
  Color         dark_shadow     () const	{ return make_color (COLOR_DARK_SHADOW); }
  Color         focus_state     () const        { return make_color (COLOR_FOCUS); }
  Color         default_state   () const        { return make_color (COLOR_DEFAULT); }
  /* color alterations */
  virtual Color generic_color           (Color          source_color) const;
  virtual Color make_light_color        (Color          source_color) const;
  virtual Color make_dark_color         (Color          source_color) const;
  virtual Color make_insensitive_color  (Color          source_color, ColorType ctype = COLOR_NONE) const;
  virtual Color make_prelight_color     (Color          source_color, ColorType ctype = COLOR_NONE) const;
  virtual Color make_impressed_color    (Color          source_color, ColorType ctype = COLOR_NONE) const;
  virtual Color make_focus_state_color  (Color          source_color, ColorType ctype = COLOR_NONE) const;
  virtual Color make_default_state_color(Color          source_color, ColorType ctype = COLOR_NONE) const;
  virtual Color make_state_color        (StateType      state,
                                         Color          color,
                                         ColorType      ctype = COLOR_NONE) const;
  /* standard scheme */
  static const ColorScheme&     default_scheme  ();
};

class Style : public virtual ReferenceCountImpl {
  Appearance &m_appearance;
  String      m_name;
public:
  explicit              Style           (Appearance     &appearance,
                                         const String   &name);
  String                name            () const                        { return m_name; }
  Style*                create_style    (const String   &style_name);
  Style*                create_style    ()                              { return create_style (name()); }
  virtual               ~Style          ();

  /* color schemes */
  class ColorSchemeKind;
  static const ColorSchemeKind &STANDARD;
  static const ColorSchemeKind &SELECTED;
  static const ColorSchemeKind &INPUT;
  const ColorScheme&            color_scheme    (const ColorSchemeKind  &kind) const;
  /* convenience */
  Color                         standard_color  (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const;
  Color                         selected_color  (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const;
  Color                         input_color     (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const;
};

class Appearance : public virtual ReferenceCountImpl {
  const String         &m_name;
  map<String,Style*>    m_styles;
  explicit              Appearance       (const String   &name);
  void                  register_style   (Style          &style);
  void                  unregister_style (Style          &style);
  friend class Style;
public:
  String                name             () const                       { return m_name; }
  virtual Style*        create_style     (const String   &style_name);
  virtual               ~Appearance      ();
  static Appearance*    create_named     (const String   &name);
  static Appearance*    create_default   ();
};

} // Rapicorn

#endif  /* __RAPICORN_APPEARANCE_HH__ */
