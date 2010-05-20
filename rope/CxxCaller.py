#!/usr/bin/env python
# PLIC-CxxCaller                                               -*-mode:python-*-
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
"""PLIC-CxxCaller - C++ Call glue generator

More details at http://www.testbit.eu/PLIC
"""
import Decls, GenUtils, re

base_code = r"""
// #include <rcore/rapicornsignal.hh>
// #include <ui/interface.hh>
#include <rapicorn.hh>
#include <ui/protocol-pb2.hh>
#include <rcore/plicutils.hh>

#include <string>
#include <vector>

#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define __UNUSED__      __attribute__ ((__unused__))
#else
#define __UNUSED__
#endif

namespace { // Anonymous

// FIXME:
typedef Rapicorn::ProtoRecord ProtoRecord;
typedef Rapicorn::ProtoSequence ProtoSequence;
typedef Rapicorn::ProtoArg ProtoArg;
typedef Rapicorn::ProtoMessage ProtoMessage;
typedef Rapicorn::Rope::RemoteProcedure RemoteProcedure;
typedef Rapicorn::Rope::RemoteProcedure_Sequence RemoteProcedure_Sequence;
typedef Rapicorn::Rope::RemoteProcedure_Record RemoteProcedure_Record;
typedef Rapicorn::Rope::RemoteProcedure_Argument RemoteProcedure_Argument;
static inline RemoteProcedure_Record&
RPRecordCast (ProtoRecord &r)
{
  return *(RemoteProcedure_Record*) &r;
}
static inline const RemoteProcedure_Record&
RPRecordCast (const ProtoRecord &r)
{
  return *(const RemoteProcedure_Record*) &r;
}
static inline ProtoRecord&
ProtoRecordCast (RemoteProcedure_Record &r)
{
  return *(ProtoRecord*) &r;
}
static inline const ProtoRecord&
ProtoRecordCast (const RemoteProcedure_Record &r)
{
  return *(const ProtoRecord*) &r;
}
static inline RemoteProcedure_Sequence&
RPSequenceCast (ProtoSequence &r)
{
  return *(RemoteProcedure_Sequence*) &r;
}
static inline const RemoteProcedure_Sequence&
RPSequenceCast (const ProtoSequence &r)
{
  return *(const RemoteProcedure_Sequence*) &r;
}
static inline ProtoSequence&
ProtoSequenceCast (RemoteProcedure_Sequence &r)
{
  return *(ProtoSequence*) &r;
}
static inline const ProtoSequence&
ProtoSequenceCast (const RemoteProcedure_Sequence &r)
{
  return *(const ProtoSequence*) &r;
}
template<class CLASS> static inline std::string
Instance2StringCast (CLASS *obj)
{
  return dynamic_cast<const Rapicorn::Deletable*> (obj)->object_url();
}
template<class CLASS> static inline std::string
Instance2StringCast (const CLASS &obj)
{
  return dynamic_cast<const Rapicorn::Deletable*> (&obj)->object_url();
}
template<class CLASS> static inline CLASS*
Instance4StringCast (const std::string &objstring)
{
  struct _DeletableWrapper : public Rapicorn::Deletable {
    static Deletable* _from_object_url (const std::string &ou) { return from_object_url (ou); }
  };
  Rapicorn::Deletable *dobj = _DeletableWrapper::_from_object_url (objstring);
  CLASS *target = dobj ? dynamic_cast<CLASS*> (dobj) : NULL;
  if (!target)
    printerr ("NULL-CAST: %s -> %p -> %p\n", objstring.c_str(), dobj, target); // FIXME
  return target;
}
#define die()      (void) 0 // FIXME
#define rope_check(cond,errmsg) do { if (!(cond)) { Rapicorn::printerr ("ROPE:error: %s\n", errmsg); return false; } } while (0)

} // Anonymous
"""

FieldBuffer = 'Rapicorn::Plic::FieldBuffer'

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

