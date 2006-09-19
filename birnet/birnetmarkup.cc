/* BirnetMarkup - simple XML parser
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
 *
 * based on glib/gmarkup.c, Copyright 2000, 2003 Red Hat, Inc.
 */
#include "birnetmarkup.hh"
#include "private.hh"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <vector>
#include <stack>

namespace Birnet {
using namespace std;

/* --- unicode hacks --- */
#define _(x) x  // FIXME
inline bool
unichar_isalpha (unichar uc) // FIXME
{
  return (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z');
}
inline int
unichar_to_utf8 (unichar c, // FIXME
                 char   *outbuf)
{
  *outbuf = c;
  return 1;
}
inline unichar
utf8_get_char (const char *p) // FIXME
{
  return *p;
}
inline char*
utf8_next_char (char *p)
{
  static const char utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
  };
  return p + utf8_skip_data[*(uint8*) p];
}
inline const char*
utf8_next_char (const char *c)
{
  char *result = utf8_next_char (const_cast<char*> (c));
  return const_cast<const char*> (result);
}
inline char*
utf8_find_prev_char (char *str,
                     char *p)
{
  for (--p; p >= str; --p)
    {
      if ((*p & 0xc0) != 0x80)
        return p;
    }
  return NULL;
}
inline const char*
utf8_find_prev_char (const char *str,
                     const char *p)
{
  char *result = utf8_find_prev_char (const_cast<char*> (str), const_cast<char*> (p));
  return const_cast<const char*> (result);
}
inline char*
utf8_find_next_char (char *p,
                     char *end)
{
  if (*p)
    {
      ++p;
      if (end)
        while (p < end && (*p & 0xc0) == 0x80)
          ++p;
      else
        while ((*p & 0xc0) == 0x80)
          ++p;
    }
  return (p == end) ? NULL : p;
}
inline const char*
utf8_find_next_char (const char *p,
                     const char *end)
{
  char *result = utf8_find_next_char (const_cast<char*> (p), const_cast<char*> (end));
  return const_cast<const char*> (result);
}
inline bool
utf8_validate (const char  *str, // FIXME
               ssize_t      max_len,
               const char **end)
{
  const char *p;
  for (p = str; (max_len < 0 || (p - str) < max_len) && *p; p++)
    {
      if (*(uint8*) p < 128)
        /* done */;
      else
        break;
    }
  if (end)
    *end = p;
  if ((max_len >= 0 && p != str + max_len) ||
      (max_len < 0 && *p != '\0'))
    return false;
  else
    return true;
}

static inline bool
str_has_prefix (const String &str,
                const String &prefix)
{
  uint sl = str.size();
  uint pl = prefix.size();
  if (pl <= sl && strncmp (str.c_str(), prefix.c_str(), pl) == 0)
    return true;
  return false;
}

static inline bool
str_has_suffix (const String &str,
                const String &suffix)
{
  uint sl = str.size();
  uint xl = suffix.size();
  if (xl <= sl && strcmp (str.c_str() + sl - xl, suffix.c_str()) == 0)
    return true;
  return false;
}


/* --- usefull type aliases --- */
typedef MarkupParser::Error     MarkupError;
typedef MarkupParser::ErrorType MarkupErrorType;
typedef MarkupParser::Context   MarkupParserContext;

typedef enum
{
  STATE_START,
  STATE_AFTER_OPEN_ANGLE,
  STATE_AFTER_CLOSE_ANGLE,
  STATE_AFTER_ELISION_SLASH, /* the slash that obviates need for end element */
  STATE_INSIDE_OPEN_TAG_NAME,
  STATE_INSIDE_ATTRIBUTE_NAME,
  STATE_AFTER_ATTRIBUTE_NAME,
  STATE_BETWEEN_ATTRIBUTES,
  STATE_AFTER_ATTRIBUTE_EQUALS_SIGN,
  STATE_INSIDE_ATTRIBUTE_VALUE_SQ,
  STATE_INSIDE_ATTRIBUTE_VALUE_DQ,
  STATE_INSIDE_TEXT,
  STATE_AFTER_CLOSE_TAG_SLASH,
  STATE_INSIDE_CLOSE_TAG_NAME,
  STATE_AFTER_CLOSE_TAG_NAME,
  STATE_INSIDE_PASSTHROUGH,
  STATE_ERROR
} GMarkupParseState;

struct MarkupParser::Context {
  MarkupParser *parser;
  
  int  line_number;
  int  char_number;
  
  /* A piece of character data or an element that
   * hasn't "ended" yet so we haven't yet called
   * the callback for it.
   */
  String partial_chunk;
  
  GMarkupParseState state;
  stack<String>  tag_stack;
  vector<string> attr_names;
  vector<string> attr_values;
  
  const char  *current_text;
  ssize_t       current_text_len;      
  const char  *current_text_end;
  
  String leftover_char_portion;
  
  /* used to save the start of the last interesting thingy */
  const char  *start;
  
  const char  *iter;
  
