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

_IFACE = '_Iface'
base_code = r"""
// #include <rcore/rapicornsignal.hh>
// #include <ui/interface.hh>
#include <rapicorn.hh>
#include <rcore/plicutils.hh>

#include <string>
#include <vector>
#include <stdexcept>

#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define __UNUSED__      __attribute__ ((__unused__))
#else
#define __UNUSED__
#endif

namespace { // Anonymous
using Plic::uint64;

class PlicProxyBase {
  uint64 m_locator_id;
public:
  explicit PlicProxyBase (uint64 locator_id) : m_locator_id (locator_id) {}
  virtual ~PlicProxyBase() {}
};

// FIXME:
typedef Plic::FieldBuffer FieldBuffer;
typedef Plic::FieldBuffer8 FieldBuffer8;
typedef Plic::FieldBufferReader FieldBufferReader;
template<class CLASS> static inline uint64
Instance2IdCast (CLASS *obj)
{
  if (!obj) return 0;
  else return dynamic_cast<const Rapicorn::Locatable*> (obj)->locatable_id();
}
template<class CLASS> static uint64
Instance2IdCast (const CLASS *obj)
{
  if (!obj) return 0;
  else return dynamic_cast<const Rapicorn::Locatable*> (obj)->locatable_id();
}
template<class CLASS> static inline uint64
Instance2IdCast (CLASS &obj)
{
  if (!&obj) return 0;
  else return dynamic_cast<const Rapicorn::Locatable*> (&obj)->locatable_id();
}
template<class CLASS> static inline CLASS*
Instance4IdCast (uint64 locator_id)
{
  Rapicorn::Locatable *dobj = Rapicorn::Locatable::from_locatable_id (locator_id);
  CLASS *target = dobj ? dynamic_cast<CLASS*> (dobj) : NULL;
  if (!target)
    printerr ("NULL-CAST: 0x%016llx -> %p -> %p\n", locator_id, dobj, target); // FIXME
  return target;
}
#define die()      (void) 0 // FIXME
#define THROW_ERROR()   throw std::runtime_error ("PLIC: Marshalling failed")
#define rope_check(cond,errmsg) do { if (!(cond)) { Rapicorn::printerr ("ROPE:error: %s\n", errmsg); return false; } } while (0)

static FieldBuffer* plic_call_remote (FieldBuffer*);
#ifndef HAVE_PLIC_CALL_REMOTE
static FieldBuffer* plic_call_remote (FieldBuffer *fb)
{ delete fb; return NULL; } // testing stub
#define HAVE_PLIC_CALL_REMOTE
#endif

} // Anonymous
"""

