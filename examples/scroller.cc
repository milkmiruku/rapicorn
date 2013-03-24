// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
add_button_row (ContainerH area, uint row)
{
  WidgetH child = area.create_child ("RapicornExamples:button-row", Strings ("id=row#" + string_from_uint (row)));
  ContainerH brow = ContainerH::down_cast (child);
  for (uint i = 0; i < 50; i++)
    {
      Strings args = Strings ("test-button-text=\"(" + string_from_uint (row) + "," + string_from_uint (i) + ")\"");
      child = brow.create_child ("RapicornExamples:test-button", args);
      ButtonAreaH button = child.component<ButtonAreaH> (".Button");
      button.on_click (string_printf ("Widget::print('Button-Click: (%d,%d)')", row, i));
    }
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app (__PRETTY_FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornExamples", "scroller.xml", argv[0]);

  // create main window
  WindowH window = app.create_window ("RapicornExamples:main-shell");

  // create button rows
  ContainerH mshell = window;
  for (uint i = 0; i < 50; i++)
    add_button_row (mshell, i);

  // show main window
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