  uint  document_empty : 1;
  uint  parsing : 1;
  uint  line_number_after_newline : 1;
  int   balance;
};

MarkupParser::MarkupParser (const String &input_name) :
  context (NULL), m_input_name (input_name), m_recap_depth (0), m_recap_outer (true)
{
  context = new MarkupParserContext;
  
  context->parser = this;
  context->line_number = 1;
  context->line_number_after_newline = 0;
  context->char_number = 1;
  context->state = STATE_START;
  context->current_text = NULL;
  context->current_text_len = -1;
  context->current_text_end = NULL;
  context->start = NULL;
  context->iter = NULL;
  context->document_empty = true;
  context->parsing = false;
  context->balance = 0;
}

MarkupParser*
MarkupParser::create_parser (const String &input_name)
{
  MarkupParser *parser = new MarkupParser (input_name);
  return parser;
}

MarkupParser::~MarkupParser ()
{
  return_if_fail (context != NULL);
  delete context;
  context = NULL;
}

static void
mark_error (MarkupParserContext *context,
            const MarkupError   &error)
{
  context->state = STATE_ERROR;
  context->parser->error (error);
}

static void BIRNET_PRINTF (4, 5)
  set_error (MarkupParserContext *context,
             MarkupError         &error,
             MarkupErrorType      code,
             const char          *format,
             ...)
{
  va_list args;
  va_start (args, format);
  String msg;
  try {
    msg = string_vprintf (format, args);
  } catch (...) {
    msg = "out of memory";
  }
  va_end (args);
  error.message = msg;
  error.code = code;
  error.line_number = context->line_number - context->line_number_after_newline;
  error.char_number = context->char_number;
  mark_error (context, error);
}


/* To make these faster, we first use the ascii-only tests, then check
 * for the usual non-alnum name-end chars, and only then call the
 * expensive unicode stuff. Nobody uses non-ascii in XML tag/attribute
 * names, so this is a reasonable hack that virtually always avoids
 * the guniprop call.
 */
#define IS_COMMON_NAME_END_CHAR(c) \
  ((c) == '=' || (c) == '/' || (c) == '>' || (c) == ' ')

static bool    
is_name_start_char (const char  *p)
{
  if ((*p >= 'A' && *p <= 'Z') ||
      (*p >= 'a' && *p <= 'z') ||
      (!IS_COMMON_NAME_END_CHAR (*p) &&
       (*p == '_' || 
	*p == ':' ||
	unichar_isalpha (utf8_get_char (p)))))
    return true;
  else
    return false;
}

static bool    
is_name_char (const char  *p)
{
  if ((*p >= 'A' && *p <= 'Z') ||
      (*p >= 'a' && *p <= 'z') ||
      (*p >= '0' && *p <= '9') ||
      (!IS_COMMON_NAME_END_CHAR (*p) &&
       (*p == '.' || 
	*p == '-' ||
	*p == '_' ||
	*p == ':' ||
	unichar_isalpha (utf8_get_char (p)))))
    return true;
  else
    return false;
}


static char *
char_str (unichar  c,
          char    *buf)
{
  memset (buf, 0, 8);
  unichar_to_utf8 (c, buf);
  return buf;
}

static char *
utf8_str (const char  *utf8,
          char        *buf)
{
  char_str (utf8_get_char (utf8), buf);
  return buf;
}

static void
set_unescape_error (MarkupParserContext *context,
                    MarkupError         &error,
                    const char          *remaining_text,
                    const char          *remaining_text_end,
                    MarkupErrorType      code,
                    const char          *format,
                    ...)
{
  int  remaining_newlines = 0;
  const char  *p = remaining_text;
  while (p != remaining_text_end)
    {
      if (*p == '\n')
        ++remaining_newlines;
      ++p;
    }
  
  va_list args;
  va_start (args, format);
  String msg;
  try {
    msg = string_vprintf (format, args);
  } catch (...) {
    msg = "out of memory";
  }
  va_end (args);
  
  error.message = msg;
  error.code = code;
  error.line_number = context->line_number - context->line_number_after_newline - remaining_newlines;
  error.char_number = 0; // context->char_number;
  mark_error (context, error);
}

typedef enum {
  USTATE_INSIDE_TEXT,
  USTATE_AFTER_AMPERSAND,
  USTATE_INSIDE_ENTITY_NAME,
  USTATE_AFTER_CHARREF_HASH
} UnescapeState;

struct UnescapeContext {
  MarkupParserContext *context;
  String               str;
  UnescapeState        state;
  const char          *text;
  const char          *text_end;
  const char          *entity_start;
};

static const char*
unescape_text_state_inside_text (UnescapeContext *ucontext,
                                 const char      *p,
                                 MarkupError     &error)
{
  const char  *start;
  bool     normalize_attribute;
  
  if (ucontext->context->state == STATE_INSIDE_ATTRIBUTE_VALUE_SQ ||
      ucontext->context->state == STATE_INSIDE_ATTRIBUTE_VALUE_DQ)
    normalize_attribute = true;
  else
    normalize_attribute = false;
  
  start = p;
  
  while (p != ucontext->text_end)
    {
      if (*p == '&')
        {
          break;
        }
      else if (normalize_attribute && (*p == '\t' || *p == '\n'))
        {
          ucontext->str.append (start, p - start);
          ucontext->str.append (" ");
          p = utf8_next_char (p);
          start = p;
        }
      else if (*p == '\r')
        {
          ucontext->str.append (start, p - start);
          ucontext->str.append (normalize_attribute ? " " : "\n");
          p = utf8_next_char (p);
          if (p != ucontext->text_end && *p == '\n')
            p = utf8_next_char (p);
          start = p;
        }
      else
        p = utf8_next_char (p);
    }
  
  if (p != start)
    ucontext->str.append (start, p - start);
  
  if (p != ucontext->text_end && *p == '&')
    {
      p = utf8_next_char (p);
      ucontext->state = USTATE_AFTER_AMPERSAND;
    }
  
  return p;
}

static const char *
unescape_text_state_after_ampersand (UnescapeContext *ucontext,
                                     const char      *p,
                                     MarkupError     &error)
{
  ucontext->entity_start = NULL;
  
  if (*p == '#')
    {
      p = utf8_next_char (p);
      
      ucontext->entity_start = p;
      ucontext->state = USTATE_AFTER_CHARREF_HASH;
    }
  else if (!is_name_start_char (p))
    {
      if (*p == ';')
        {
          set_unescape_error (ucontext->context, error,
                              p, ucontext->text_end,
                              MarkupParser::PARSE_ERROR,
                              _("Empty entity '&;' seen; valid "
                                "entities are: &amp; &quot; &lt; &gt; &apos;"));
        }
      else
        {
          char  buf[8];
          
          set_unescape_error (ucontext->context, error,
                              p, ucontext->text_end,
                              MarkupParser::PARSE_ERROR,
                              _("Character '%s' is not valid at "
                                "the start of an entity name; "
                                "the & character begins an entity; "
                                "if this ampersand isn't supposed "
                                "to be an entity, escape it as "
                                "&amp;"),
                              utf8_str (p, buf));
        }
    }
  else
    {
      ucontext->entity_start = p;
      ucontext->state = USTATE_INSIDE_ENTITY_NAME;
    }
  
  return p;
}

static const char *
unescape_text_state_inside_entity_name (UnescapeContext *ucontext,
                                        const char      *p,
                                        MarkupError     &error)
{
  while (p != ucontext->text_end)
    {
      if (*p == ';')
        break;
      else if (!is_name_char (p))
        {
          char  ubuf[8];
          
          set_unescape_error (ucontext->context, error,
                              p, ucontext->text_end,
                              MarkupParser::PARSE_ERROR,
                              _("Character '%s' is not valid "
                                "inside an entity name"),
                              utf8_str (p, ubuf));
          break;
        }
      
      p = utf8_next_char (p);
    }
  
  if (ucontext->context->state != STATE_ERROR)
    {
      if (p != ucontext->text_end)
        {
	  int  len = p - ucontext->entity_start;
          
          /* move to after semicolon */
          p = utf8_next_char (p);
          ucontext->state = USTATE_INSIDE_TEXT;
          
          if (strncmp (ucontext->entity_start, "lt", len) == 0)
            ucontext->str.append ("<");
          else if (strncmp (ucontext->entity_start, "gt", len) == 0)
            ucontext->str.append (">");
          else if (strncmp (ucontext->entity_start, "amp", len) == 0)
            ucontext->str.append ("&");
          else if (strncmp (ucontext->entity_start, "quot", len) == 0)
            ucontext->str.append ("\"");
          else if (strncmp (ucontext->entity_start, "apos", len) == 0)
            ucontext->str.append ("'");
          else
            {
	      String name;
              name.append (ucontext->entity_start, len);
              set_unescape_error (ucontext->context, error,
                                  p, ucontext->text_end,
                                  MarkupParser::PARSE_ERROR,
                                  _("Entity name '%s' is not known"),
                                  name.c_str());
            }
        }
      else
        {
          set_unescape_error (ucontext->context, error,
                              /* give line number of the & */
                              ucontext->entity_start, ucontext->text_end,
                              MarkupParser::PARSE_ERROR,
                              _("Entity did not end with a semicolon; "
                                "most likely you used an ampersand "
                                "character without intending to start "
                                "an entity - escape ampersand as &amp;"));
        }
    }
#undef MAX_ENT_LEN
  
  return p;
}

static const char *
unescape_text_state_after_charref_hash (UnescapeContext *ucontext,
                                        const char      *p,
                                        MarkupError     &error)
{
  bool     is_hex = false;
  const char *start;
  
  start = ucontext->entity_start;
  
  if (*p == 'x')
    {
      is_hex = true;
      p = utf8_next_char (p);
      start = p;
    }
  
  while (p != ucontext->text_end && *p != ';')
    p = utf8_next_char (p);
  
  if (p != ucontext->text_end)
    {
      assert (*p == ';');
      
      /* digit is between start and p */
      
      if (start != p)
        {
          char  *end = NULL;
          uint l;
          
          errno = 0;
          if (is_hex)
            l = strtoul (start, &end, 16);
          else
            l = strtoul (start, &end, 10);
          
          if (end != p || errno != 0)
            {
              set_unescape_error (ucontext->context, error,
                                  start, ucontext->text_end,
                                  MarkupParser::PARSE_ERROR,
                                  _("Failed to parse '%-.*s', which "
                                    "should have been a digit "
                                    "inside a character reference "
                                    "(&#234; for example) - perhaps "
                                    "the digit is too large"),
                                  p - start, start);
            }
          else
            {
              /* characters XML permits */
              if (l == 0x9 ||
                  l == 0xA ||
                  l == 0xD ||
                  (l >= 0x20 && l <= 0xD7FF) ||
                  (l >= 0xE000 && l <= 0xFFFD) ||
                  (l >= 0x10000 && l <= 0x10FFFF))
                {
                  char  buf[8];
                  ucontext->str.append (char_str (l, buf));
                }
              else
                {
                  set_unescape_error (ucontext->context, error,
                                      start, ucontext->text_end,
                                      MarkupParser::PARSE_ERROR,
                                      _("Character reference '%-.*s' does not "
					"encode a permitted character"),
                                      p - start, start);
                }
            }
          
          /* Move to next state */
          p = utf8_next_char (p); /* past semicolon */
          ucontext->state = USTATE_INSIDE_TEXT;
        }
      else
        {
          set_unescape_error (ucontext->context, error,
                              start, ucontext->text_end,
                              MarkupParser::PARSE_ERROR,
                              _("Empty character reference; "
                                "should include a digit such as "
                                "&#454;"));
        }
    }
  else
    {
      set_unescape_error (ucontext->context, error,
                          start, ucontext->text_end,
                          MarkupParser::PARSE_ERROR,
                          _("Character reference did not end with a "
                            "semicolon; "
                            "most likely you used an ampersand "
                            "character without intending to start "
                            "an entity - escape ampersand as &amp;"));
    }
  
  return p;
}

static bool    
unescape_text (MarkupParserContext *context,
               const char          *text,
               const char          *text_end,
               String              *unescaped,
               MarkupError         &error)
{
  UnescapeContext ucontext;
  const char  *p;
  
  ucontext.context = context;
  ucontext.text = text;
  ucontext.text_end = text_end;
  ucontext.entity_start = NULL;
  
  ucontext.state = USTATE_INSIDE_TEXT;
  p = text;
  
  while (p != text_end && context->state != STATE_ERROR)
    {
      assert (p < text_end);
      
      switch (ucontext.state)
        {
        case USTATE_INSIDE_TEXT:
          {
            p = unescape_text_state_inside_text (&ucontext,
                                                 p,
                                                 error);
          }
          break;
          
        case USTATE_AFTER_AMPERSAND:
          {
            p = unescape_text_state_after_ampersand (&ucontext,
                                                     p,
                                                     error);
          }
          break;
          
          
        case USTATE_INSIDE_ENTITY_NAME:
          {
            p = unescape_text_state_inside_entity_name (&ucontext,
                                                        p,
                                                        error);
          }
          break;
          
        case USTATE_AFTER_CHARREF_HASH:
          {
            p = unescape_text_state_after_charref_hash (&ucontext,
                                                        p,
                                                        error);
          }
          break;
          
        default:
          assert_not_reached ();
          break;
        }
    }
  
  if (context->state != STATE_ERROR) 
    {
      switch (ucontext.state) 
	{
	case USTATE_INSIDE_TEXT:
	  break;
	case USTATE_AFTER_AMPERSAND:
	case USTATE_INSIDE_ENTITY_NAME:
	  set_unescape_error (context, error,
			      NULL, NULL,
			      MarkupParser::PARSE_ERROR,
			      _("Unfinished entity reference"));
	  break;
	case USTATE_AFTER_CHARREF_HASH:
	  set_unescape_error (context, error,
			      NULL, NULL,
			      MarkupParser::PARSE_ERROR,
			      _("Unfinished character reference"));
	  break;
	}
    }
  
  if (context->state == STATE_ERROR)
    {
      *unescaped = "";
      return false;
    }
  else
    {
      *unescaped = ucontext.str;
      return true;
    }
}

static inline bool    
advance_char (MarkupParserContext *context)
{  
  context->iter = utf8_next_char (context->iter);
  context->char_number += 1;
  context->line_number_after_newline = 0;

  if (context->iter == context->current_text_end)
    {
      return false;
    }
  else if (*context->iter == '\n')
    {
      context->line_number += 1;
      context->char_number = 1;
      context->line_number_after_newline = 1;
    }
  
  return true;
}

static inline bool    
xml_isspace (char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void
skip_spaces (MarkupParserContext *context)
{
  do
    {
      if (!xml_isspace (*context->iter))
        return;
    }
  while (advance_char (context));
}

static void
advance_to_name_end (MarkupParserContext *context)
{
  do
    {
      if (!is_name_char (context->iter))
        return;
    }
  while (advance_char (context));
}

static void
add_to_partial (MarkupParserContext *context,
                const char          *text_start,
                const char          *text_end)
{
  if (text_start != text_end)
    context->partial_chunk.append (text_start, text_end - text_start);
}

static void
truncate_partial (MarkupParserContext *context)
{
  context->partial_chunk = "";
}

static const char *
current_element (MarkupParserContext *context)
{
  assert (!context->tag_stack.empty());
  return context->tag_stack.top().c_str();
}

static String&
last_attribute (MarkupParserContext *context)
{
  assert (context->attr_names.size() > 0);
  return context->attr_names[context->attr_names.size() - 1];
}

static String&
last_value (MarkupParserContext *context)
{
  assert (context->attr_values.size() > 0);
  return context->attr_values[context->attr_values.size() - 1];
}

static void
find_current_text_end (MarkupParserContext *context)
{
  /* This function must be safe (non-segfaulting) on invalid UTF8.
   * It assumes the string starts with a character start
   */
  const char  *end = context->current_text + context->current_text_len;
  const char  *p;
  const char  *next;
  
  assert (context->current_text_len > 0);
  
  p = utf8_find_prev_char (context->current_text, end);
  
  assert (p != NULL); /* since current_text was a char start */
  
  /* p is now the start of the last character or character portion. */
  assert (p != end);
  next = utf8_next_char (p); /* this only touches *p, nothing beyond */
  
  if (next == end)
    {
      /* whole character */
      context->current_text_end = end;
    }
  else
    {
      /* portion */
      context->leftover_char_portion = "";
      context->leftover_char_portion.append (p, end - p);
      context->current_text_len -= (end - p);
      context->current_text_end = p;
    }
}


static void
add_attribute (MarkupParserContext *context, const char *name)
{
  context->attr_names.push_back (name);
  context->attr_values.push_back ("");
}
bool    
MarkupParser::parse (const char          *text,
                     ssize_t              text_len,
                     MarkupError         *errorp)
{
  Error dummy, &error = errorp ? *errorp : dummy;
  const char  *first_invalid;
  
  return_val_if_fail (context != NULL, false);
  return_val_if_fail (text != NULL, false);
  return_val_if_fail (context->state != STATE_ERROR, false);
  return_val_if_fail (!context->parsing, false);
  
  if (text_len < 0)
    text_len = strlen (text);
  
  if (text_len == 0)
    return true;
  
  context->parsing = true;
  
  if (context->leftover_char_portion.size())
    {
      const char  *first_char;
      
      if ((*text & 0xc0) != 0x80)
        first_char = text;
      else
        first_char = utf8_find_next_char (text, text + text_len);
      
      if (first_char)
        {
          /* leftover_char_portion was completed. Parse it. */
          String portion = context->leftover_char_portion;
          portion.append (text, first_char - text);
          
          /* hacks to allow recursion */
          context->parsing = false;
          context->leftover_char_portion = "";
          if (!parse (portion.c_str(), portion.size(), &error))
            assert (context->state == STATE_ERROR);
          context->parsing = true;
          
          /* Skip the fraction of char that was in this text */
          text_len -= (first_char - text);
          text = first_char;
        }
      else
        {
          /* another little chunk of the leftover char; geez
           * someone is inefficient.
           */
          context->leftover_char_portion.append (text, text_len);
          if (context->leftover_char_portion.size() > 7)
            {
              /* The leftover char portion is too big to be
               * a UTF-8 character
               */
              set_error (context, error, MarkupParser::BAD_UTF8, _("Invalid UTF-8 encoded text"));
            }
          goto finished;
        }
    }
  
  context->current_text = text;
  context->current_text_len = text_len;
  context->iter = context->current_text;
  context->start = context->iter;
  
  /* Nothing left after finishing the leftover char, or nothing
   * passed in to begin with.
   */
  if (context->current_text_len == 0)
    goto finished;
  
  /* find_current_text_end () assumes the string starts at
   * a character start, so we need to validate at least
   * that much. It doesn't assume any following bytes
   * are valid.
   */
  if ((*context->current_text & 0xc0) == 0x80) /* not a char start */
    {
      set_error (context, error, MarkupParser::BAD_UTF8, _("Invalid UTF-8 encoded text"));
      goto finished;
    }
  
  /* Initialize context->current_text_end, possibly adjusting
   * current_text_len, and add any leftover char portion
   */
  find_current_text_end (context);
  
  /* Validate UTF8 (must be done after we find the end, since
   * we could have a trailing incomplete char)
   */
  if (!utf8_validate (context->current_text,
                      context->current_text_len,
                      &first_invalid))
    {
      int  newlines = 0;
      const char  *p;
      p = context->current_text;
      while (p != context->current_text_end)
        {
          if (*p == '\n')
            ++newlines;
          ++p;
        }
      
      context->line_number += newlines;
      
      set_error (context, error, MarkupParser::BAD_UTF8, _("Invalid UTF-8 encoded text"));
      goto finished;
    }
  
  while (context->iter != context->current_text_end)
    {
      switch (context->state)
        {
        case STATE_START:
          /* Possible next state: AFTER_OPEN_ANGLE */
          
          assert (context->tag_stack.empty());
          
          /* whitespace is ignored outside of any elements */
          skip_spaces (context);
          
          if (context->iter != context->current_text_end)
            {
              if (*context->iter == '<')
                {
                  /* Move after the open angle */
                  advance_char (context);
                  
                  context->state = STATE_AFTER_OPEN_ANGLE;
                  
                  /* this could start a passthrough */
                  context->start = context->iter;
                  
                  /* document is now non-empty */
                  context->document_empty = false;
                }
              else
                {
                  set_error (context, error, MarkupParser::PARSE_ERROR, _("Document must begin with an element (e.g. <book>)"));
                }
            }
          break;
          
        case STATE_AFTER_OPEN_ANGLE:
          /* Possible next states: INSIDE_OPEN_TAG_NAME,
           *  AFTER_CLOSE_TAG_SLASH, INSIDE_PASSTHROUGH
           */
          if (*context->iter == '?' ||
              *context->iter == '!')
            {
              /* include < in the passthrough */
              const char  *openangle = "<";
              add_to_partial (context, openangle, openangle + 1);
              context->start = context->iter;
	      context->balance = 1;
              context->state = STATE_INSIDE_PASSTHROUGH;
            }
          else if (*context->iter == '/')
            {
              /* move after it */
              advance_char (context);
              
              context->state = STATE_AFTER_CLOSE_TAG_SLASH;
            }
          else if (is_name_start_char (context->iter))
            {
              context->state = STATE_INSIDE_OPEN_TAG_NAME;
              
              /* start of tag name */
              context->start = context->iter;
            }
          else
            {
              char  buf[8];
              
              set_error (context, error, MarkupParser::PARSE_ERROR, _("'%s' is not a valid character following "
                                                                      "a '<' character; it may not begin an "
                                                                      "element name"),
                         utf8_str (context->iter, buf));
            }
          break;
          
          /* The AFTER_CLOSE_ANGLE state is actually sort of
           * broken, because it doesn't correspond to a range
           * of characters in the input stream as the others do,
           * and thus makes things harder to conceptualize
           */
        case STATE_AFTER_CLOSE_ANGLE:
          /* Possible next states: INSIDE_TEXT, STATE_START */
          if (context->tag_stack.empty())
            {
              context->start = NULL;
              context->state = STATE_START;
            }
          else
            {
              context->start = context->iter;
              context->state = STATE_INSIDE_TEXT;
            }
          break;
          
        case STATE_AFTER_ELISION_SLASH:
          /* Possible next state: AFTER_CLOSE_ANGLE */
          
          {
            /* We need to pop the tag stack and call the end_element
             * function, since this is the close tag
             */
            assert (!context->tag_stack.empty());
            error.line_number = context->line_number - context->line_number_after_newline;;
            error.char_number = context->char_number;
            if (m_recap_depth)
              recap_end_element (context->tag_stack.top(), error);
            else
              context->parser->end_element (context->tag_stack.top(), error);
            if (error.code)
              mark_error (context, error);
            else
              {
                if (*context->iter == '>')
                  {
                    /* move after the close angle */
                    advance_char (context);
                    context->state = STATE_AFTER_CLOSE_ANGLE;
                  }
                else
                  {
                    char  buf[8];
                    set_error (context, error, MarkupParser::PARSE_ERROR,
                               _("Odd character '%s', expected a '>' character "
                                 "to end the start tag of element '%s'"),
                               utf8_str (context->iter, buf),
                               current_element (context));
                  }
              }
            
            context->tag_stack.pop();
          }
          break;
          
        case STATE_INSIDE_OPEN_TAG_NAME:
          /* Possible next states: BETWEEN_ATTRIBUTES */
          
          /* if there's a partial chunk then it's the first part of the
           * tag name. If there's a context->start then it's the start
           * of the tag name in current_text, the partial chunk goes
           * before that start though.
           */
          advance_to_name_end (context);
          
          if (context->iter == context->current_text_end)
            {
              /* The name hasn't necessarily ended. Merge with
               * partial chunk, leave state unchanged.
               */
              add_to_partial (context, context->start, context->iter);
            }
          else
            {
              /* The name has ended. Combine it with the partial chunk
               * if any; push it on the stack; enter next state.
               */
              add_to_partial (context, context->start, context->iter);
              context->tag_stack.push (context->partial_chunk);
              context->partial_chunk = "";
              
              context->state = STATE_BETWEEN_ATTRIBUTES;
              context->start = NULL;
            }
          break;
          
        case STATE_INSIDE_ATTRIBUTE_NAME:
          /* Possible next states: AFTER_ATTRIBUTE_NAME */
          
          advance_to_name_end (context);
	  add_to_partial (context, context->start, context->iter);
          
          /* read the full name, if we enter the equals sign state
           * then add the attribute to the list (without the value),
           * otherwise store a partial chunk to be prepended later.
           */
          if (context->iter != context->current_text_end)
	    context->state = STATE_AFTER_ATTRIBUTE_NAME;
	  break;
          
	case STATE_AFTER_ATTRIBUTE_NAME:
          /* Possible next states: AFTER_ATTRIBUTE_EQUALS_SIGN */
          
	  skip_spaces (context);
          
	  if (context->iter != context->current_text_end)
	    {
	      /* The name has ended. Combine it with the partial chunk
	       * if any; push it on the stack; enter next state.
	       */
              add_attribute (context, context->partial_chunk.c_str());
              context->partial_chunk = "";
              context->start = NULL;
	      
              if (*context->iter == '=')
                {
                  advance_char (context);
                  context->state = STATE_AFTER_ATTRIBUTE_EQUALS_SIGN;
                }
              else
                {
                  char  buf[8];
                  set_error (context, error, MarkupParser::PARSE_ERROR,
                             _("Odd character '%s', expected a '=' after "
                               "attribute name '%s' of element '%s'"),
                             utf8_str (context->iter, buf),
                             last_attribute (context).c_str(),
                             current_element (context));
		  
                }
            }
          break;
          
        case STATE_BETWEEN_ATTRIBUTES:
          /* Possible next states: AFTER_CLOSE_ANGLE,
           * AFTER_ELISION_SLASH, INSIDE_ATTRIBUTE_NAME
           */
          skip_spaces (context);
          
          if (context->iter != context->current_text_end)
            {
              if (*context->iter == '/')
                {
                  advance_char (context);
                  context->state = STATE_AFTER_ELISION_SLASH;
                }
              else if (*context->iter == '>')
                {
                  
                  advance_char (context);
                  context->state = STATE_AFTER_CLOSE_ANGLE;
                }
              else if (is_name_start_char (context->iter))
                {
                  context->state = STATE_INSIDE_ATTRIBUTE_NAME;
                  /* start of attribute name */
                  context->start = context->iter;
                }
              else
                {
                  char  buf[8];
                  set_error (context, error, MarkupParser::PARSE_ERROR,
                             _("Odd character '%s', expected a '>' or '/' "
                               "character to end the start tag of "
                               "element '%s', or optionally an attribute; "
                               "perhaps you used an invalid character in "
                               "an attribute name"),
                             utf8_str (context->iter, buf),
                             current_element (context));
                }
              
              /* If we're done with attributes, invoke
               * the start_element callback
               */
              if (context->state == STATE_AFTER_ELISION_SLASH ||
                  context->state == STATE_AFTER_CLOSE_ANGLE)
                {
                  /* Call user callback for element start */
                  error.line_number = context->line_number - context->line_number_after_newline;
                  error.char_number = context->char_number;
                  if (m_recap_depth)
                    recap_start_element (current_element (context), context->attr_names, context->attr_values, error);
                  else
                    context->parser->start_element (current_element (context), context->attr_names, context->attr_values, error);
                  /* Go ahead and free the attributes. */
                  context->attr_names.clear();
                  context->attr_values.clear();
                  if (error.code)
                    {
                      mark_error (context, error);
                    }
                }
            }
          break;
          
        case STATE_AFTER_ATTRIBUTE_EQUALS_SIGN:
          /* Possible next state: INSIDE_ATTRIBUTE_VALUE_[SQ/DQ] */
          
	  skip_spaces (context);
          
	  if (context->iter != context->current_text_end)
	    {
	      if (*context->iter == '"')
		{
		  advance_char (context);
		  context->state = STATE_INSIDE_ATTRIBUTE_VALUE_DQ;
		  context->start = context->iter;
		}
	      else if (*context->iter == '\'')
		{
		  advance_char (context);
		  context->state = STATE_INSIDE_ATTRIBUTE_VALUE_SQ;
		  context->start = context->iter;
		}
	      else
		{
		  char  buf[8];
		  set_error (context, error, MarkupParser::PARSE_ERROR,
			     _("Odd character '%s', expected an open quote mark "
			       "after the equals sign when giving value for "
			       "attribute '%s' of element '%s'"),
			     utf8_str (context->iter, buf),
			     last_attribute (context).c_str(),
			     current_element (context));
		}
	    }
          break;
          
        case STATE_INSIDE_ATTRIBUTE_VALUE_SQ:
        case STATE_INSIDE_ATTRIBUTE_VALUE_DQ:
          /* Possible next states: BETWEEN_ATTRIBUTES */
	  {
	    char  delim;
            
	    if (context->state == STATE_INSIDE_ATTRIBUTE_VALUE_SQ) 
	      {
		delim = '\'';
	      }
	    else 
	      {
		delim = '"';
	      }
            
	    do
	      {
		if (*context->iter == delim)
		  break;
	      }
	    while (advance_char (context));
	  }
          if (context->iter == context->current_text_end)
            {
              /* The value hasn't necessarily ended. Merge with
               * partial chunk, leave state unchanged.
               */
              add_to_partial (context, context->start, context->iter);
            }
          else
            {
              /* The value has ended at the quote mark. Combine it
               * with the partial chunk if any; set it for the current
               * attribute.
               */
              add_to_partial (context, context->start, context->iter);
              String unescaped;
              if (unescape_text (context,
                                 context->partial_chunk.c_str(),
                                 context->partial_chunk.c_str() + context->partial_chunk.size(),
                                 &unescaped,
                                 error))
                {
                  /* success, advance past quote and set state. */
                  last_value (context) = unescaped;
                  advance_char (context);
                  context->state = STATE_BETWEEN_ATTRIBUTES;
                  context->start = NULL;
                }
              
              truncate_partial (context);
            }
          break;
          
        case STATE_INSIDE_TEXT:
          /* Possible next states: AFTER_OPEN_ANGLE */
          do
            {
              if (*context->iter == '<')
                break;
            }
          while (advance_char (context));
          
          /* The text hasn't necessarily ended. Merge with
           * partial chunk, leave state unchanged.
           */
          
          add_to_partial (context, context->start, context->iter);
          
          if (context->iter != context->current_text_end)
            {
              /* The text has ended at the open angle. Call the text
               * callback.
               */
              String unescaped;
              if (unescape_text (context,
                                 context->partial_chunk.c_str(),
                                 context->partial_chunk.c_str() + context->partial_chunk.size(),
                                 &unescaped,
                                 error))
                {
                  error.line_number = context->line_number - context->line_number_after_newline;
                  error.char_number = context->char_number;
                  if (m_recap_depth)
                    recap_text (unescaped, error);
                  else
                    context->parser->text (unescaped, error);
                  if (!error.code)
                    {
                      /* advance past open angle and set state. */
                      advance_char (context);
                      context->state = STATE_AFTER_OPEN_ANGLE;
                      /* could begin a passthrough */
                      context->start = context->iter;
                    }
                  else
                    {
                      mark_error (context, error);
                    }
                }
              
              truncate_partial (context);
            }
          break;
          
        case STATE_AFTER_CLOSE_TAG_SLASH:
          /* Possible next state: INSIDE_CLOSE_TAG_NAME */
          if (is_name_start_char (context->iter))
            {
              context->state = STATE_INSIDE_CLOSE_TAG_NAME;
              
              /* start of tag name */
              context->start = context->iter;
            }
          else
            {
              char  buf[8];
              set_error (context, error, MarkupParser::PARSE_ERROR,
                         _("'%s' is not a valid character following "
                           "the characters '</'; '%s' may not begin an "
                           "element name"),
                         utf8_str (context->iter, buf),
                         utf8_str (context->iter, buf));
            }
          break;
          
        case STATE_INSIDE_CLOSE_TAG_NAME:
          /* Possible next state: AFTER_CLOSE_TAG_NAME */
          advance_to_name_end (context);
	  add_to_partial (context, context->start, context->iter);
          
          if (context->iter != context->current_text_end)
	    context->state = STATE_AFTER_CLOSE_TAG_NAME;
	  break;
          
	case STATE_AFTER_CLOSE_TAG_NAME:
          /* Possible next state: AFTER_CLOSE_TAG_SLASH */
          
	  skip_spaces (context);
	  
	  if (context->iter != context->current_text_end)
	    {
	      String close_name;
              
	      /* The name has ended. Combine it with the partial chunk
	       * if any; check that it matches stack top and pop
	       * stack; invoke proper callback; enter next state.
	       */
	      close_name = context->partial_chunk;
	      context->partial_chunk = "";
              
	      if (*context->iter != '>')
		{
		  char  buf[8];
		  set_error (context, error, MarkupParser::PARSE_ERROR,
			     _("'%s' is not a valid character following "
			       "the close element name '%s'; the allowed "
			       "character is '>'"),
			     utf8_str (context->iter, buf),
			     close_name.c_str());
		}
	      else if (context->tag_stack.empty())
		{
		  set_error (context, error, MarkupParser::PARSE_ERROR,
			     _("Element '%s' was closed, no element "
			       "is currently open"),
			     close_name.c_str());
		}
	      else if (close_name != current_element (context))
		{
		  set_error (context, error, MarkupParser::PARSE_ERROR,
			     _("Element '%s' was closed, but the currently "
			       "open element is '%s'"),
			     close_name.c_str(),
			     current_element (context));
		}
	      else
		{
		  advance_char (context);
		  context->state = STATE_AFTER_CLOSE_ANGLE;
		  context->start = NULL;
		  
		  /* call the end_element callback */
                  error.line_number = context->line_number - context->line_number_after_newline;
                  error.char_number = context->char_number;
                  if (m_recap_depth)
                    recap_end_element (close_name, error);
                  else
                    context->parser->end_element (close_name, error);
		  
		  /* Pop the tag stack */
		  context->tag_stack.pop();
		  if (error.code)
                    {
                      mark_error (context, error);
                    }
                }
            }
          break;
	  
        case STATE_INSIDE_PASSTHROUGH:
          /* Possible next state: AFTER_CLOSE_ANGLE */
          do
            {
	      if (*context->iter == '<') 
		context->balance++;
              if (*context->iter == '>') 
		{
		  context->balance--;
		  add_to_partial (context, context->start, context->iter);
		  context->start = context->iter;
		  if ((str_has_prefix (context->partial_chunk, "<?")
		       && str_has_suffix (context->partial_chunk, "?")) ||
		      (str_has_prefix (context->partial_chunk, "<!--")
		       && str_has_suffix (context->partial_chunk, "--")) ||
		      (str_has_prefix (context->partial_chunk, "<![CDATA[") 
		       && str_has_suffix (context->partial_chunk, "]]")) ||
		      (str_has_prefix (context->partial_chunk, "<!DOCTYPE")
		       && context->balance == 0)) 
		    break;
		}
            }
          while (advance_char (context));
          
          if (context->iter == context->current_text_end)
            {
              /* The passthrough hasn't necessarily ended. Merge with
               * partial chunk, leave state unchanged.
               */
              add_to_partial (context, context->start, context->iter);
            }
          else
            {
              /* The passthrough has ended at the close angle. Combine
               * it with the partial chunk if any. Call the passthrough
               * callback. Note that the open/close angles are
               * included in the text of the passthrough.
               */
              advance_char (context); /* advance past close angle */
              add_to_partial (context, context->start, context->iter);
              
              error.line_number = context->line_number - context->line_number_after_newline;
              error.char_number = context->char_number;
              if (m_recap_depth)
                recap_pass_through (context->partial_chunk, error);
              else
                context->parser->pass_through (context->partial_chunk, error);
              
              truncate_partial (context);
              
              if (!error.code)
                {
                  context->state = STATE_AFTER_CLOSE_ANGLE;
                  context->start = context->iter; /* could begin text */
                }
              else
                {
                  mark_error (context, error);
                }
            }
          break;
          
        case STATE_ERROR:
          goto finished;
          break;
          
        default:
          assert_not_reached ();
          break;
        }
    }
  
 finished:
  context->parsing = false;
  
  return context->state != STATE_ERROR;
}

bool
MarkupParser::end_parse (Error *errorp)
{
  Error dummy, &error = errorp ? *errorp : dummy;
  return_val_if_fail (context != NULL, false);
  return_val_if_fail (!context->parsing, false);
  return_val_if_fail (context->state != STATE_ERROR, false);
  
  context->partial_chunk = "";
  
  if (context->document_empty)
    {
      set_error (context, error, MarkupParser::DOCUMENT_EMPTY, _("Document was empty or contained only whitespace"));
      return false;
    }
  
  context->parsing = true;
  
  switch (context->state)
    {
    case STATE_START:
      /* Nothing to do */
      break;
      
    case STATE_AFTER_OPEN_ANGLE:
      set_error (context, error, MarkupParser::PARSE_ERROR, _("Document ended unexpectedly just after an open angle bracket '<'"));
      break;
      
    case STATE_AFTER_CLOSE_ANGLE:
      if (!context->tag_stack.empty())
        {
          /* Error message the same as for INSIDE_TEXT */
          set_error (context, error, MarkupParser::PARSE_ERROR,
                     _("Document ended unexpectedly with elements still open - "
                       "'%s' was the last element opened"),
                     current_element (context));
        }
      break;
      
    case STATE_AFTER_ELISION_SLASH:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly, expected to see a close angle "
                   "bracket ending the tag <%s/>"), current_element (context));
      break;
      
    case STATE_INSIDE_OPEN_TAG_NAME:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly inside an element name"));
      break;
      
    case STATE_INSIDE_ATTRIBUTE_NAME:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly inside an attribute name"));
      break;
      
    case STATE_BETWEEN_ATTRIBUTES:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly inside an element-opening "
                   "tag."));
      break;
      
    case STATE_AFTER_ATTRIBUTE_EQUALS_SIGN:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly after the equals sign "
                   "following an attribute name; no attribute value"));
      break;
      
    case STATE_INSIDE_ATTRIBUTE_VALUE_SQ:
    case STATE_INSIDE_ATTRIBUTE_VALUE_DQ:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly while inside an attribute "
                   "value"));
      break;
      
    case STATE_INSIDE_TEXT:
      assert (!context->tag_stack.empty());
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly with elements still open - "
                   "'%s' was the last element opened"),
                 current_element (context));
      break;
      
    case STATE_AFTER_CLOSE_TAG_SLASH:
    case STATE_INSIDE_CLOSE_TAG_NAME:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly inside the close tag for element '%s'"), current_element (context));
      break;
      
    case STATE_INSIDE_PASSTHROUGH:
      set_error (context, error, MarkupParser::PARSE_ERROR,
                 _("Document ended unexpectedly inside a comment or "
                   "processing instruction"));
      break;
      
    case STATE_ERROR:
    default:
      assert_not_reached ();
      break;
    }
  
  context->parsing = false;
  
  return context->state != STATE_ERROR;
}

