# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

# Import Aida helper module
from Aida1307 import loop as _Aida_loop

# Load generated C++ API
import __pyrapicorn as _cxxrapicorn

# Load generated Python API
import pyrapicorn
from pyrapicorn import *        # incorporate namespace

# Hook up Python API with C++ API
pyrapicorn._CPY = _cxxrapicorn

# Integrate Rapicorn dispatching with event loop
class _RapicornSource (_Aida_loop.Source):
  def __init__ (self):
    super (_RapicornSource, self).__init__ (_cxxrapicorn._event_dispatch)
    import select
    fd, pollmask = _cxxrapicorn._event_fd()
    if fd >= 0:
      self.set_poll (fd, pollmask)
  def prepare (self, current_time, timeout):
    return _cxxrapicorn._event_check()
  def check (self, current_time, fdevents):
    return _cxxrapicorn._event_check()
  def dispatch (self, fdevents):
    self.callable() # calls _event_dispatch
    return True

# Main event loop intergration for Rapicorn.Application
class MainApplication (Rapicorn.Application):
  def __init__ (self):
    super (MainApplication, self).__init__()
    self.__dict__['__cached_exitable'] = False
    self.sig_missing_primary += lambda: Rapicorn.app.__dict__.__setitem__ ('__cached_exitable', True)
  def iterate (self, may_block, may_dispatch):
    delete_loop = None
    try:
      event_loop = self.__dict__['__event_loop']
    except KeyError:
      event_loop = _Aida_loop.Loop()
      event_loop += _RapicornSource()
      delete_loop = event_loop
    needs_dispatch = event_loop.iterate (may_block, may_dispatch)
    event_loop = None
    del delete_loop
    return needs_dispatch
  def inloop (self):
    return self.__dict__.has_key ('__event_loop')
  def loop (self):
    assert not self.inloop()
    event_loop = _Aida_loop.Loop()
    event_loop += _RapicornSource()
    self.__dict__['__event_loop'] = event_loop                  # inloop == True
    while event_loop.quit_status == None:
      if not event_loop.iterate (False, True):                  # complete events already queued
        break
    self.__dict__['__cached_exitable'] = Rapicorn.app.finishable() # run while not exitable
    while event_loop.quit_status == None and not self.__dict__['__cached_exitable']:
      self.iterate (True, True)                                 # also processes signals
    del self.__dict__['__event_loop']                           # inloop == False
    exit_status = event_loop.quit_status
    return exit_status

# Application Initialization
Rapicorn.app = None
Rapicorn._appclass = MainApplication
def app_init (application_name = None):
  from pyrapicorn import Rapicorn
  assert Rapicorn.app == None
  # initialize dispatching Rapicorn thread
  import sys
  if application_name == None:
    import os
    application_name = os.path.abspath (sys.argv[0] or '-')
  # initialize and setup global Application
  Rapicorn.app = _cxxrapicorn._init_dispatcher (application_name, sys.argv, "Rapicorn._appclass")
  del Rapicorn._appclass
  return Rapicorn.app
Rapicorn.app_init = app_init
del app_init
