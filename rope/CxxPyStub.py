#!/usr/bin/env python
# Rapicorn-CxxPyStub                                           -*-mode:python-*-
# Copyright (C) 2009-2010 Tim Janik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
"""Rapicorn-CxxPyStub - C-Python RPC glue generator

More details at http://www.rapicorn.org
"""
import Decls, GenUtils, re

FieldBuffer = 'Plic::FieldBuffer'

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

base_code = """
#include <Python.h> // must be included first to configure std headers
#include <string>

#include <rapicorn-core.hh>

#define None_INCREF()   ({ Py_INCREF (Py_None); Py_None; })
#define GOTO_ERROR()    goto error
#define ERRORif(cond)   if (cond) goto error
#define ERRORifpy()     if (PyErr_Occurred()) goto error
#define ERRORpy(msg)    do { PyErr_Format (PyExc_RuntimeError, msg); goto error; } while (0)
#define ERRORifnotret(fr) do { if (PLIC_UNLIKELY (!fr) || \\
                                   PLIC_UNLIKELY (!Plic::is_callid_return (fr->first_id()))) { \\
                                 PyErr_Format_from_PLIC_error (fr); \\
                                 goto error; } } while (0)

static PyObject*
PyErr_Format_from_PLIC_error (const Plic::FieldBuffer *fr)
{
  if (!fr)
    return PyErr_Format (PyExc_RuntimeError, "PLIC: missing return value");
  if (Plic::is_callid_error (fr->first_id()))
    {
      Plic::FieldBufferReader frr (*fr);
      frr.skip(); // proc_id
      std::string msg = frr.pop_string(), domain = frr.pop_string();
      if (domain.size()) domain += ": ";
      msg = domain + msg;
      return PyErr_Format (PyExc_RuntimeError, "%s", msg.c_str());
    }

  return PyErr_Format (PyExc_RuntimeError, "PLIC: garbage return: 0x%s", fr->first_id_str().c_str());
}

static inline PY_LONG_LONG
PyIntLong_AsLongLong (PyObject *intlong)
{
  if (PyInt_Check (intlong))
    return PyInt_AS_LONG (intlong);
  else
    return PyLong_AsLongLong (intlong);
}

static inline std::string
PyString_As_std_string (PyObject *pystr)
{
  char *s = NULL;
  Py_ssize_t len = 0;
  PyString_AsStringAndSize (pystr, &s, &len);
  return std::string (s, len);
}

static inline std::string
PyAttr_As_std_string (PyObject *pyobj, const char *attr_name)
{
  PyObject *o = PyObject_GetAttrString (pyobj, attr_name);
  if (o)
     return PyString_As_std_string (o);
  return "";
}

static inline PyObject*
PyString_From_std_string (const std::string &s)
{
  return PyString_FromStringAndSize (s.data(), s.size());
}

static inline int
PyDict_Take_Item (PyObject *pydict, const char *key, PyObject **pyitemp)
{
  int r = PyDict_SetItemString (pydict, key, *pyitemp);
  if (r >= 0)
    {
      Py_DECREF (*pyitemp);
      *pyitemp = NULL;
    }
  return r;
}

static inline int
PyList_Take_Item (PyObject *pylist, PyObject **pyitemp)
{
  int r = PyList_Append (pylist, *pyitemp);
  if (r >= 0)
    {
      Py_DECREF (*pyitemp);
      *pyitemp = NULL;
    }
  return r;
}

static Plic::FieldBuffer* plic_call_remote (Plic::FieldBuffer*);
#ifndef HAVE_PLIC_CALL_REMOTE
static Plic::FieldBuffer* plic_call_remote (Plic::FieldBuffer *fb)
{ delete fb; return NULL; } // testing stub
#define HAVE_PLIC_CALL_REMOTE
#endif
"""