String
MarkupParser::get_element()
{
  return_val_if_fail (context != NULL, NULL);
  if (context->tag_stack.empty())
    return NULL;
  else
    return current_element (context);
}

String
MarkupParser::input_name ()
{
  return m_input_name;
}

void
MarkupParser::get_position (int            *line_number,
                            int            *char_number,
                            const char    **input_name_p)
{
  /* For user-constructed error messages, has no precise semantics */
  return_if_fail (context != NULL);
  if (line_number)
    *line_number = context->line_number - context->line_number_after_newline;
  if (char_number)
    *char_number = context->char_number;
  if (input_name_p)
    *input_name_p = m_input_name.c_str();
}

void
MarkupParser::recap_element (const String   &element_name,
                             ConstStrings   &attribute_names,
                             ConstStrings   &attribute_values,
                             Error          &error,
                             bool            include_outer)
{
  if (error.set())
    return;
  assert (m_recap_depth == 0);
  m_recap = "";
  m_recap_outer = include_outer;
  recap_start_element (element_name, attribute_names, attribute_values, error);
  if (!m_recap_outer)
    m_recap = "";
}

const String&
MarkupParser::recap_string () const
{
  return m_recap;
}

void
MarkupParser::start_element (const String  &element_name,
                             ConstStrings  &attribute_names,
                             ConstStrings  &attribute_values,
                             Error         &error)
{
  /* Called for open tags <foo bar="baz"> */
}

