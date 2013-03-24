#!/usr/bin/env python
# Aida - Abstract Interface Definition Architecture
# Licensed GNU GPL v3 or later: http://www.gnu.org/licenses/gpl.html
"""AidaPrettyDump - Pretty printing of Aida type information
"""

def generate (namespace_list, **args):
  import pprint, re
  for ns in namespace_list:
    print "namespace %s:" % ns.name
    consts = ns.consts()
    if consts:
      print "  Constants:"
      str = pprint.pformat (consts, 2)
      print re.compile (r'^', re.MULTILINE).sub ('    ', str)
    types = ns.types()
    if types:
      print "  Types:"
      for tp in types:
        print "    TypeInfo %s:" % tp.name
        str = pprint.pformat (tp.__dict__)
        print re.compile (r'^', re.MULTILINE).sub ('      ', str)

# control module exports
__all__ = ['generate']
