// This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
#include <ui/clientapi.hh>

namespace {
using namespace Rapicorn;

static bool
handle_commands (WindowH &window, const String &command, const StringSeq &args)
{
  printout ("%s(): command: %s(%s)\n", __func__, command.c_str(), string_join (",", args).c_str());
  if (command == "close")
    window.close();
  return true;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn for X11 backend with application name */
  ApplicationH app = init_app ("HelloWorld", &argc, argv);

  /* find and load GUI definitions relative to argv[0] */
  app.auto_load ("RapicornTest",        // namespace domain
                 "hello.xml",           // GUI file name
                 argv[0]);

  /* create main window */
  WindowH window = app.create_window ("RapicornTest:main-window");

  /* connect custom callback to handle UI commands */
  window.sig_commands ([&window] (const String &command, const StringSeq &args) -> bool { return handle_commands (window, command, args); });

  /* display window on screen */
  window.show();

  /* run event loops while windows are on screen */
  return app.run_and_exit();
}

} // anon