void
MarkupParser::end_element (const String  &element_name,
                           Error         &error)
{
  /* Called for close tags </foo> */
}

void
MarkupParser::text (const String        &text,
                    Error               &error)
{
  /* Called for character data */
}

void
MarkupParser::pass_through (const String        &pass_through_text,
                            Error               &error)
{
  /* Called for strings that should be re-saved verbatim in this same
   * position, but are not otherwise interpretable.  At the moment
   * this includes comments and processing instructions.
   */
}

void
MarkupParser::error (const Error &error)
{
  /* Called on error, including one set by other
   * methods in the vtable. The MarkupParserBase::Error should not be freed.
   */
}

static void
append_escaped_text (String      &str,
                     const char  *text,
                     ssize_t       length)    
{
  const char  *p = text;
  const char  *end = text + length;
  while (p != end)
    {
      const char  *next = utf8_next_char (p);
      switch (*p)
        {
        case '&':
          str += "&amp;";
          break;
        case '<':
          str += "&lt;";
          break;
        case '>':
          str += "&gt;";
          break;
        case '\'':
          str += "&apos;";
          break;
        case '"':
          str += "&quot;";
          break;
        default:
          str.append (p, next - p);
          break;
        }
      p = next;
    }
}

String
MarkupParser::escape_text (const String   &text)
{
  return escape_text (&*text.begin(), text.size());
}

