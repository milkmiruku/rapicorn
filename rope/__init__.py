# Rapicorn                              -*- Mode: python; -*-
# Copyright (C) 2010 Tim Janik
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# A copy of the GNU Lesser General Public License should ship along
# with this library; if not, see http://www.gnu.org/copyleft/.

"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

app = None
def app_init (application_name = None):
  global app
  assert app == None
  _CPY, _PY = app_init._CPY, app_init._PY # from _module_init_once_
  del globals()['app_init'] # run once only
  # initialize dispatching Rapicorn thread
  cmdline_args = None
  import sys
  if application_name == None:
    import os
    application_name = os.path.abspath (sys.argv[0] or '-')
  if cmdline_args == None:
    cmdline_args = sys.argv
  aida_id = _CPY._init_dispatcher (application_name, cmdline_args)
  # integrate Rapicorn dispatching with main loop
  class RapicornSource (main.Source):
    def __init__ (self):
      super (RapicornSource, self).__init__ (_CPY._event_dispatch)
      import select
      fd, pollmask = _CPY._event_fd()
      if fd >= 0:
        self.set_poll (fd, pollmask)
    def prepare (self, current_time, timeout):
      return _CPY._event_check()
    def check (self, current_time, fdevents):
      return _CPY._event_check()
    def dispatch (self, fdevents):
      self.callable() # _event_dispatch
      return True
  main.RapicornSource = RapicornSource
  # setup global Application
  app = Application (_PY._BaseClass_._AidaID_ (aida_id))
  def iterate (self, may_block, may_dispatch):
    if hasattr (self, "main_loop"):
      loop = self.main_loop
      dloop = None
    else:
      dloop = main.Loop()
      loop = dloop
      loop += main.RapicornSource()
    needs_dispatch = loop.iterate (may_block, may_dispatch)
    loop = None
    del dloop
    return needs_dispatch
  app.__class__.iterate = iterate # extend for main loop integration
  def loop (self):
    self.main_loop = main.Loop()
    self.main_loop += main.RapicornSource()
    exit_status = self.main_loop.loop()
    del self.main_loop
    return exit_status
  app.__class__.loop = loop # extend for main loop integration
  main.app = app # integrate main loop with app
  return app

class AidaObjectFactory:
  def __init__ (self, _PY):
    self._PY = _PY
    self.AidaID = _PY._BaseClass_._AidaID_
  def __call__ (self, type_name, rpc_id):
    klass = getattr (self._PY, type_name)
    if not klass or not rpc_id:
      return None
    return klass (self.AidaID (rpc_id))

def _module_init_once_():
  global _module_init_once_ ; del _module_init_once_
  import pyRapicorn     # generated _AIDA_... cpy methods
  import py2cpy         # generated Python classes, Application, etc
  py2cpy.__aida_module_init_once__ (pyRapicorn)
  pyRapicorn._AIDA___register_object_factory_callable (AidaObjectFactory (py2cpy))
  del globals()['AidaObjectFactory']
  app_init._CPY, app_init._PY = (pyRapicorn, py2cpy) # app_init() internals
  del globals()['pyRapicorn']
  del globals()['py2cpy']
_module_init_once_()

# introduce module symbols
from py2cpy import *
import main