FieldBuffer = 'Plic::FieldBuffer'

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
  def namespaced_identifier (self, ident):
    return '::'.join ([d.name for d in self.namespaces] + [ident])
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
  def format_vartype (self, type, _iface):
    s = ''
    s += self.type2cpp (type)
    if type.storage == Decls.INTERFACE:
      s += _iface + ' *'
    else:
      s += ' '
    return s
  def format_var (self, ident, type, _iface):
    return self.format_vartype (type, _iface) + ident
  def format_arg (self, ident, type, ipostfix = '', defaultinit = None):
    s = ''
    constref = type.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD)
    if constref:
      s += 'const '                             # <const >int foo = 3
    s += self.type2cpp (type)                   # const <int> foo = 3
    if type.storage == Decls.INTERFACE:
      s += ipostfix                             # Object<_Iface> &self
    s += ' '                                    # const int< >foo = 3
    if type.storage == Decls.INTERFACE:
      s += '&'                                  # Object_Iface <&>self
    if constref:
      s += '&'                                  # const String <&>foo = "bar"
    s += ident                                  # const int <foo> = 3
    if defaultinit != None:
      if type.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.type2cpp (type), defaultinit)
      elif type.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.type2cpp (type)
      elif type.storage == Decls.INTERFACE:
        s += ' = *(%s%s*) NULL' % (self.type2cpp (type), ipostfix)
      else:
        s += ' = %s' % defaultinit
    return s
  def use_arg (self, ident, type, interfacechar = '*'):
    s = ''
    if type.storage == Decls.INTERFACE:
      s += interfacechar
    s += ident
    return s
  def generate_prop (self, fident, ftype): # FIXME: properties
    v = 'virtual '
    # getter
    s = '  ' + self.format_to_tab (v + ftype.name) + fident + ' () const = 0;\n'
    # setter
    s += '  ' + self.format_to_tab (v + 'void') + fident + ' (const &' + ftype.name + ') = 0;\n'
    return s
  def generate_proplist (self, ctype): # FIXME: properties
    return '  ' + self.format_to_tab ('virtual const PropertyList&') + 'list_properties ();\n'
  def generate_field (self, fident, ftype_name):
    return '  ' + self.format_to_tab (ftype_name) + fident + ';\n'
  def generate_signal_name (self, functype, ctype):
    return 'Signal_%s' % functype.name
  def generate_sigdef (self, functype, ctype):
    signame = self.generate_signal_name (functype, ctype)
    s = ''
    cpp_rtype = self.rtype2cpp (functype.rtype)
    rtiface = _IFACE if functype.rtype.storage == Decls.INTERFACE else ''
    s += '  typedef Rapicorn::Signals::Signal<%s%s, %s%s (' % (ctype.name, _IFACE, cpp_rtype, rtiface)
    l = []
    for a in functype.args:
      l += [ self.format_arg (a[0], a[1], _IFACE) ]
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
  def generate_recseq_decl (self, type_info):
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
    if type_info.storage == Decls.RECORD:
      s += '  bool proto_add  (' + FieldBuffer + '&) const;\n'
      s += '  bool proto_pop  (' + FieldBuffer + 'Reader&);\n'
      s += '  inline %s () {' % type_info.name
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};'
    return s
  def accessor_name (self, decls_type):
    # map Decls storage to FieldBuffer accessors
    return { Decls.INT:       'int64',
             Decls.ENUM:      'evalue',
             Decls.FLOAT:     'double',
             Decls.STRING:    'string',
             Decls.FUNC:      'func',
             Decls.INTERFACE: 'object' }.get (decls_type, None)
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix,
                               onerr = 'return false'):
    s = ''
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_add (%s)) %s;\n' % (ident, fb, onerr)
      elif type.storage == Decls.INTERFACE:
        if self.gen_clientcc:
          s += '  %s.add_object (Instance2IdCast (%s));\n' % (fb, ident)
        else:
          s += '  %s.add_object (Instance2IdCast (%s));\n' % (fb, ident)
      else:
        s += '  %s.add_%s (%s);\n' % (fb, self.accessor_name (type.storage), ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix, _iface = '',
                               onerr = 'return false'):
    s = ''
    for arg_it in arg_info_list:
      ident, type = arg_it
      ident = aprefix + ident + apostfix
      if type.storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  if (!%s.proto_pop (%s)) %s;\n' % (ident, fbr, onerr)
      elif type.storage == Decls.ENUM:
        s += '  %s = %s (%s.pop_evalue());\n' % (ident, self.type2cpp (type), fbr)
      elif type.storage == Decls.INTERFACE:
        s += '  %s = Instance4IdCast<%s%s> (%s.pop_%s());\n' \
            % (ident, type.name, _iface, fbr, self.accessor_name (type.storage))
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
      s += '    %s.push_back (Instance4IdCast<%s> (fbr.pop_%s()));\n' % (eident, el[1].name, self.accessor_name (el[1].storage))
    else:
      s += '    %s.push_back (fbr.pop_%s());\n' % (eident, self.accessor_name (el[1].storage))
    s += '  }\n'
    s += '  return true;\n'
    s += '}\n'
    return s
  def generate_client_method_stub (self, class_info, mtype):
    s = ''
    therr = 'THROW_ERROR()'
    hasret = mtype.rtype.storage != Decls.VOID
    interfacechar = '*' if mtype.rtype.storage == Decls.INTERFACE else ''
    # prototype
    s += '%s%s\n' % (self.rtype2cpp (mtype.rtype), interfacechar)
    q = '%s::%s (' % (class_info.name, mtype.name)
    s += q
    argindent = len (q)
    l = []
    for a in mtype.args:
      l += [ self.format_arg ('arg_' + a[0], a[1]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    # vars, procedure
    rdecl = ', *fr = NULL' if hasret else ''
    s += '  FieldBuffer &fb = *FieldBuffer::_new (4 + 1 + %u)%s;\n' % (len (mtype.args), rdecl)
    s += '  fb.add_type_hash (%s); // proc_id\n' % self.method_digest (mtype)
    # marshal args
    s += self.generate_proto_add_args ('fb', class_info, '', [('this', class_info)], '')
    ident_type_args = [('arg_' + a[0], a[1]) for a in mtype.args]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '', therr)
    # call out
    s += '  fr = ' if hasret else '  '
    s += 'plic_call_remote (&fb); // deletes fb\n'
    # unmarshal return
    if hasret:
      rarg = ('retval', mtype.rtype)
      s += '  FieldBufferReader frr (*fr);\n'
      s += '  frr.skip(); // proc_id\n'
      # FIXME: check return error and return type
      if rarg[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var (rarg[0], rarg[1], '') + ';\n'
        s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '', '', therr)
      else:
        vtype = self.format_vartype (rarg[1], '') # 'int*' + ...
        s += self.generate_proto_pop_args ('frr', class_info, vtype, [rarg], '', '', therr) # ... + 'x = 5;'
      s += '  return retval;\n'
    s += '}\n'
    return s
  def generate_server_method_dispatcher (self, class_info, mtype, reglines):
    s = ''
    dispatcher_name = '_dispatch__%s_%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static FieldBuffer*\n'
    s += dispatcher_name + ' (const FieldBuffer &fb)\n'
    s += '{\n'
    s += '  ' + FieldBuffer + 'Reader fbr (fb);\n'
    s += '  fbr.skip4(); // TypeHash\n'
    s += '  if (fbr.remaining() != 1 + %u) return false;\n' % len (mtype.args)
    # fetch self
    s += '  %s%s *self;\n' % (self.type2cpp (class_info), _IFACE)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)], '', _IFACE)
    s += '  rope_check (self, "self must be non-NULL");\n'
    # fetch args
    for a in mtype.args:
      if a[1].storage in (Decls.RECORD, Decls.SEQUENCE):
        s += '  ' + self.format_var ('arg_' + a[0], a[1], _IFACE) + ';\n'
        s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])], '', _IFACE)
      else:
        tstr = self.format_vartype (a[1], _IFACE) + 'arg_'
        s += self.generate_proto_pop_args ('fbr', class_info, tstr, [(a[0], a[1])], '', _IFACE)
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.format_vartype (mtype.rtype, _IFACE) + 'rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.use_arg ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      s += '  FieldBuffer &rb  = *FieldBuffer::new_return();\n'
      s += self.generate_proto_add_args ('rb', class_info, '', [('rval', mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def digest2cbytes (self, digest):
    return ('0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL, ' +
            '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL') % \
            digest
  def method_digest (self, mtype):
    return self.digest2cbytes (mtype.type_hash())
  def generate_server_method_registry (self, reglines):
    s = ''
    s += 'static const Plic::DispatcherEntry _dispatcher_entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { { ' + cdigest + ' }, '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static Plic::DispatcherRegistry _dispatcher_registry (_dispatcher_entries);\n'
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
  def generate_method_decl (self, functype, pad, gen4server):
    s = ''
    _iface = _IFACE if gen4server else ''
    s += '  ' + ('virtual ' if gen4server else '')
    if functype.rtype.storage == Decls.INTERFACE:
      rtpostfix = _iface + '*' if gen4server else '*'
    else:
      rtpostfix = ''
    s += self.format_to_tab (self.rtype2cpp (functype.rtype) + rtpostfix)
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.format_arg (a[0], a[1], _iface, a[2]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if functype.pure and gen4server:
      s += ' = 0'
    s += ';\n'
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    if functype.pure:
      return ''
    fs = type_info.name + _IFACE + '::' + functype.name
    tnsl = type_info.list_namespaces() # type namespace list
    absname = '::'.join ([n.name for n in tnsl] + [ fs ])
    if absname in self.skip_symbols:
      return ''
    s = self.open_namespace (type_info)
    rtpostfix = _IFACE + '*' if functype.rtype.storage == Decls.INTERFACE else ''
    sret = self.rtype2cpp (functype.rtype) + rtpostfix + '\n'
    fs += ' ('
    argindent = len (fs)
    s += '\n' + sret + fs
    l = []
    for a in functype.args:
      l += [ self.format_arg (a[0], a[1], _IFACE) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    if functype.rtype.storage == Decls.VOID:
      pass
    elif functype.rtype.storage == Decls.ENUM:
      s += '  return %s (0); // FIXME\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += '  return %s(); // FIXME\n' % self.rtype2cpp (functype.rtype)
    elif functype.rtype.storage == Decls.INTERFACE:
      s += '  return (%s%s*) NULL; // FIXME\n' % (self.rtype2cpp (functype.rtype), _IFACE)
    else:
      s += '  return 0; // FIXME\n'
    s += '}\n'
    return s
  def generate_interface_decl (self, type_info, gen4server):
    s = ''
    _iface = _IFACE if gen4server else ''
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    if gen4server:
      l = ['public virtual ' + pr.name + _iface for pr in l] # types -> names
    else:
      l = ['public ' + pr.name for pr in l] # types -> names
      if not l:
        l = ['protected PlicProxyBase']
    s += 'class %s%s' % (type_info.name, _iface)
    if l:
      s += ' : %s' % ', '.join (l)
    s += ' {\n'
    if gen4server:
      s += 'protected:\n'
      s += '  virtual ' + self.format_to_tab ('/*Des*/') + '~%s%s () = 0;\n' % (type_info.name, _iface)
    else:
      pass # s += '  ' + self.format_to_tab ('virtual /*Des*/') + '~%s%s ();\n' % (type_info.name, _iface)
    s += 'public:\n'
    if gen4server:
      for sg in type_info.signals:
        s += self.generate_sigdef (sg, type_info)
      for sg in type_info.signals:
        s += self.generate_signal (sg, type_info)
    ml = 0
    if type_info.methods:
      ml = max (len (m.name) for m in type_info.methods)
    for m in type_info.methods:
      s += self.generate_method_decl (m, ml, gen4server)
    s += self.insertion_text ('class_scope:' + type_info.name + _iface)
    s += '};'
    return s
  def generate_interface_impl (self, type_info):
    s = ''
    tname = type_info.name + _IFACE
    s += '%s::~%s () {}\n' % (tname, tname)
    return s
  def generate_interface_skel (self, type_info):
    s = ''
    for m in type_info.methods:
      s += self.generate_virtual_method_skel (m, type_info)
    return s
  def generate_enum_decl (self, type_info):
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
    if text:
      ind = '  ' if key.startswith ('class_scope:') else '' # inner class indent
      return ind + '// ' + key + ':\n' + text + '\n'
    else:
      return ''
  def insertion_file (self, filename):
    f = open (filename, 'r')
    key = None
    for line in f:
      m = (re.match ('(class_scope:\w+):\s*(//.*)?$', line) or
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
    # inclusions
    for i in self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
      s += '#include %s\n' % i
    s += self.insertion_text ('global_scope')
    s += '\n// --- Boilerplate ---\n'
    if self.gen_servercc:
      s += base_code + '\n'
    self.tabwidth (16)
    s += self.open_namespace (None)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      s += '\n// --- Interfaces (class declarations) ---\n'
      for tp in types:
        s += self.open_namespace (tp)
        if tp.typedef_origin:
          s += 'typedef %s %s;\n' % (self.type2cpp (tp.typedef_origin), tp.name)
        elif tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.generate_recseq_decl (tp) + '\n'
        elif tp.storage == Decls.ENUM:
          s += self.generate_enum_decl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          s += '\n'
          if self.gen_clienthh:
            s += self.generate_interface_decl (tp, False) + '\n'
          if self.gen_serverhh:
            s += self.generate_interface_decl (tp, True) + '\n'
      s += self.open_namespace (None)
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.typedef_origin:
          continue
        if tp.storage == Decls.RECORD:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp) + '\n'
        elif tp.storage == Decls.SEQUENCE:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp) + '\n'
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          if self.gen_servercc:
            s += self.generate_interface_impl (tp) + '\n'
          if self.gen_clientcc:
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
    # generate unmarshalling server calls
    if self.gen_servercc:
      s += '\n// --- Method Dispatchers & Registry ---\n'
      reglines = []
      for tp in types:
        if tp.typedef_origin:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE:
          for m in tp.methods:
            s += self.generate_server_method_dispatcher (tp, m, reglines)
          s += '\n'
      s += self.generate_server_method_registry (reglines) + '\n'
      s += self.open_namespace (None)
    # generate interface method skeletons
    if self.gen_server_skel:
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
  gg.gen_serverhh = all or 'serverhh' in config['backend-options']
  gg.gen_servercc = all or 'servercc' in config['backend-options']
  gg.gen_server_skel = 'server-skel' in config['backend-options']
  gg.gen_clienthh = all or 'clienthh' in config['backend-options']
  gg.gen_clientcc = all or 'clientcc' in config['backend-options']
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt.startswith ('iface-postfix='):
      global _IFACE
      _IFACE = opt[14:]
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