/**
 * g_markup_escape_text:
 * @text: some valid UTF-8 text
 * @length: length of @text in bytes
 * 
 * Escapes text so that the markup parser will parse it verbatim.
 * Less than, greater than, ampersand, etc. are replaced with the
 * corresponding entities. This function would typically be used
 * when writing out a file to be parsed with the markup parser.
 * 
 * Note that this function doesn't protect whitespace and line endings
 * from being processed according to the XML rules for normalization
 * of line endings and attribute values.
 * 
 * Return value: escaped text
 **/
String
MarkupParser::escape_text (const char  *text,
                           ssize_t       length)  
{
  return_val_if_fail (text != NULL, NULL);
  
  if (length < 0)
    length = strlen (text);
  
  /* prealloc at least as long as original text */
  String str;
  append_escaped_text (str, text, length);
  
  return str;
}

/**
 * find_conversion:
 * @format: a printf-style format string
 * @after: location to store a pointer to the character after
 *   the returned conversion. On a %NULL return, returns the
 *   pointer to the trailing NUL in the string
 * 
 * Find the next conversion in a printf-style format string.
 * Partially based on code from printf-parser.c,
 * Copyright (C) 1999-2000, 2002-2003 Free Software Foundation, Inc.
 * 
 * Return value: pointer to the next conversion in @format,
 *  or %NULL, if none.
 **/
