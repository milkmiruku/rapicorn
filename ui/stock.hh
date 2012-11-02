// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_STOCK_HH__
#define __RAPICORN_STOCK_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// == Stock ==
class Stock {
public:
  static String       stock_string      (const String &stock_id, const String &key);
  static String       stock_label       (const String &stock_id);
  static String       stock_tooltip     (const String &stock_id);
  static String       stock_accelerator (const String &stock_id);
  static Blob         stock_image       (const String &stock_id);
};

} // Rapicorn

#endif // __RAPICORN_STOCK_HH__
