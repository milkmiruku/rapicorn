// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_TEXT_PANGO_HH__
#define __RAPICORN_TEXT_PANGO_HH__

#include <ui/utilities.hh>
#include <ui/text-editor.hh>

namespace Rapicorn {

#if     RAPICORN_WITH_PANGO
class TextLayout : public virtual WidgetImpl {
  virtual String      markup_text      () const = 0;
  virtual void        markup_text      (const String &markup) = 0;
};
class TextPango : public virtual TextLayout { // FIXME: move to TextBlock
public:
  virtual void          font_name       (const String &fname) = 0;
  virtual String        font_name       () const = 0;
  virtual AlignType     align           () const = 0;
  virtual void          align           (AlignType at) = 0;
  virtual EllipsizeType ellipsize       () const = 0;
  virtual void          ellipsize       (EllipsizeType et) = 0;
  virtual uint16        spacing         () const = 0;
  virtual void          spacing         (uint16 sp) = 0;
  virtual int16         indent          () const = 0;
  virtual void          indent          (int16 sp) = 0;
  virtual void          text            (const String &text) = 0;
  virtual String        text            () const = 0;
};
#endif  /* RAPICORN_WITH_PANGO */

} // Rapicorn

#endif  /* __RAPICORN_TEXT_PANGO_HH__ */
