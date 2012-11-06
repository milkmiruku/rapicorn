// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// C++ interface code insertions for server stubs

includes:
#include <rcore/rapicornconfig.h>

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:StringSeq:
  explicit StringSeqImpl () {}
  /*ctor*/ StringSeqImpl (const std::vector<std::string> &strv) : Sequence (strv) {}

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
