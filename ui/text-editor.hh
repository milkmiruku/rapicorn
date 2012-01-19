/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_TEXT_EDITOR_HH__
#define __RAPICORN_TEXT_EDITOR_HH__

#include <ui/container.hh>

namespace Rapicorn {
namespace Text {

struct ParaState {
  AlignType     align;
  EllipsizeType ellipsize;
  double        line_spacing;
  double        indent;
  String        font_family;
  double        font_size; /* absolute font size */
  explicit      ParaState();
};

struct AttrState {
  String   font_family;
  double   font_scale; /* relative font size */
  bool     bold;
  bool     italic;
  bool     underline;
  bool     small_caps;
  bool     strike_through;
  Color    foreground;
  Color    background;
  explicit AttrState();
};

class Editor : public virtual ContainerImpl {
protected:
  virtual const PropertyList& list_properties();
public:
  /* Text::Editor::Client */
  class Client {
  protected:
    const PropertyList& client_list_properties();
    virtual String      save_markup  () const = 0;
    virtual void        load_markup  (const String    &markup) = 0;
  public:
    virtual            ~Client ();
    virtual const char* peek_text    (int *byte_length) = 0;
    virtual ParaState   para_state   () const = 0;
    virtual void        para_state   (const ParaState &pstate) = 0;
    virtual AttrState   attr_state   () const = 0;
    virtual void        attr_state   (const AttrState &astate) = 0;
    /* properties */
    virtual String      markup_text  () const;
    virtual void        markup_text  (const String &markup);
    virtual String      plain_text   () const;
    virtual void        plain_text   (const String &ptext);
    virtual TextMode    text_mode    () const = 0;
    virtual void        text_mode    (TextMode      text_mode) = 0;
    /* size negotiation */
    virtual double      text_requisition (uint          n_chars,
                                          uint          n_digits) = 0;
    /* mark handling */
    virtual int         mark            () const = 0; /* byte_index */
    virtual void        mark            (int              byte_index) = 0;
    virtual bool        mark_at_end     () const = 0;
    virtual bool        mark_to_coord   (double x, double y) = 0;
    virtual void        step_mark       (int              visual_direction) = 0;
    virtual void        mark2cursor     () = 0;
    virtual void        hide_cursor     () = 0;
    virtual void        mark_delete     (uint             n_utf8_chars) = 0;
    virtual void        mark_insert     (String           utf8string,
                                         const AttrState *astate = NULL) = 0;
  };
public:
  /* Text::Editor */
  virtual void          text           (const String &text) = 0;
  virtual String        text           () const = 0;
  virtual TextMode      text_mode      () const = 0;
  virtual void          text_mode      (TextMode      text_mode) = 0;
  virtual String        markup_text    () const = 0;
  virtual void          markup_text    (const String &markup) = 0;
  virtual String        plain_text     () const = 0;
  virtual void          plain_text     (const String &ptext) = 0;
  virtual uint          request_chars  () const = 0;
  virtual void          request_chars  (uint nc) = 0;
  virtual uint          request_digits () const = 0;
  virtual void          request_digits (uint nd) = 0;
};

} // Text
} // Rapicorn

#endif  /* __RAPICORN_TEXT_EDITOR_HH__ */