static const char *
find_conversion (const char  *format,
		 const char **after)
{
  const char *start = format;
  const char *cp;
  
  while (*start != '\0' && *start != '%')
    start++;
  
  if (*start == '\0')
    {
      *after = start;
      return NULL;
    }
  
  cp = start + 1;
  
  if (*cp == '\0')
    {
      *after = cp;
      return NULL;
    }
  
  /* Test for positional argument.  */
  if (*cp >= '0' && *cp <= '9')
    {
      const char *np;
      
      for (np = cp; *np >= '0' && *np <= '9'; np++)
	;
      if (*np == '$')
	cp = np + 1;
    }
  
  /* Skip the flags.  */
  for (;;)
    {
      if (*cp == '\'' ||
	  *cp == '-' ||
	  *cp == '+' ||
	  *cp == ' ' ||
	  *cp == '#' ||
	  *cp == '0')
	cp++;
      else
	break;
    }
  
  /* Skip the field width.  */
  if (*cp == '*')
    {
      cp++;
      
      /* Test for positional argument.  */
      if (*cp >= '0' && *cp <= '9')
	{
	  const char *np;
          
	  for (np = cp; *np >= '0' && *np <= '9'; np++)
	    ;
	  if (*np == '$')
	    cp = np + 1;
	}
    }
  else
    {
      for (; *cp >= '0' && *cp <= '9'; cp++)
	;
    }
  
  /* Skip the precision.  */
  if (*cp == '.')
    {
      cp++;
      if (*cp == '*')
	{
	  /* Test for positional argument.  */
	  if (*cp >= '0' && *cp <= '9')
	    {
	      const char *np;
              
	      for (np = cp; *np >= '0' && *np <= '9'; np++)
		;
	      if (*np == '$')
		cp = np + 1;
	    }
	}
      else
	{
	  for (; *cp >= '0' && *cp <= '9'; cp++)
	    ;
	}
    }
  
  /* Skip argument type/size specifiers.  */
  while (*cp == 'h' ||
	 *cp == 'L' ||
	 *cp == 'l' ||
	 *cp == 'j' ||
	 *cp == 'z' ||
	 *cp == 'Z' ||
	 *cp == 't')
    cp++;
  
  /* Skip the conversion character.  */
  cp++;
  
  *after = cp;
  return start;
}

