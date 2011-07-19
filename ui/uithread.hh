// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_UITHREAD_HH__
#define __RAPICORN_UITHREAD_HH__

#include <rapicorn-core.hh>
#include <ui/application.hh>
#include <ui/arrangement.hh>
#include <ui/buttons.hh>
#include <ui/coffer.hh>
#include <ui/container.hh>
#include <ui/cmdlib.hh>
#include <ui/evaluator.hh>
#include <ui/events.hh>
#include <ui/factory.hh>
#include <ui/heritage.hh>
#include <ui/image.hh>
#include <ui/item.hh>
#include <ui/layoutcontainers.hh>
#include <ui/listarea.hh>
#include <ui/loop.hh>
#include <ui/paintcontainers.hh>
#include <ui/paintitems.hh>
#include <ui/painter.hh>
#include <ui/primitives.hh>
#include <ui/properties.hh>
#include <ui/region.hh>
#include <ui/rope.hh>
#include <ui/scrollitems.hh>
#include <ui/sinfex.hh>
#include <ui/sizegroup.hh>
#include <ui/table.hh>
#include <ui/text-editor.hh>
// conditional: #include <ui/text-pango.hh>
#include <ui/utilities.hh>
#include <ui/viewp0rt.hh>
#include <ui/window.hh>

namespace Rapicorn {

uint64  uithread_bootup       (int *argcp, char **argv, const StringVector &args);
int     shutdown_app          (int exit_status = 0); // also in clientapi.hh
void    uithread_test_trigger (void (*) ());

/// Register a standard test function for execution in the ui-thread.
#define REGISTER_UITHREAD_TEST(name, ...)     static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('T', name, __VA_ARGS__)

/// Register a slow test function for execution in the ui-thread during slow unit testing.
#define REGISTER_UITHREAD_SLOWTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('S', name, __VA_ARGS__)

/// Register a logging test function with the ui-thread for output recording and verification.
#define REGISTER_UITHREAD_LOGTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('L', name, __VA_ARGS__)

} // Rapicorn

#endif  // __RAPICORN_UITHREAD_HH__