class Generator:
  def __init__ (self):
    self.ntab = 26
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def generate_enum_impl (self, type_info):
    s = ''
    l = []
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, number)
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};'
    return s
  def generate_proto_add_py (self, fb, type, var):
    s = ''
    if type.storage == Decls.INT:
      s += '  %s.add_int64 (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.ENUM:
      s += '  %s.add_evalue (PyIntLong_AsLongLong (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.FLOAT:
      s += '  %s.add_double (PyFloat_AsDouble (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage == Decls.STRING:
      s += '  %s.add_string (PyString_As_std_string (%s)); ERRORifpy();\n' % (fb, var)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  if (!plic_py%s_proto_add (%s, %s)) goto error;\n' % (type.name, var, fb)
    elif type.storage == Decls.INTERFACE:
      s += '  %s.add_string (PyAttr_As_std_string (%s, "__plic__object__")); ERRORifpy();\n' % (fb, var)
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_proto_pop_py (self, fbr, type, var):
    s = ''
    if type.storage == Decls.INT:
      s += '  %s = PyLong_FromLongLong (%s.pop_int64()); ERRORifpy ();\n' % (var, fbr)
    elif type.storage == Decls.ENUM:
      s += '  %s = PyLong_FromLongLong (%s.pop_evalue()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.FLOAT:
      s += '  %s = PyFloat_FromDouble (%s.pop_double()); ERRORifpy();\n' % (var, fbr)
    elif type.storage == Decls.STRING:
      s += '  %s = PyString_From_std_string (%s.pop_string()); ERRORifpy();\n' % (var, fbr)
    elif type.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s = plic_py%s_proto_pop (%s); ERRORif (!pyfoR);\n' % (var, type.name, fbr)
    elif type.storage == Decls.INTERFACE:
      s += '  %s = PyString_From_std_string (%s.pop_string()); ERRORifpy();\n' % (var, fbr)
      s += '  // FIXME: convert to "__plic__object__"\n'
    else: # FUNC VOID
      raise RuntimeError ("marshalling not implemented: " + type.storage)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    # record proto add
    s += 'static RAPICORN_UNUSED bool\n'
    s += 'plic_py%s_proto_add (PyObject *pyrec, %s &dst)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  %s &fb = dst.add_rec (%u);\n' % (FieldBuffer, len (type_info.fields))
    s += '  bool success = false;\n'
    s += '  PyObject *dictR = NULL, *item = NULL;\n'
    s += '  dictR = PyObject_GetAttrString (pyrec, "__dict__"); ERRORif (!dictR);\n'
    for fl in type_info.fields:
      s += '  item = PyDict_GetItemString (dictR, "%s"); ERRORif (!item);\n' % (fl[0])
      s += self.generate_proto_add_py ('fb', fl[1], 'item')
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (dictR);\n'
    s += '  return success;\n'
    s += '}\n'
    # record proto pop
    s += 'static RAPICORN_UNUSED PyObject*\n'
    s += 'plic_py%s_proto_pop (%sReader &src)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  PyObject *pyinstR = NULL, *dictR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() != %u) ERRORpy ("PLIC: marshalling error: invalid record length");\n' % len (type_info.fields)
    s += '  pyinstR = PyInstance_NewRaw ((PyObject*) &PyBaseObject_Type, NULL); ERRORif (!pyinstR);\n'
    s += '  dictR = PyObject_GetAttrString (pyinstR, "__dict__"); ERRORif (!dictR);\n'
    for fl in type_info.fields:
      s += self.generate_proto_pop_py ('fbr', fl[1], 'pyfoR')
      s += '  if (PyDict_Take_Item (dictR, "%s", &pyfoR) < 0) goto error;\n' % fl[0]
    s += '  pyret = pyinstR;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (pyfoR);\n'
    s += '  Py_XDECREF (dictR);\n'
    s += '  if (pyret != pyinstR)\n'
    s += '    Py_XDECREF (pyinstR);\n'
    s += '  return pyret;\n'
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    for fl in [ type_info.elements ]:
      pstar = '*' if fl[1].storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE) else ''
      s += '  ' + self.format_to_tab (self.type2cpp (fl[1].name)) + pstar + fl[0] + ';\n'
    s += '};\n'
    el = type_info.elements
    # sequence proto add
    s += 'static RAPICORN_UNUSED bool\n'
    s += 'plic_py%s_proto_add (PyObject *pylist, %s &dst)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  const ssize_t len = PyList_Size (pylist); if (len < 0) return false;\n'
    s += '  %s &fb = dst.add_seq (len);\n' % FieldBuffer
    s += '  bool success = false;\n'
    s += '  for (ssize_t k = 0; k < len; k++) {\n'
    s += '    PyObject *item = PyList_GET_ITEM (pylist, k);\n'
    s += reindent ('  ', self.generate_proto_add_py ('fb', el[1], 'item')) + '\n'
    s += '  }\n'
    s += '  success = true;\n'
    s += ' error:\n'
    s += '  return success;\n'
    s += '}\n'
    # sequence proto pop
    s += 'static RAPICORN_UNUSED PyObject*\n'
    s += 'plic_py%s_proto_pop (%sReader &src)\n' % (type_info.name, FieldBuffer)
    s += '{\n'
    s += '  PyObject *listR = NULL, *pyfoR = NULL, *pyret = NULL;\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    s += '  listR = PyList_New (len); if (!listR) GOTO_ERROR();\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_pop_py ('fbr', el[1], 'pyfoR')) + '\n'
    s += '    if (PyList_Take_Item (listR, &pyfoR) < 0) goto error;\n'
    s += '  }\n'
    s += '  pyret = listR;\n'
    s += ' error:\n'
    s += '  Py_XDECREF (pyfoR);\n'
    s += '  if (pyret != listR)\n'
    s += '    Py_XDECREF (listR);\n'
    s += '  return pyret;\n'
    s += '}\n'
    return s
  def method_digest (self, mtype):
    d = mtype.type_hash()
    return ('0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL, ' +
            '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL') % d
  def generate_rpc_call_wrapper (self, class_info, mtype, mdefs):
    s = ''
    mth = mtype.type_hash()
    mname = ('%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x' +
             '%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x') % mth
    mdefs += [ '{ "_PLIC_%s", plic_pycall_%s_%s, METH_VARARGS, "pyRapicorn glue call" }' %
               (mname, class_info.name, mtype.name) ]
    hasret = mtype.rtype.storage != Decls.VOID
    s += 'static PyObject*\n'
    s += 'plic_pycall_%s_%s (PyObject *pyself, PyObject *pyargs)\n' % (class_info.name, mtype.name)
    s += '{\n'
    s += '  PyObject *item%s;\n' % (', *pyfoR = NULL' if hasret else '')
    s += '  ' + FieldBuffer + ' &fb = *' + FieldBuffer + '::_new (4 + 1 + %u), *fr = NULL;\n' % len (mtype.args) # proc_id self
    s += '  fb.add_type_hash (%s); // proc_id\n' % self.method_digest (mtype)
    s += '  if (PyTuple_Size (pyargs) != 1 + %u) ERRORpy ("PLIC: wrong number of arguments");\n' % len (mtype.args) # proc_id self
    arg_counter = 0
    s += '  item = PyTuple_GET_ITEM (pyargs, %d);  // self\n' % arg_counter
    s += self.generate_proto_add_py ('fb', class_info, 'item')
    arg_counter += 1
    for ma in mtype.args:
      s += '  item = PyTuple_GET_ITEM (pyargs, %d); // %s\n' % (arg_counter, ma[0])
      s += self.generate_proto_add_py ('fb', ma[1], 'item')
      arg_counter += 1
    s += '  fr = plic_call_remote (&fb); // deletes fb\n'
    if mtype.rtype.storage == Decls.VOID:
      s += '  if (fr) { delete fr; fr = NULL; }\n'
      s += '  return None_INCREF();\n'
    else:
      s += '  ERRORifnotret (fr);\n'
      s += '  if (fr) {\n'
      s += '    ' + FieldBuffer + 'Reader frr (*fr);\n'
      s += '    frr.skip(); // proc_id for return\n'
      s += '    if (frr.remaining() == 1) {\n'
      s += reindent ('      ', self.generate_proto_pop_py ('frr', mtype.rtype, 'pyfoR')) + '\n'
      s += '    }\n'
      s += '    delete fr; fr = NULL;\n'
      s += '  }\n'
      s += '  return pyfoR;\n'
    s += ' error:\n'
    s += '  if (fr) delete fr;\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def type2cpp (self, typename):
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    return typename
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by Rapicorn-CxxPyStub --- */\n'
    s += base_code + '\n'
    self.tabwidth (16)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate prototypes
    for tp in types:
      if tp.typedef_origin:
        pass # s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin.name), tp.name)
      elif tp.storage == Decls.RECORD:
        s += 'struct %s;\n' % tp.name
      elif tp.storage == Decls.ENUM:
        s += self.generate_enum_impl (tp) + '\n'
    # generate types
    for tp in types:
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        s += self.generate_record_impl (tp) + '\n'
      elif tp.storage == Decls.SEQUENCE:
        s += self.generate_sequence_impl (tp) + '\n'
      elif tp.storage == Decls.INTERFACE:
        s += ''
    # generate accessors
    mdefs = []
    for tp in types:
      if tp.typedef_origin:
        pass
      elif tp.storage == Decls.RECORD:
        pass
      elif tp.storage == Decls.SEQUENCE:
        pass
      elif tp.storage == Decls.INTERFACE:
        pass
        for m in tp.methods:
          s += self.generate_rpc_call_wrapper (tp, m, mdefs)
    # method def array
    if mdefs:
      s += '#define PLIC_PYSTUB_METHOD_DEFS() \\\n  ' + ',\\\n  '.join (mdefs) + '\n'
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  gg = Generator()
  textstring = gg.generate_impl_types (config['implementation_types'])
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']