class Generator:
  def __init__ (self):
    self.ntab = 26
    self.namespaces = []
    self.insertions = {}
    self.gen_inclusions = []
    self.skip_symbols = set()
  def close_inner_namespace (self):
    return '} // %s\n' % self.namespaces.pop().name
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    return '\nnamespace %s {\n' % self.namespaces[-1].name
  def open_namespace (self, type):
    s = ''
    newspaces = []
    if type:
      newspaces = type.list_namespaces()
      # s += '// ' + str ([n.name for n in newspaces]) + '\n'
    while len (self.namespaces) > len (newspaces):
      s += self.close_inner_namespace()
    while self.namespaces and newspaces[0:len (self.namespaces)] != self.namespaces:
      s += self.close_inner_namespace()
    for n in newspaces[len (self.namespaces):]:
      s += self.open_inner_namespace (n)
    return s
  def type_relative_namespaces (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    # remove common prefix with global namespace list
    for n in self.namespaces:
      if tnsl and tnsl[0] == n:
        tnsl = tnsl[1:]
      else:
        break
    namespace_names = [d.name for d in tnsl]
    return namespace_names
  def rtype2cpp (self, type_node):
    return self.type2cpp (type_node)
  def type2cpp (self, type_node):
    typename = type_node.name
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def tabwidth (self, n):
    self.ntab = n
  def format_to_tab (self, string, indent = ''):
    if len (string) >= self.ntab:
      return indent + string + ' '
    else:
      f = '%%-%ds' % self.ntab  # '%-20s'
      return indent + f % string
  def format_vartype (self, type, interfacechar = '&'):
    s = ''
    s += self.type2cpp (type) + ' '
    if type.storage == Decls.INTERFACE:
      s += interfacechar
    return s
  def format_var (self, ident, type, interfacechar = '&'):
    return self.format_vartype (type, interfacechar) + ident
  def format_arg (self, ident, type, defaultinit, interfacechar = '&'):
    return self.format_default_arg (ident, type, None, interfacechar)
  def format_default_arg (self, ident, type, defaultinit, interfacechar = '&'):
    s = ''
    constref = type.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD)
    if constref:
      s += 'const '
    s += self.type2cpp (type) + ' '
    if type.storage == Decls.INTERFACE:
      s += interfacechar
    if constref:
      s += '&'
    s += ident
    if defaultinit != None:
      if type.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.type2cpp (type), defaultinit)
      elif type.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.type2cpp (type)
      elif type.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.type2cpp (type)
      else:
        s += ' = %s' % defaultinit
    return s
  def use_arg (self, ident, type, interfacechar = '*'):
    s = ''
    if type.storage == Decls.INTERFACE:
      s += interfacechar
    s += ident
    return s
  def generate_prop (self, fident, ftype):
    v = 'virtual '
    # getter
    s = '  ' + self.format_to_tab (v + ftype.name) + fident + ' () const = 0;\n'
    # setter
    s += '  ' + self.format_to_tab (v + 'void') + fident + ' (const &' + ftype.name + ') = 0;\n'
    return s
  def generate_proplist (self, ctype):
    return '  ' + self.format_to_tab ('virtual const PropertyList&') + 'list_properties ();\n'
  def generate_field (self, fident, ftype_name):
    return '  ' + self.format_to_tab (ftype_name) + fident + ';\n'
  def generate_signal_name (self, functype, ctype):
    return 'Signal_%s' % functype.name
  def generate_sigdef (self, functype, ctype):
    signame = self.generate_signal_name (functype, ctype)
    s = ''
    cpp_rtype = self.rtype2cpp (functype.rtype)
    s += '  typedef Rapicorn::Signals::Signal<%s, %s (' % (self.type2cpp (ctype), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.format_arg (*a) ]
    s += ', '.join (l)
    s += ')'
    if functype.rtype.collector != 'void':
      s += ', Rapicorn::Signals::Collector' + functype.rtype.collector.capitalize()
      s += '<' + cpp_rtype + '> '
    s += '> ' + signame + ';\n'
    return s
  def mkzero (self, type):
    if type.storage == Decls.ENUM:
      return self.type2cpp (type) + ' (0)'
    return '0'
  def generate_record_interface (self, type_info):
    s = ''
    s += 'struct %s {\n' % type_info.name
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += self.generate_field (fl[0], self.type2cpp (fl[1]))
    elif type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += self.generate_field (fl[0], 'std::vector<' + self.type2cpp (fl[1]) + '>')
      s += '  bool proto_add  (' + FieldBuffer + '&) const;\n'
      s += '  bool proto_pop  (' + FieldBuffer + 'Reader&);\n'
      s += '  bool to_proto   (ProtoSequence &) const;\n'
      s += '  bool from_proto (const ProtoSequence &);\n'
    if type_info.storage == Decls.RECORD:
      s += '  bool proto_add  (' + FieldBuffer + '&) const;\n'
      s += '  bool proto_pop  (' + FieldBuffer + 'Reader&);\n'
      s += '  bool to_proto   (ProtoRecord &) const;\n'
      s += '  bool from_proto (const ProtoRecord &);\n'
      s += '  inline %s () {' % type_info.name
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += self.insertion_text (type_info.name)
    s += '};'
    return s
  def generate_to_proto (self, argname, type_info, valname, onerror = 'return false'):
    s = ''
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage in (Decls.INT, Decls.ENUM):
      s += '  %s->set_vint64 (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.FLOAT:
      s += '  %s->set_vdouble (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.STRING:
      s += '  %s->set_vstring (%s);\n' % (argname, valname)
    elif type_info.storage == Decls.INTERFACE:
      s += '  %s->set_vstring (Instance2StringCast (%s));\n' % (argname, valname)
    elif type_info.storage == Decls.RECORD:
      s += '  if (!%s.to_proto (ProtoRecordCast (*%s->mutable_vrec()))) %s;\n' % (valname, argname, onerror)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  if (!%s.to_proto (ProtoSequenceCast (*%s->mutable_vseq()))) %s;\n' % (valname, argname, onerror)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def generate_from_proto (self, argname, type_info, valname):
    s = ''
    if type_info.storage == Decls.VOID:
      pass
    elif type_info.storage == Decls.INT:
      s += '  if (!%s->has_vint64()) return false;\n' % argname
      s += '  %s = %s->vint64();\n' % (valname, argname)
    elif type_info.storage == Decls.ENUM:
      s += '  if (!%s->has_vint64()) return false;\n' % argname
      s += '  %s = %s (%s->vint64());\n' % (valname, self.type2cpp (type_info), argname)
    elif type_info.storage == Decls.FLOAT:
      s += '  if (!%s->has_vdouble()) return false;\n' % argname
      s += '  %s = %s->vdouble();\n' % (valname, argname)
    elif type_info.storage == Decls.STRING:
      s += '  if (!%s->has_vstring()) return false;\n' % argname
      s += '  %s = %s->vstring();\n' % (valname, argname)
    elif type_info.storage in Decls.INTERFACE:
      s += '  if (!%s->has_vstring()) return false;\n' % argname
      s += '  %s = Instance4StringCast<%s> (%s->vstring());\n' % (valname, type_info.name, argname)
    elif type_info.storage == Decls.RECORD:
      s += '  if (!%s->has_vrec() || !%s.from_proto (ProtoRecordCast (%s->vrec()))) return false;\n' % (argname, valname, argname)
    elif type_info.storage == Decls.SEQUENCE:
      s += '  if (!%s->has_vseq() || !%s.from_proto (ProtoSequenceCast (%s->vseq()))) return false;\n' % (argname, valname, argname)
    else: # FUNC
      raise RuntimeError ("Unexpected storage type: " + type_info.storage)
    return s
  def storage_fieldname (self, storage):
    if storage in (Decls.INT, Decls.ENUM):
      return 'vint64'
    elif storage == Decls.FLOAT:
      return 'vdouble'
    elif storage in (Decls.STRING, Decls.INTERFACE):
      return 'vstring'
    elif storage == Decls.RECORD:
      return 'vrec'
    elif storage == Decls.SEQUENCE:
      return 'vseq'
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + storage)
  def accessor_name (self, decls_type):
    # map Decls storage to FieldBuffer accessors
    return { Decls.INT:       'int64',
             Decls.ENUM:      'evalue',
             Decls.FLOAT:     'double',
             Decls.STRING:    'string',
             Decls.FUNC:      'func',
             Decls.INTERFACE: 'object' }.get (decls_type, None)
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_add (%s)) return false;\n' % (ident, fb)
      elif type.storage == Decls.INTERFACE:
        s += '  %s.add_object (Instance2StringCast (%s));\n' % (fb, ident)
      else:
        s += '  %s.add_%s (%s);\n' % (fb, self.accessor_name (type.storage), ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_pop (%s)) return false;\n' % (ident, fbr)
      elif type.storage == Decls.ENUM:
        s += '  %s = %s (%s.pop_evalue());\n' % (ident, self.type2cpp (type), fbr)
      elif type.storage == Decls.INTERFACE:
        s += '  %s = Instance4StringCast<%s> (%s.pop_%s());\n' % (ident, type.name, fbr, self.accessor_name (type.storage))
      else:
        s += '  %s = %s.pop_%s();\n' % (ident, fbr, self.accessor_name (type.storage))
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += 'bool\n%s::proto_add (' % type_info.name + FieldBuffer + ' &dst) const\n{\n'
    s += '  ' + FieldBuffer + ' &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args ('fb', type_info, 'this->', type_info.fields, '')
    s += '  return true;\n'
    s += '}\n'
    s += 'bool\n%s::proto_pop (' % type_info.name + FieldBuffer + 'Reader &src)\n{\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() != %u) return false;\n' % len (type_info.fields)
    s += self.generate_proto_pop_args ('fbr', type_info, 'this->', type_info.fields, '')
    s += '  return true;\n'
    s += '}\n'
    s += 'bool\n%s::to_proto (ProtoRecord &dst) const\n{\n' % type_info.name
    s += '  RemoteProcedure_Record &rpr = RPRecordCast (dst);\n'
    s += '  RemoteProcedure_Argument *field;\n'
    for fl in type_info.fields:
      s += '  field = rpr.add_fields();\n'
      s += self.generate_to_proto ('field', fl[1], fl[0])
    s += '  return true;\n}\n'
    s += 'bool\n%s::from_proto (const ProtoRecord &src)\n{\n' % type_info.name
    s += '  const RemoteProcedure_Record &rpr = RPRecordCast (src);\n'
    s += '  if (rpr.fields_size() < %d) return false;\n' % len (type_info.fields)
    s += '  const RemoteProcedure_Argument *field;\n'
    field_counter = 0
    for fl in type_info.fields:
      s += '  field = &rpr.fields (%d);\n' % field_counter
      s += self.generate_from_proto ('field', fl[1], fl[0])
      field_counter += 1
    s += '  return true;\n}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    el = type_info.elements
    s += 'bool\n%s::proto_add (' % type_info.name + FieldBuffer + ' &dst) const\n{\n'
    s += '  const size_t len = %s.size();\n' % el[0]
    s += '  ' + FieldBuffer + ' &fb = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args ('fb', type_info, 'this->',
                                                       [type_info.elements], '[k]')) + '\n'
    s += '  }\n'
    s += '  return true;\n'
    s += '}\n'
    eident = 'this->%s' % el[0]
    s += 'bool\n%s::proto_pop (' % type_info.name + FieldBuffer + 'Reader &src)\n{\n'
    s += '  ' + FieldBuffer + 'Reader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s.resize (len);\n' % eident
    else:
      s += '  %s.reserve (len);\n' % eident
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += reindent ('  ', self.generate_proto_pop_args ('fbr', type_info,
                                                         'this->',
                                                         [type_info.elements], '[k]')) + '\n'
    elif el[1].storage == Decls.ENUM:
      s += '    %s.push_back (%s (fbr.pop_evalue()));\n' % (eident, self.type2cpp (el[1]))
    elif el[1].storage == Decls.INTERFACE:
      s += '    %s.push_back (Instance4StringCast<%s> (fbr.pop_%s()));\n' % (eident, el[1].name, self.accessor_name (el[1].storage))
    else:
      s += '    %s.push_back (fbr.pop_%s());\n' % (eident, self.accessor_name (el[1].storage))
    s += '  }\n'
    s += '  return true;\n'
    s += '}\n'
    s += 'bool\n%s::to_proto (ProtoSequence &dst) const\n{\n' % type_info.name
    s += '  RemoteProcedure_Sequence &rps = RPSequenceCast (dst);\n'
    s += '  const size_t len = %s.size();\n' % el[0]
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.INT, Decls.ENUM, Decls.FLOAT, Decls.STRING, Decls.INTERFACE):
      s += '    rps.add_%s (%s[k]);\n' % (self.storage_fieldname (el[1].storage), el[0])
    elif el[1].storage == Decls.RECORD:
      s += '    if (!%s[k].to_proto (ProtoRecordCast (*rps.add_vrec()))) return false;\n' % el[0]
    elif el[1].storage == Decls.SEQUENCE:
      s += '    if (!%s[k].to_proto (ProtoSequenceCast (*rps.add_vseq()))) return false;\n' % el[0]
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + el[1].storage)
    s += '  }\n'
    s += '  return true;\n}\n'
    s += 'bool\n%s::from_proto (const ProtoSequence &src)\n{\n' % type_info.name
    s += '  const RemoteProcedure_Sequence &rps = RPSequenceCast (src);\n'
    s += '  const size_t len = rps.%s_size();\n' % self.storage_fieldname (el[1].storage)
    if el[1].storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  %s.resize (len);\n' % el[0]
    s += '  for (size_t k = 0; k < len; k++) {\n'
    if el[1].storage in (Decls.INT, Decls.FLOAT, Decls.STRING, Decls.INTERFACE):
      s += '    %s.push_back (rps.%s (k));\n' % (el[0], self.storage_fieldname (el[1].storage))
    elif el[1].storage == Decls.ENUM:
      s += '    %s.push_back (%s (rps.%s (k)));\n' % \
           (el[0], self.type2cpp (el[1]), self.storage_fieldname (el[1].storage))
    elif el[1].storage == Decls.RECORD:
      s += '    if (!%s[k].from_proto (ProtoRecordCast (rps.vrec (k)))) return false;\n' % el[0]
    elif el[1].storage == Decls.SEQUENCE:
      s += '    if (!%s[k].from_proto (ProtoSequenceCast (rps.vseq (k)))) return false;\n' % el[0]
    else: # FUNC VOID
      raise RuntimeError ("Unexpected storage type: " + el[1].storage)
    s += '  }\n'
    s += '  return true;\n}\n'
    return s
  def generate_class_caller_impl (self, type_info):
    s = ''
    for m in type_info.methods:
      interfacechar = '*' if m.rtype.storage == Decls.INTERFACE else ''
      s += '%s%s\n' % (self.rtype2cpp (m.rtype), interfacechar)
      q = '%s::%s (' % (type_info.name, m.name)
      s += q
      argindent = len (q)
      l = []
      for a in m.args:
        l += [ self.format_arg (*a) ]
      s += (',\n' + argindent * ' ').join (l)
      s += ')\n{\n'
      s += '  RemoteProcedure rp;\n'
      s += '  rp.set_proc_id (0x%08x);\n' % GenUtils.type_id (m)
      if m.args:
        s += '  RemoteProcedure_Argument *arg;\n'
      for a in m.args:
        s += '  arg = rp.add_args();\n'
        s += self.generate_to_proto ('arg', a[1], a[0], 'die()')
      if m.rtype.storage == Decls.VOID:
        pass
      elif m.rtype.storage == Decls.ENUM:
        s += '  return %s (0); // FIXME\n' % self.rtype2cpp (m.rtype)
      elif m.rtype.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  return %s(); // FIXME\n' % self.rtype2cpp (m.rtype)
      elif m.rtype.storage == Decls.INTERFACE:
        s += '  return (%s*) NULL; // FIXME\n' % self.rtype2cpp (m.rtype)
      else:
        s += '  return 0; // FIXME\n'
      s += '}\n'
    return s
  def generate_class_call_wrapper (self, class_info, mtype, switchlines):
    s = ''
    switchlines += [ (GenUtils.type_id (mtype), class_info, mtype) ]
    s += 'static bool\n'
    call = 'call_%s_%s (const ' % (class_info.name, mtype.name)
    s += call + FieldBuffer + ' &fb, ' + FieldBuffer + ' &rb)\n'
    s += '{\n'
    s += '  ' + FieldBuffer + 'Reader fbr (fb);\n'
    s += '  fbr.skip(); // proc_id\n'
    s += '  if (fbr.remaining() != 1 + %u) return false;\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.type2cpp (class_info)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)], '')
    s += '  rope_check (self, "self must be non-NULL");\n'
    # fetch args
    for a in mtype.args:
      if a[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var ('arg_' + a[0], a[1], '*') + ';\n'
        s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])], '')
      else:
        tstr = self.format_vartype (a[1], '*') + 'arg_'
        s += self.generate_proto_pop_args ('fbr', class_info, tstr, [(a[0], a[1])], '')
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.format_vartype (mtype.rtype, '*') + 'rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.use_arg ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      s += '  rb.add_int64 (0x02000000); // proc_id for return value\n'
      s += self.generate_proto_add_args ('rb', class_info, '', [('rval', mtype.rtype)], '')
    # done
    s += '  return true;\n'
    s += '}\n'
    return s
  def generate_class_call_switch (self, switchlines):
    s = ''
    s += '\nstatic bool __UNUSED__\nplic_call_wrapper_switch (const '
    s += FieldBuffer + ' &fb, ' + FieldBuffer + ' &rb)\n'
    s += '{\n'
    s += '  switch (fb.first_id()) {\n'
    for swcase in switchlines:
      mid, type_info, m = swcase
      s += '  case 0x%08x: // %s::%s\n' % (mid, type_info.name, m.name)
      nsname = '::'.join (self.type_relative_namespaces (type_info))
      s += '    return %s::call_%s_%s (fb, rb);\n' % (nsname, type_info.name, m.name)
    s += '  default:\n'
    s += '    die();\n'
    s += '  }\n'
    s += '  return false;\n'
    s += '}\n'
    return s
  def generate_class_callee_impl (self, type_info, switchlines):
    s = ''
    for m in type_info.methods:
      switchlines += [ (GenUtils.type_id (m), type_info, m) ]
      hasret = m.rtype.storage != Decls.VOID
      s += 'static bool\n'
      c = 'handle_%s_%s ' % (type_info.name, m.name)
      s += c + '(const RemoteProcedure    &_rope_rp,\n'
      s += ' ' * len (c) + ' RemoteProcedure_Argument &_rope_aret)\n{\n'
      s += '  const RemoteProcedure_Argument *_rope_arg;\n'
      s += '  %s *self;\n' % self.type2cpp (type_info)
      for a in m.args:
        s += '  ' + self.format_var (a[0], a[1], '*') + ';\n'
      s += '  rope_check (_rope_rp.args_size() == %d, "invalid nuber of args");\n' % (1 + len (m.args))
      s += '  _rope_arg = &_rope_rp.args (0);\n'
      s += self.generate_from_proto ('_rope_arg', type_info, 'self')
      s += '  rope_check (self, "self must be non-NULL");\n'
      arg_counter = 1
      for arg in m.args:
        s += '  _rope_arg = &_rope_rp.args (%d);\n' % arg_counter
        s += self.generate_from_proto ('_rope_arg', arg[1], arg[0])
        arg_counter += 1
      s += '  '
      if hasret:
        interfacechar = '*' if m.rtype.storage == Decls.INTERFACE else ''
        s += '%s%s _rope_retval = ' % (self.rtype2cpp (m.rtype), interfacechar)
      s += 'self->' + m.name + ' ('
      s += ', '.join (self.use_arg (a[0], a[1]) for a in m.args)
      s += ');\n'
      if hasret:
        s += self.generate_to_proto ('(&_rope_aret)', m.rtype, '_rope_retval')
      s += '  return true;\n}\n'
    return s
  def generate_callee_impl (self, switchlines):
    s = ''
    s += '\nstatic bool __UNUSED__\nrope_callee_handler (const RemoteProcedure _rope_rp,\n'
    s += '                     RemoteProcedure_Argument &_rope_aret)\n{\n'
    s += '  switch (_rope_rp.proc_id()) {\n'
    for swcase in switchlines:
      mid, type_info, m = swcase
      s += '  case 0x%08x: // %s::%s\n' % (mid, type_info.name, m.name)
      nsname = '::'.join (self.type_relative_namespaces (type_info))
      s += '    return %s::handle_%s_%s (_rope_rp, _rope_aret);\n' % (nsname, type_info.name, m.name)
    s += '  default:\n'
    s += '    die();\n'
    s += '  }\n'
    s += '  return false;\n'
    s += '}\n'
    return s
  def generate_signal (self, functype, ctype):
    signame = self.generate_signal_name (functype, ctype)
    return '  // ' + self.format_to_tab (signame) + 'sig_%s;\n' % functype.name
  def inherit_reduce (self, type_list):
    def hasancestor (child, parent):
      if child == parent:
        return True
      for childpre in child.prerequisites:
        if hasancestor (childpre, parent):
          return True
    reduced = []
    while type_list:
      p = type_list.pop()
      skip = 0
      for c in type_list + reduced:
        if hasancestor (c, p):
          skip = 1
          break
      if not skip:
        reduced = [ p ] + reduced
    return reduced
  def generate_virtual_method (self, functype, pad = 0):
    s = ''
    interfacechar = '*' if functype.rtype.storage == Decls.INTERFACE else ''
    s += '  virtual ' + self.format_to_tab (self.rtype2cpp (functype.rtype) + interfacechar)
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.format_default_arg (*a) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if functype.pure:
      s += ' = 0'
    s += ';\n'
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    if functype.pure:
      return ''
    fs = type_info.name + '::' + functype.name
    tnsl = type_info.list_namespaces() # type namespace list
    absname = '::'.join ([n.name for n in tnsl] + [ fs ])
    if absname in self.skip_symbols:
      return ''
    s = self.open_namespace (type_info)
    interfacechar = '*' if functype.rtype.storage == Decls.INTERFACE else ''
    sret = self.rtype2cpp (functype.rtype) + interfacechar + '\n'
    fs += ' ('
    argindent = len (fs)
    s += '\n' + sret + fs
    l = []
    for a in functype.args:
      l += [ self.format_arg (*a) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    if functype.rtype.storage == Decls.VOID:
      pass
    elif functype.rtype.storage == Decls.ENUM:
      s += '  return %s (0); // FIXME\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  return %s(); // FIXME\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage == Decls.INTERFACE:
      s += '  return (%s*) NULL; // FIXME\n' % self.rtype2cpp (functype.rtype)
    else:
      s += '  return 0; // FIXME\n'
    s += '}\n'
    return s
  def generate_class_interface (self, type_info):
    s = ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    l = ['public virtual ' + pr.name for pr in l] # types -> names
    s += '\nclass %s' % type_info.name
    if l:
      s += ' : %s' % ', '.join (l)
    s += ' {\n'
    s += 'protected:\n'
    s += '  virtual ' + self.format_to_tab ('/*Des*/') + '~%s () = 0;\n' % type_info.name
    s += 'public:\n'
    for sg in type_info.signals:
      s += self.generate_sigdef (sg, type_info)
    for sg in type_info.signals:
      s += self.generate_signal (sg, type_info)
    ml = 0
    if type_info.methods:
      ml = max (len (m.name) for m in type_info.methods)
    for m in type_info.methods:
      s += self.generate_virtual_method (m, ml)
    s += self.insertion_text (type_info.name)
    s += '};'
    return s
  def generate_interface_impl (self, type_info):
    s = ''
    s += '%s::~%s () {}\n' % (type_info.name, type_info.name)
    return s
  def generate_interface_skel (self, type_info):
    s = ''
    for m in type_info.methods:
      s += self.generate_virtual_method_skel (m, type_info)
    return s
  def generate_enum_interface (self, type_info):
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
  def insertion_text (self, key):
    text = self.insertions.get (key, '')
    lstrip = re.compile ('^\n*')
    rstrip = re.compile ('\n*$')
    text = lstrip.sub ('', text)
    text = rstrip.sub ('', text)
    return text + ('\n' if text else '')
  def insertion_file (self, filename):
    f = open (filename, 'r')
    key = None
    for line in f:
      m = (re.match ('class_scope:(\w+):\s*(//.*)?$', line) or
           re.match ('(global_scope):\s*(//.*)?$', line))
      if not m:
        m = re.match ('(IGNORE):\s*(//.*)?$', line)
      if m:
        key = m.group (1)
        continue
      if key:
        block = self.insertions.get (key, '')
        block += line
        self.insertions[key] = block
  def symbol_file (self, filename):
    f = open (filename)
    txt = f.read()
    f.close()
    import re
    w = re.findall (r'([a-zA-Z_][a-zA-Z_0-9$:]*)', txt)
    self.skip_symbols.update (set (w))
  def generate_impl_types (self, implementation_types):
    s = '/* --- Generated by PLIC-CxxCaller --- */\n'
    s += self.insertion_text ('global_scope')
    if self.gen_server or self.gen_iface_impl:
      s += base_code + '\n'
    self.tabwidth (16)
    # inclusions
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    s += self.open_namespace (None)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate type skeletons
    if self.gen_iface:
      s += '\n// --- Interfaces ---\n'
      for tp in types:
        s += self.open_namespace (tp)
        if tp.typedef_origin:
          s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin), tp.name)
        elif tp.storage == Decls.RECORD:
          s += self.generate_record_interface (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE:
          s += self.generate_record_interface (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          s += self.generate_class_interface (tp) + '\n'
        elif tp.storage == Decls.ENUM:
          s += self.generate_enum_interface (tp) + '\n'
      s += self.open_namespace (None)
    # generate client stubs
    if self.gen_client:
      s += '\n// --- Client Stubs ---\n'
      for tp in types:
        if tp.typedef_origin:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.RECORD:
          s += self.generate_record_impl (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE:
          s += self.generate_sequence_impl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          s += self.generate_class_caller_impl (tp) + '\n'
    # generate server stubs
    if self.gen_server:
      s += '\n// --- Server Stubs ---\n'
      switchlinesO = []
      switchlines = []
      for tp in types:
        if tp.typedef_origin:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.RECORD:
          pass # s += self.generate_record_impl (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE:
          pass # s += self.generate_sequence_impl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          for m in tp.methods:
            s += self.generate_class_call_wrapper (tp, m, switchlines)
          s += '\n'
          s += self.generate_class_callee_impl (tp, switchlinesO) + '\n'
      s += self.open_namespace (None)
      s += self.generate_class_call_switch (switchlines) + '\n'
      s += self.generate_callee_impl (switchlinesO) + '\n'
    # generate interface impls
    if self.gen_iface_impl:
      s += '\n// --- Interface Implementation Helpers ---\n'
      for tp in types:
        if tp.typedef_origin:
          continue
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += self.generate_interface_impl (tp) + '\n'
        elif tp.storage == Decls.RECORD and not self.gen_client:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE and not self.gen_client:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp) + '\n'
    # generate interface method skeletons
    if self.gen_iface_skel:
      s += '\n// --- Interface Skeletons ---\n'
      for tp in types:
        if tp.typedef_origin:
          continue
        elif tp.storage == Decls.INTERFACE:
          s += self.generate_interface_skel (tp)
    s += self.open_namespace (None) # close all namespaces
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
  all = config['backend-options'] == []
  gg.gen_iface = all or 'interface' in config['backend-options']
  gg.gen_iface_impl = all or 'interface-impl' in config['backend-options']
  gg.gen_iface_skel = 'interface-skel' in config['backend-options']
  gg.gen_server = all or 'server' in config['backend-options']
  gg.gen_client = all or 'client' in config['backend-options']
  gg.gen_inclusions = config['inclusions']
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  for ssfile in config['skip-skels']:
    gg.symbol_file (ssfile)
  textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']