String
MarkupParser::vprintf_escaped (const char *format,
                               va_list     args)
{
  /* The technique here, is that we make two format strings that
   * have the identical conversions in the identical order to the
   * original strings, but differ in the text in-between. We
   * then use the normal g_strdup_vprintf() to format the arguments
   * with the two new format strings. By comparing the results,
   * we can figure out what segments of the output come from
   * the the original format string, and what from the arguments,
   * and thus know what portions of the string to escape.
   *
   * For instance, for:
   *
   *  g_markup_printf_escaped ("%s ate %d apples", "Susan & Fred", 5);
   *
   * We form the two format strings "%sX%dX" and %sY%sY". The results
   * of formatting with those two strings are
   *
   * "%sX%dX" => "Susan & FredX5X"
   * "%sY%dY" => "Susan & FredY5Y"
   *
   * To find the span of the first argument, we find the first position
   * where the two arguments differ, which tells us that the first
   * argument formatted to "Susan & Fred". We then escape that
   * to "Susan &amp; Fred" and join up with the intermediate portions
   * of the format string and the second argument to get
   * "Susan &amp; Fred ate 5 apples".
   */
  
  /* Create the two modified format strings
   */
  String format1, format2;
  const char *p = format;
  while (true)
    {
      const char *after;
      const char *conv = find_conversion (p, &after);
      if (!conv)
	break;
      
      format1.append (conv, after - conv);
      format1.append ("X");
      format2.append (conv, after - conv);
      format2.append ("Y");
      p = after;
    }
  
  /* Use them to format the arguments
   */
  va_list args2;
  va_copy (args2, args);
  
  String output1, output2, result;
  try {
    output1 = string_vprintf (format1.c_str(), args);
    output2 = string_vprintf (format2.c_str(), args2);
  } catch (...) {
    /* cleanup */
    va_end (args);
    va_end (args2);
    throw; /* rethrow */
  }
  va_end (args);
  va_end (args2);
  
  /* Iterate through the original format string again,
   * copying the non-conversion portions and the escaped
   * converted arguments to the output string.
   */
  const char *op1 = output1.c_str();
  const char *op2 = output2.c_str();
  p = format;
  while (true)
    {
      const char *after;
      const char *output_start;
      const char *conv = find_conversion (p, &after);
      
      if (!conv)	/* The end, after points to the trailing \0 */
	{
	  result.append (p, after - p);
	  break;
	}
      
      result.append (p, conv - p);
      output_start = op1;
      while (*op1 == *op2)
	{
	  op1++;
	  op2++;
	}
      
      String escaped = escape_text (output_start, op1 - output_start);
      result.append (escaped);
      
      p = after;
      op1++;
      op2++;
    }
  
  return result;
}

String
MarkupParser::printf_escaped (const char *format,
                              ...)
{
  String result;
  va_list args;
  va_start (args, format);
  try {
    result = vprintf_escaped (format, args);
  } catch (...) {
    /* cleanup */
    va_end (args);
    throw; /* rethrow */
  }
  va_end (args);
  return result;
}

void
MarkupParser::recap_start_element (const String   &element_name,
                                   ConstStrings   &attribute_names,
                                   ConstStrings   &attribute_values,
                                   Error          &error)
{
  m_recap_depth++;
  String appendix;
  appendix += "<" + element_name;
  for (uint i = 0; i < attribute_names.size(); i++)
    {
      appendix += "\n" + attribute_names[i] + "='";
      String qvalue;
      for (String::const_iterator it = attribute_values[i].begin(); it != attribute_values[i].end(); it++)
        if (*it == '\'')
          qvalue += "\\'";
        else
          qvalue += *it;
      appendix += qvalue + "'";
    }
  appendix += ">";
  m_recap += appendix; // takes potentially long
}

void
MarkupParser::recap_end_element (const String   &element_name,
                                 Error          &error)
{
  m_recap_depth--;
  if (m_recap_depth || m_recap_outer)
    {
      String appendix ("</" + element_name + ">");
      m_recap += appendix; // takes potentially long
    }
  if (!m_recap_depth)
    {
      end_element (element_name, error);
      m_recap.clear(); // potentially large savings
    }
}

void
MarkupParser::recap_text (const String   &text,
                          Error          &error)
{
  m_recap += escape_text (text);
}

void
MarkupParser::recap_pass_through (const String   &pass_through_text,
                                  Error          &error)
{}

} // Birnet
