# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
"""AidaCxxStub - Aida C++ Code Generator

More details at http://www.rapicorn.org/
"""
import Decls, GenUtils, TmplFiles, re, os

clienthh_boilerplate = r"""
// --- ClientHH Boilerplate ---
#include <rapicorn-core.hh>
"""

serverhh_boilerplate = r"""
// --- ServerHH Boilerplate ---
#include <rapicorn-core.hh>
"""

rapicornsignal_boilerplate = r"""
#include <rapicorn-core.hh> // for rcore/signal.hh
"""

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

I_prefix_postfix = ('', 'Iface')

class G4STUB: pass    # generate stub classes (remote handles)
class G4SERVANT: pass    # generate servants classes (interfaces)

class Generator:
  def __init__ (self, idl_file):
    assert len (idl_file) > 0
    self.ntab = 26
    self.namespaces = []
    self.insertions = {}
    self.idl_file = idl_file
    self.cppmacro = re.sub (r'(^[^A-Z_a-z])|([^A-Z_a-z0-9])', '_', self.idl_file)
    self.ns_aida = None
    self.gen_inclusions = []
    self.skip_symbols = set()
    self.iface_base = 'Rapicorn::Aida::ImplicitBase'
    self.property_list = 'Rapicorn::Aida::PropertyList'
    self.gen_mode = None
    self.strip_path = ""
  def warning (self, message, input_file = '', input_line = -1, input_col = -1):
    import sys
    if input_file:
      if input_line > 0 and input_col > 0:
        loc = '%s:%u:%u' % (input_file, input_line, input_col)
      elif input_line > 0:
        loc = '%s:%u' % (input_file, input_line)
      else:
        loc = input_file
    else:
      loc = self.idl_path()
    input_file = input_file if input_file else self.idl_path()
    print >>sys.stderr, '%s: WARNING: %s' % (loc, message)
  @staticmethod
  def prefix_namespaced_identifier (prefix, ident, postfix = ''):     # Foo::bar -> Foo::PREFIX_bar_POSTFIX
    cc = ident.rfind ('::')
    if cc >= 0:
      ns, base = ident[:cc+2], ident[cc+2:]
    else:
      ns, base = '', ident
    return ns + prefix + base + postfix
  def Iwrap (self, name):
    return self.prefix_namespaced_identifier (I_prefix_postfix[0], name, I_prefix_postfix[1])
  def type2cpp (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'std::string'
    if tstorage == Decls.ANY:           return 'Rapicorn::Aida::Any'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def C4server (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return self.Iwrap (tname)                         # construct servant class interface name
    elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return self.prefix_namespaced_identifier ('SrvT_', tname)
    return tname
  def C4client (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return tname + 'Handle'                           # construct client class RemoteHandle
    elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return self.prefix_namespaced_identifier ('ClnT_', tname)
    return tname
  def C (self, type_node, mode = None):                 # construct Class name
    mode = mode or self.gen_mode
    if mode == G4SERVANT:
      return self.C4server (type_node)
    else: # G4STUB
      return self.C4client (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      tname += 'P'
    return tname
  def M (self, type_node):                              # construct Member type
    if self.gen_mode == G4STUB and type_node.storage == Decls.INTERFACE:
      classH = self.C4client (type_node) # remote handle class name
      classC = self.C4server (type_node) # servant class name
      return 'Rapicorn::Aida::RemoteMember<%s>' % classH # classC
    else:
      return self.R (type_node)
  def V (self, ident, type_node, f_delta = -999999):    # construct Variable
    s = ''
    s += self.C (type_node)
    s = self.F (s, f_delta)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      s += '*'
    else:
      s += ' '
    return s + ident
  def A (self, ident, type_node, defaultinit = None):   # construct call Argument
    constref = type_node.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD, Decls.ANY)
    needsref = constref or type_node.storage == Decls.INTERFACE
    s = self.C (type_node)                      # const {Obj} &foo = 3
    s += ' ' if ident else ''                   # const Obj{ }&foo = 3
    if constref:
      s = 'const ' + s                          # {const }Obj &foo = 3
    if needsref:
      s += '&'                                  # const Obj {&}foo = 3
    s += ident                                  # const Obj &{foo} = 3
    if defaultinit != None:
      if type_node.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.C (type_node), defaultinit)  # { = 3}
      elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.C (type_node)                    # { = 3}
      elif type_node.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.C (type_node)             # { = 3}
      else:
        s += ' = %s' % defaultinit                             # { = 3}
    return s
  def Args (self, ftype, prefix, argindent = 2):        # construct list of call Arguments
    l = []
    for a in ftype.args:
      l += [ self.A (prefix + a[0], a[1]) ]
    return (',\n' + argindent * ' ').join (l)
  def U (self, ident, type_node):                       # construct function call argument Use
    s = '*' if type_node.storage == Decls.INTERFACE and self.gen_mode == G4SERVANT else ''
    return s + ident
  def F (self, string, delta = 0):                      # Format string to tab stop
    return string + ' ' * max (1, self.ntab + delta - len (string))
  def tab_stop (self, n):
    self.ntab = n
  def close_inner_namespace (self):
    current_namespace = self.namespaces.pop()
    return '} // %s\n' % current_namespace.name if current_namespace.name else ''
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    current_namespace = self.namespaces[-1]
    return '\nnamespace %s {\n' % current_namespace.name if current_namespace.name else ''
  def open_namespace (self, typeinfo):
    s = ''
    newspaces = []
    if type (typeinfo) == tuple:
      newspaces = list (typeinfo)
    elif typeinfo:
      newspaces = typeinfo.list_namespaces()
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
    namespace_names = [d.name for d in tnsl if d.name]
    return namespace_names
  def namespaced_identifier (self, ident):
    names = [d.name for d in self.namespaces if d.name]
    if ident:
      names += [ ident ]
    return '::'.join (names)
  def mkzero (self, type):
    if type.storage == Decls.STRING:
      return '""'
    if type.storage == Decls.ENUM:
      return self.C (type) + ' (0)'
    if type.storage in (Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      return self.C (type) + '()'
    return '0'
  def generate_recseq_decl (self, type_info):
    s = '\n'
    classFull = self.namespaced_identifier (type_info.name)
    # s += self.generate_shortdoc (type_info)   # doxygen IDL snippet
    if type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      #s += '/// @cond GeneratedRecords\n'
      s += 'struct ' + self.C (type_info) + ' : public std::vector<' + self.M (fl[1]) + '>\n'
      s += '{\n'
    else:
      #s += '/// @cond GeneratedSequences\n'
      s += 'struct %s\n' % self.C (type_info)
      s += '{\n'
    if type_info.storage == Decls.RECORD:
      s += '  /// @cond GeneratedFields\n'
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += '  ' + self.F (self.M (fl[1])) + fl[0] + ';\n'
      s += '  /// @endcond\n'
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.M (fl[1]) + '> Sequence;\n'
      s += '  reference append_back(); ///< Append data at the end, returns write reference to data.\n'
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('inline') + '%s () {' % self.C (type_info) # ctor
      for fl in fieldlist:
        if fl[1].storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += '  ' + self.F ('std::string') + '__aida_type_name__ () const\t{ return "%s"; }\n' % classFull
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('bool') + 'operator==  (const %s &other) const;\n' % self.C (type_info)
      s += '  ' + self.F ('bool') + 'operator!=  (const %s &other) const { return !operator== (other); }\n' % self.C (type_info)
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if type_info.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s&);\n' % self.C (type_info)
    #s += '/// @endcond\n'
    return s
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s <<= %s;\n' % (fb, ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix = ''):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s >>= %s;\n' % (fbr, ident)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += 'bool\n'
    s += '%s::operator== (const %s &other) const\n{\n' % (self.C (type_info), self.C (type_info))
    for field in type_info.fields:
      ident, type_node = field
      s += '  if (this->%s != other.%s) return false;\n' % (ident, ident)
    s += '  return true;\n'
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args ('fb', type_info, 'self.', type_info.fields, '')
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() < %u) return;\n' % len (type_info.fields)
    s += self.generate_proto_pop_args ('fbr', type_info, 'self.', type_info.fields)
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    el = type_info.elements
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  const size_t len = self.size();\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args ('fb', type_info, '',
                                                       [('self', type_info.elements[1])],
                                                       '[k]')) + '\n'
    s += '  }\n'
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    s += '  self.resize (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += '    fbr >>= self[k];\n'
    s += '  }\n'
    s += '}\n'
    s += '%s::reference\n' % self.C (type_info)
    s += '%s::append_back()\n{\n' % self.C (type_info)
    s += '  resize (size() + 1);\n'
    s += '  return back();\n'
    s += '}\n'
    return s
  def generate_enum_impl (self, type_info):
    s = '\n'
    enum_ns, enum_class = '::'.join (self.type_relative_namespaces (type_info)), type_info.name
    l = []
    s += 'template<> const EnumValue*\n'
    s += 'enum_value_list<%s::%s> () {\n' % (enum_ns, enum_class)
    s += '  static const EnumValue values[] = {\n'
    import TypeMap
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      # number = self.c_long_postfix (number)
      number = enum_ns + '::' + ident
      ident = TypeMap.cquote (ident)
      label = TypeMap.cquote (label)
      label = "NULL" if label == '""' else label
      blurb = TypeMap.cquote (blurb)
      blurb = "NULL" if blurb == '""' else blurb
      s += '    { %s,\t%s, %s, %s },\n' % (number, ident, label, blurb)
    s += '    { 0,\tNULL, NULL, NULL }     // sentinel\n  };\n'
    s += '  return values;\n}\n'
    s += 'template const EnumValue* enum_value_list<%s::%s> ();\n' % (enum_ns, enum_class)
    return s
  def digest2cbytes (self, digest):
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def method_digest (self, method_info):
    return self.digest2cbytes (method_info.type_hash())
  def class_digest (self, class_info):
    return self.digest2cbytes (class_info.type_hash())
  def list_types_digest (self, class_info):
    return self.digest2cbytes (class_info.twoway_hash ('__aida_typelist__ # internal-method'))
  def setter_digest (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), True)
    return self.digest2cbytes (setter_hash)
  def getter_digest (self, class_info, fident, ftype):
    getter_hash = class_info.property_hash ((fident, ftype), False)
    return self.digest2cbytes (getter_hash)
  def class_ancestry (self, type_info):
    def deep_ancestors (type_info):
      l = [ type_info ]
      for a in type_info.prerequisites:
        l += deep_ancestors (a)
      return l
    def make_list_uniq (lst):
      r, q = [], set()
      for e in lst:
        if not e in q:
          q.add (e)
          r += [ e ]
      return r
    return make_list_uniq (deep_ancestors (type_info))
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
  def interface_class_ancestors (self, type_info):
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    return l
  def interface_class_inheritance (self, type_info):
    aida_remotehandle, ddc = 'Rapicorn::Aida::RemoteHandle', False
    l = self.interface_class_ancestors (type_info)
    l = [self.C (pr) for pr in l] # types -> names
    if self.gen_mode == G4SERVANT:
      if l:
        heritage = 'public virtual'
      else:
        l, ddc = [self.iface_base], True
        heritage = 'public virtual'
    else:
      if l:
        heritage = 'public virtual'
      else:
        l, ddc = [aida_remotehandle], True
        heritage = 'public virtual'
    if self.gen_mode == G4SERVANT:
      cl = []
    else:
      cl = l if l == [aida_remotehandle] else [aida_remotehandle] + l
    return (l, heritage, cl, ddc) # prerequisites, heritage type, constructor args, direct-descendant (of ancestry root)
  def generate_interface_class (self, type_info, class_name_list):
    s, classC, classH, classFull = '\n', self.C (type_info), self.C4client (type_info), self.namespaced_identifier (type_info.name)
    class_name_list += [ classFull ]
    if self.gen_mode == G4SERVANT:
      s += 'class %s;\n' % classC
      s += 'typedef std::shared_ptr<%s> %sP;\n' % (classC, classC)
      s += 'typedef std::weak_ptr  <%s> %sW;\n' % (classC, classC)
      s += '\n'
    # declare
    s += self.generate_shortdoc (type_info)     # doxygen IDL snippet
    s += 'class %s' % classC
    # inherit
    precls, heritage, cl, ddc = self.interface_class_inheritance (type_info)
    s += ' : ' + heritage + ' %s' % (', ' + heritage + ' ').join (precls) + '\n'
    s += '{\n'
    if self.gen_mode == G4STUB:
      for sg in type_info.signals:
        s += self.generate_client_signal_decl (sg, type_info)
      s += '  ' + self.F ('static %s' % classC, 9) + '__aida_cast__ (Rapicorn::Aida::RemoteHandle&, const Rapicorn::Aida::TypeHashList&);\n'
      s += '  ' + self.F ('static const Rapicorn::Aida::TypeHash&') + '__aida_typeid__();\n'
    # constructors
    s += 'protected:\n'
    if self.gen_mode == G4SERVANT:
      s += '  explicit ' + self.F ('') + '%s ();\n' % self.C (type_info) # ctor
      s += '  virtual ' + self.F ('/*Des*/') + '~%s () override = 0;\n' % self.C (type_info) # dtor
    s += 'public:\n'
    if self.gen_mode == G4STUB:
      s += '  virtual ' + self.F ('/*Des*/') + '~%s () override;\n' % self.C (type_info) # dtor
    c  = '  ' + self.F ('static Rapicorn::Aida::BaseConnection*') + '__aida_connection__();\n'
    if ddc:
      s += c
    if self.gen_mode == G4SERVANT:
      s += '  virtual ' + self.F ('Rapicorn::Aida::TypeHashList') + ' __aida_typelist__  () const override;\n'
      s += '  virtual ' + self.F ('std::string') + ' __aida_type_name__ () const override\t{ return "%s"; }\n' % classFull
      if self.property_list:
        s += '  virtual ' + self.F ('const ' + self.property_list + '&') + '__aida_properties__ ();\n'
    else: # G4STUB
      s += '  ' + self.F ('Rapicorn::Aida::TypeHashList          ') + '__aida_typelist__  () const;\n'
      s += '  template<class RemoteHandle>\n'
      s += '  ' + self.F ('static %s' % classH) + 'down_cast (RemoteHandle smh) '
      s += '{ return smh == NULL ? %s() : __aida_cast__ (smh, smh.__aida_typelist__()); }\n' % classH
      s += '  ' + self.F ('explicit') + '%s ();\n' % classH # ctor
      #s += '  ' + self.F ('inline') + '%s (const %s &src)' % (classH, classH) # copy ctor
      #s += ' : ' + ' (src), '.join (cl) + ' (src) {}\n'
    # properties
    il = 0
    if type_info.fields:
      il = max (len (fl[0]) for fl in type_info.fields)
    for fl in type_info.fields:
      s += self.generate_property_prototype (type_info, fl[0], fl[1], il)
    # signals
    if self.gen_mode == G4SERVANT:
      for sg in type_info.signals:
        s += '  ' + self.generate_server_signal_typedef (sg, type_info)
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_typename (sg, type_info) + ' sig_%s;\n' % sg.name
    else: # G4STUB
      for sg in type_info.signals:
        s += self.generate_client_signal_api (sg, type_info)
    # methods
    il = 0
    if type_info.methods:
      il = max (len (m.name) for m in type_info.methods)
      il = max (il, len (self.C (type_info)))
    for m in type_info.methods:
      s += self.generate_method_decl (type_info, m, il)
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if self.gen_mode == G4SERVANT:
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, %s*);\n' % self.C (type_info)
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, %sP&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s*&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %sP&);\n' % self.C (type_info)
    else: # G4STUB
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s&);\n' % self.C (type_info)
    # conversion operators
    if self.gen_mode == G4SERVANT:       # ->* _servant and ->* _handle operators
      s += '%s* operator->* (%s &sh, Rapicorn::Aida::_ServantType);\n' % (classC, classH)
      s += '%s operator->* (%s *obj, Rapicorn::Aida::_HandleType);\n' % (classH, classC)
    # typedef alias
    if self.gen_mode == G4STUB:
      s += self.generate_shortalias (type_info)
    return s
  def generate_shortdoc (self, type_info):      # doxygen snippets
    classC = self.C (type_info) # class name
    xkind = 'servant' if self.gen_mode == G4SERVANT else 'stub'
    s  = '/** @interface %s\n' % type_info.name
    s += ' * See also the corresponding C++ %s class %s. */\n' % (xkind, classC)
    s += '/// See also the corresponding IDL class %s.\n' % type_info.name
    return s
  def generate_shortalias (self, type_info):
    assert self.gen_mode == G4STUB
    s = ''
    alias = self.type2cpp (type_info) + 'H'
    s += 'typedef %s %s;' % (self.C (type_info), alias)
    s += ' ///< Convenience alias for the IDL type %s.\n' % type_info.name
    return s
  def generate_method_decl (self, class_info, functype, pad):
    s = '  '
    copydoc = 'See ' + self.type2cpp (class_info) + '::' + functype.name + '()'
    if self.gen_mode == G4SERVANT:
      s += 'virtual '
    s += self.F (self.R (functype.rtype))
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1], a[2]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if self.gen_mode == G4SERVANT and functype.pure:
      s += ' = 0'
    s += '; \t///< %s\n' % copydoc
    return s
  def generate_aida_connection_impl (self, class_info):
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    s, classC = '', self.C (class_info)
    if not ddc:
      return s
    s += 'Rapicorn::Aida::BaseConnection*\n'
    s += '%s::__aida_connection__()\n{\n' % classC
    if self.gen_mode == G4SERVANT:
      s += '  return ::Rapicorn::Aida::ObjectBroker::get_server_connection (__AIDA_Local__::server_connection);\n'
    else:
      s += '  return ::Rapicorn::Aida::ObjectBroker::get_client_connection (__AIDA_Local__::client_connection);\n'
    s += '}\n'
    return s
  def generate_client_class_methods (self, class_info):
    s, classH = '', self.C4client (class_info)
    classH2 = (classH, classH)
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    s += '%s::%s ()' % classH2 # ctor
    s += '\n{}\n'
    s += '%s::~%s ()\n{} // define empty dtor to emit vtable\n' % classH2 # dtor
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, const %s &handle)\n{\n' % classH
    s += '  __AIDA_Local__::client_connection->add_handle (fb, handle);\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &fbr, %s &handle)\n{\n' % classH
    s += '  __AIDA_Local__::client_connection->pop_handle (fbr, handle);\n'
    s += '}\n'
    s += 'const Rapicorn::Aida::TypeHash&\n'
    s += '%s::__aida_typeid__()\n{\n' % classH
    s += '  static const Rapicorn::Aida::TypeHash type_hash = Rapicorn::Aida::TypeHash (%s);\n' % self.class_digest (class_info)
    s += '  return type_hash;\n'
    s += '}\n'
    s += '%s\n%s::__aida_cast__ (Rapicorn::Aida::RemoteHandle &other, const Rapicorn::Aida::TypeHashList &types)\n{\n' % classH2 # similar to ctor
    s += '  const Rapicorn::Aida::TypeHash &mine = __aida_typeid__();\n'
    s += '  %s target;\n' % classH
    s += '  for (size_t i = 0; i < types.size(); i++)\n'
    s += '    if (mine == types[i]) {\n'
    s += '      target.__aida_upgrade_from__ (other);\n'
    s += '      break;\n'
    s += '    }\n'
    s += '  return target;\n'
    s += '}\n'
    s += self.generate_aida_connection_impl (class_info)
    s += 'Rapicorn::Aida::TypeHashList\n'
    s += '%s::__aida_typelist__() const\n{\n' % classH
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1);\n' # header + self
    s += '  __AIDA_Local__::add_header2_call (fb, *this, %s);\n' % self.list_types_digest (class_info)
    s += self.generate_proto_add_args ('fb', class_info, '', [('*this', class_info)], '')
    s += '  Rapicorn::Aida::FieldBuffer *fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    s += '  AIDA_CHECK (fr != NULL, "missing result from 2-way call");\n'
    s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
    s += '  frr.skip_header();\n'
    s += '  size_t len;\n'
    s += '  frr >>= len;\n'
    s += '  AIDA_CHECK (frr.remaining() == len * 2, "result truncated");\n'
    s += '  Rapicorn::Aida::TypeHashList thl;\n'
    s += '  Rapicorn::Aida::TypeHash thash;\n'
    s += '  for (size_t i = 0; i < len; i++) {\n'
    s += '    frr >>= thash;\n'
    s += '    thl.push_back (thash);\n'
    s += '  }\n'
    s += '  delete fr;\n'
    s += '  return thl;\n'
    s += '}\n'
    return s
  def generate_server_class_methods (self, class_info):
    assert self.gen_mode == G4SERVANT
    s, classC, classH = '\n', self.C (class_info), self.C4client (class_info)
    s += '%s::%s ()' % (classC, classC) # ctor
    s += '\n{}\n'
    s += '%s::~%s ()\n{} // define empty dtor to emit vtable\n' % (classC, classC) # dtor
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, %sP &ptr)\n{\n' % classC
    s += '  fb <<= ptr.get();\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, %s *obj)\n{\n' % classC
    s += '  __AIDA_Local__::field_buffer_add_interface (fb, obj);\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &fbr, %sP &obj)\n{\n' % classC
    s += '  obj = __AIDA_Local__::field_reader_pop_interface<%s> (fbr);\n' % classC
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &fbr, %s* &obj)\n{\n' % classC
    s += '  obj = __AIDA_Local__::field_reader_pop_interface<%s> (fbr).get();\n' % classC
    s += '}\n'
    s += '%s*\noperator->* (%s &sh, Rapicorn::Aida::_ServantType)\n{\n' % (classC, classH)
    s += '  return __AIDA_Local__::remote_handle_to_interface<%s> (sh);\n' % classC
    s += '}\n'
    s += '%s\noperator->* (%s *obj, Rapicorn::Aida::_HandleType)\n{\n' % (classH, classC)
    s += '  return __AIDA_Local__::interface_to_remote_handle<%s> (obj);\n' % classH
    s += '}\n'
    s += self.generate_aida_connection_impl (class_info)
    s += 'Rapicorn::Aida::TypeHashList\n'
    s += '%s::__aida_typelist__ () const\n{\n' % classC
    s += '  Rapicorn::Aida::TypeHashList thl;\n'
    ancestors = self.class_ancestry (class_info)
    for an in ancestors:
      s += '  thl.push_back (Rapicorn::Aida::TypeHash (%s)); // %s\n' % (self.class_digest (an), an.name)
    s += '  return thl;\n'
    s += '}\n'
    return s
  def generate_server_list_properties (self, class_info):
    def fill_range (ptype, hints):
      range_config = { Decls.INT32   : ('INT32_MIN', 'INT32_MAX', '1'),
                       Decls.INT64   : ('INT64_MIN', 'INT64_MAX', '1'),
                       Decls.FLOAT64 : ('DBL_MIN',   'DBL_MAX',   '0'),
                     }
      rconf = range_config.get (ptype.storage, None)
      if rconf:
        rmin, rmax, rstp = rconf
        return '%s, %s, %s, %s' % (rmin, rmax, rstp, hints)
      return hints
    if not self.property_list:
      return ''
    assert self.gen_mode == G4SERVANT
    s, classC, constPList = '', self.C (class_info), 'const ' + self.property_list
    s += constPList + '&\n' + classC + '::__aida_properties__ ()\n{\n'
    s += '  static ' + self.property_list + '::Property *properties[] = {\n'
    for fl in class_info.fields:
      cmmt = '// ' if fl[1].storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE, Decls.ANY) else ''
      default_flags = '""' if fl[1].auxdata.has_key ('label') else '"rw"'
      label, blurb = fl[1].auxdata.get ('label', '"' + fl[0] + '"'), fl[1].auxdata.get ('blurb', '""')
      hints = fl[1].auxdata.get ('hints', default_flags)
      s += '    ' + cmmt + 'RAPICORN_AIDA_PROPERTY (%s, %s, %s, %s, %s),\n' % (classC, fl[0], label, blurb, fill_range (fl[1], hints))
      if cmmt:
        self.warning ('%s::%s: property type not supported: %s' %
                      (self.namespaced_identifier (classC), fl[0], self.type2cpp (fl[1])), *fl[1].location)
    s += '  };\n'
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    calls = [cl + '::__aida_properties__()' for cl in precls]
    s += '  static ' + constPList + ' property_list (properties, %s);\n' % (', ').join (calls)
    s += '  return property_list;\n'
    s += '}\n'
    return s
  def generate_client_method_stub (self, class_info, mtype):
    s = ''
    hasret = mtype.rtype.storage != Decls.VOID
    copydoc = 'See ' + self.type2cpp (class_info) + '::' + mtype.name + '()'
    # prototype
    s += self.C (mtype.rtype) + '\n'
    q = '%s::%s (' % (self.C (class_info), mtype.name)
    s += q + self.Args (mtype, 'arg_', len (q)) + ') /// %s\n{\n' % copydoc
    # vars, procedure
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + %u), *fr = NULL;\n' % len (mtype.args) # header + self + args
    if hasret:  s += '  __AIDA_Local__::add_header2_call (fb, *this, %s);\n' % self.method_digest (mtype)
    else:       s += '  __AIDA_Local__::add_header1_call (fb, *this, %s);\n' % self.method_digest (mtype)
    # marshal args
    s += self.generate_proto_add_args ('fb', class_info, '', [('*this', class_info)], '')
    ident_type_args = [('arg_' + a[0], a[1]) for a in mtype.args]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    # call out
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    # unmarshal return
    if hasret:
      rarg = ('retval', mtype.rtype)
      s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
      s += '  frr.skip_header();\n'
      s += '  ' + self.V (rarg[0], rarg[1]) + ';\n'
      s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '')
      s += '  delete fr;\n'
      s += '  return retval;\n'
    else:
      s += '  if (AIDA_UNLIKELY (fr != NULL)) delete fr;\n' # FIXME: check return error
    s += '}\n'
    return s
  def generate_server_method_stub (self, class_info, mtype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_call__%s__%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT (fbr.remaining() == 3 + 1 + %u);\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "invalid \'this\' pointer");\n'
    # fetch args
    for a in mtype.args:
      s += '  ' + self.V ('arg_' + a[0], a[1]) + ';\n'
      s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])])
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.R (mtype.rtype) + ' rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.U ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_call_result (fbr, %s);\n' % self.method_digest (mtype) # invalidates fbr
      rval = 'rval'
      s += self.generate_proto_add_args ('rb', class_info, '', [(rval, mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def generate_property_prototype (self, class_info, fident, ftype, pad = 0):
    s, v, v0, rptr, ptr = '', '', '', '', ''
    copydoc = 'See ' + self.type2cpp (class_info) + '::' + fident
    if self.gen_mode == G4SERVANT:
      v, v0, rptr, ptr = 'virtual ', ' = 0', 'P', '*'
    tname = self.C (ftype)
    pid = fident + ' ' * max (0, pad - len (fident))
    if ftype.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s; \t///< %s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ')%s; \t///< %s\n' % (v0, copydoc)
    elif ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s; \t///< %s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (const ' + tname + '&)%s; \t///< %s\n' % (v0, copydoc)
    elif ftype.storage == Decls.INTERFACE:
      s += '  ' + v + self.F (tname + rptr)  + pid + ' () const%s; \t///< %s\n' % (v0, copydoc)
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ptr + ')%s; \t///< %s\n' % (v0, copydoc)
    return s
  def generate_client_property_stub (self, class_info, fident, ftype):
    s = ''
    tname, copydoc = self.C (ftype), 'See ' + self.type2cpp (class_info) + '::' + fident
    # getter prototype
    s += tname + '\n'
    q = '%s::%s (' % (self.C (class_info), fident)
    s += q + ') const /// %s\n{\n' % copydoc
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1), *fr = NULL;\n'
    s += '  __AIDA_Local__::add_header2_call (fb, *this, %s);\n' % self.getter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('*this', class_info)], '')
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    if 1: # hasret
      rarg = ('retval', ftype)
      s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
      s += '  frr.skip_header();\n'
      s += '  ' + self.V (rarg[0], rarg[1]) + ';\n'
      s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '')
      s += '  delete fr;\n'
      s += '  return retval;\n'
    s += '}\n'
    # setter prototype
    s += 'void\n'
    if ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += q + 'const ' + tname + ' &value) /// %s\n{\n' % copydoc
    else:
      s += q + tname + ' value) /// %s\n{\n' % copydoc
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + 1), *fr = NULL;\n' # header + self + value
    s += '  __AIDA_Local__::add_header1_call (fb, *this, %s);\n' % self.setter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('*this', class_info)], '')
    ident_type_args = [('value', ftype)]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    s += '  if (fr) delete fr;\n'
    s += '}\n'
    return s
  def generate_server_property_setter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_set__%s__%s' % (class_info.name, fident)
    setter_hash = self.setter_digest (class_info, fident, ftype)
    reglines += [ (setter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT (fbr.remaining() == 3 + 1 + 1);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "invalid \'this\' pointer");\n'
    # fetch property
    s += '  ' + self.V ('arg_' + fident, ftype) + ';\n'
    s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(fident, ftype)])
    ref = '&' if ftype.storage == Decls.INTERFACE else ''
    # call out
    s += '  self->' + fident + ' (' + ref + self.U ('arg_' + fident, ftype) + ');\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def generate_server_property_getter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_get__%s__%s' % (class_info.name, fident)
    getter_hash = self.getter_digest (class_info, fident, ftype)
    reglines += [ (getter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT (fbr.remaining() == 3 + 1);\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "invalid \'this\' pointer");\n'
    # return var
    s += '  '
    s += self.R (ftype) + ' rval = '
    # call out
    s += 'self->' + fident + ' ();\n'
    # store return value
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_call_result (fbr, %s);\n' % getter_hash # invalidates fbr
    rval = 'rval'
    s += self.generate_proto_add_args ('rb', class_info, '', [(rval, ftype)], '')
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_list_types (self, class_info, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_call__%s____aida_typelist__' % class_info.name
    digest = self.list_types_digest (class_info)
    reglines += [ (digest, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT (fbr.remaining() == 3 + 1);\n'
    s += '  Rapicorn::Aida::TypeHashList thl;\n'
    s += '  %s *self;\n' % self.C (class_info)  # fetch self
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    # support self==NULL here, to allow invalid cast handling at the client
    s += '  if (self) // guard against invalid casts\n'
    s += '    thl = self->__aida_typelist__();\n'
    # return: length (typehi, typelo)*length
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_call_result (fbr, %s, 1 + 2 * thl.size());\n' % digest # invalidates fbr
    s += '  rb <<= int64_t (thl.size());\n'
    s += '  for (size_t i = 0; i < thl.size(); i++)\n'
    s += '    rb <<= thl[i];\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_signal_typename (self, functype, ctype, prefix = 'Signal'):
    return '%s_%s' % (prefix, functype.name)
  def generate_signal_signature_tuple (self, functype, funcname = ''):
    s, r = '', self.R (functype.rtype)
    if funcname:
      s += '%s ' % funcname
    s += '('
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')'
    return (r, s)
  def generate_client_signal_decl (self, functype, ctype):
    assert self.gen_mode == G4STUB
    s, classH = '', self.C4client (ctype)
    sigret, sigargs = self.generate_signal_signature_tuple (functype)
    connector_name = '__Aida_Signal__%s' % functype.name
    s += '  ' + 'typedef Rapicorn::Aida::Connector<%s, %s %s> ' % (classH, sigret, sigargs) + connector_name + ';\n'
    s += '  size_t ' + '__aida_connect__' + functype.name + ' (size_t, const std::function<%s %s>&);\n' % (sigret, sigargs)
    return s
  def generate_client_signal_api (self, functype, ctype):
    assert self.gen_mode == G4STUB
    s, classH = '', self.C4client (ctype)
    sigret, sigargs = self.generate_signal_signature_tuple (functype)
    connector_name = '__Aida_Signal__%s' % functype.name
    s += '  ' + self.F (connector_name) + 'sig_' + functype.name + ' () '
    s += '{ return %s (*this, &%s::__aida_connect__%s); }\n' % (connector_name, classH, functype.name)
    return s
  def generate_client_signal_def (self, class_info, stype):
    assert self.gen_mode == G4STUB
    s, classH = '', self.C4client (class_info)
    digest, async = self.method_digest (stype), stype.rtype.storage != Decls.VOID
    (sigret, sigargs) = self.generate_signal_signature_tuple (stype)
    emitfunc = '__aida_emit%d__%s__%s' % ((2 if async else 1), classH, stype.name)
    s += 'static Rapicorn::Aida::FieldBuffer*\n%s ' % emitfunc
    s += '(const Rapicorn::Aida::FieldBuffer *sfb, void *data)\n{\n'
    s += '  auto fptr = (const std::function<%s %s>*) data;\n' % (sigret, sigargs)
    s += '  if (AIDA_UNLIKELY (!sfb)) { delete fptr; return NULL; }\n'
    s += '  Rapicorn::Aida::uint64 emit_result_id;\n'
    if async:
      s += '  ' + self.R (stype.rtype) + ' rval = Rapicorn::Aida::field_buffer_emit_signal (*sfb, *fptr, emit_result_id);\n'
      s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_emit_result (sfb, %s, 2);\n' % digest # invalidates fbr
      s += '  rb <<= emit_result_id;\n'
      s += self.generate_proto_add_args ('rb', class_info, '', [('rval', stype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  Rapicorn::Aida::field_buffer_emit_signal (*sfb, *fptr, emit_result_id);\n'
      s += '  return NULL;\n'
    s += '}\n'
    s += 'size_t\n%s::__aida_connect__%s (size_t signal_handler_id, const std::function<%s %s> &func)\n{\n' % (classH, stype.name, sigret, sigargs)
    s += '  if (signal_handler_id)\n'
    s += '    return __AIDA_Local__::signal_disconnect (signal_handler_id);\n'
    s += '  void *fptr = new std::function<%s %s> (func);\n' % (sigret, sigargs)
    s += '  return __AIDA_Local__::signal_connect (%s, *this, %s, fptr);\n}\n' % (self.method_digest (stype), emitfunc)
    return s
  def generate_server_signal_typedef (self, functype, ctype, prefix = ''):
    s, signame = '', self.generate_signal_typename (functype, ctype)
    cpp_rtype, async = self.R (functype.rtype), functype.rtype.storage != Decls.VOID
    s += 'typedef Rapicorn::Aida::%s<%s (' % (('AsyncSignal' if async else 'Signal'), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')> ' + prefix + signame + ';\n'
    return s
  def generate_server_signal_dispatcher (self, class_info, stype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__aida_connect__%s__%s' % (class_info.name, stype.name)
    digest, async = self.method_digest (stype), stype.rtype.storage != Decls.VOID
    reglines += [ (digest, self.namespaced_identifier (dispatcher_name)) ]
    closure_class = '__AIDA_Closure__%s__%s' % (class_info.name, stype.name)
    s += 'class %s {\n' % closure_class
    s += '  size_t handler_id_;\n'
    s += 'public:\n'
    s += '  typedef std::shared_ptr<%s> SharedPtr;\n' % closure_class
    s += '  %s (size_t h) : handler_id_ (h) {}\n' % closure_class # ctor
    s += '  ~%s()\n' % closure_class # dtor
    s += '  {\n'
    s += '    Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1);\n' # header + handler
    s += '    __AIDA_Local__::add_header1_discon (fb, handler_id_, %s);\n' % digest
    s += '    fb <<= handler_id_;\n'
    s += '    __AIDA_Local__::post_msg (&fb);\n' # deletes fb
    s += '  }\n'
    cpp_rtype = self.R (stype.rtype)
    s += '  static %s\n' % ('std::future<%s>' % cpp_rtype if async else cpp_rtype)
    s += '  handler (const SharedPtr &sp'
    if stype.args:
      s += ',\n' + ' ' * 11
      s += self.Args (stype, 'arg_', 11)
    s += ')\n  {\n'
    s += '    Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + %u + %d);\n' \
        % (len (stype.args), 1 if async else 0) # header + handler + args
    if not async:
      s += '    __AIDA_Local__::add_header1_emit (fb, sp->handler_id_, %s);\n' % digest
      s += '    fb <<= sp->handler_id_;\n'
    else:
      s += '    __AIDA_Local__::add_header2_emit (fb, sp->handler_id_, %s);\n' % digest
      s += '    fb <<= sp->handler_id_;\n'
      s += '    auto promise = std::make_shared<std::promise<%s>> ();\n' % cpp_rtype
      s += '    auto future = promise->get_future();\n'
      s += '    const size_t lambda_id = 1 + size_t (promise.get());\n' # generate unique (non-pointer) id
      s += '    auto lambda = [promise] (Rapicorn::Aida::FieldReader &frr) {\n'
      s += '      ' + self.R (stype.rtype) + ' retval;\n'
      s += '    ' + self.generate_proto_pop_args ('frr', class_info, '', [('retval', stype.rtype)], '')
      s += '      promise->set_value (retval);\n'
      s += '    };\n'
      s += '    __AIDA_Local__::erhandler_add (lambda_id, lambda);\n'
      s += '    fb <<= lambda_id;\n'
    ident_type_args = [(('&arg_' if a[1].storage == Decls.INTERFACE else 'arg_')+ a[0], a[1]) for a in stype.args] # marshaller args
    args2fb = self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    if args2fb:
      s += reindent ('  ', args2fb) + '\n'
    s += '    __AIDA_Local__::post_msg (&fb);\n' # deletes fb
    if async:
      s += '    return future;\n'
    s += '  }\n'
    s += '};\n'
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  AIDA_ASSERT (fbr.remaining() == 3 + 1 + 2);\n'
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "invalid \'this\' pointer");\n'
    s += '  size_t handler_id;\n'
    s += '  Rapicorn::Aida::uint64 signal_connection, result = 0;\n'
    s += '  fbr >>= handler_id;\n'
    s += '  fbr >>= signal_connection;\n'
    s += '  if (signal_connection)\n'
    s += '    result = self->sig_%s() -= signal_connection;\n' % stype.name
    s += '  if (handler_id) {\n'
    s += '    %s::SharedPtr sp (new %s (handler_id));\n' % (closure_class, closure_class)
    if async:
      s += '    result = self->sig_%s().connect_future (__AIDA_Local__::slot (sp, sp->handler));\n' % stype.name
    else:
      s += '    result = self->sig_%s() += __AIDA_Local__::slot (sp, sp->handler);\n' % stype.name
    s += '  }\n'
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_connect_result (fbr, %s);\n' % digest # invalidates fbr
    s += '  rb <<= result;\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_method_registry (self, reglines):
    s = ''
    if len (reglines) == 0:
      return '// Skipping empty MethodRegistry\n'
    s += 'static const __AIDA_Local__::MethodEntry _aida_stub_entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { ' + cdigest + ', '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static __AIDA_Local__::MethodRegistry _aida_stub_registry (_aida_stub_entries);\n'
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    assert self.gen_mode == G4SERVANT
    s = ''
    if functype.pure:
      return s
    absname = self.C (type_info) + '::' + functype.name
    if absname in self.skip_symbols:
      return ''
    sret = self.R (functype.rtype)
    sret += '\n'
    absname += ' ('
    argindent = len (absname)
    s += '\n' + sret + absname
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    if functype.rtype.storage == Decls.VOID:
      pass
    else:
      s += '  return %s;\n' % self.mkzero (functype.rtype)
    s += '}\n'
    return s
  def generate_interface_skel (self, type_info):
    s = ''
    for m in type_info.methods:
      s += self.generate_virtual_method_skel (m, type_info)
    return s
  def c_long_postfix (self, number):
    num, minus = (-number, '-') if number < 0 else (number, '')
    if num <= 2147483647:
      return minus + str (num)
    if num <= 9223372036854775807:
      return minus + str (num) + 'LL'
    if num <= 18446744073709551615:
      return minus + str (num) + 'uLL'
    return number # not a ULL?
  def generate_enum_decl (self, type_info):
    s = '\n'
    nm = type_info.name
    l = []
    s += '/// @cond GeneratedEnums\n'
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, self.c_long_postfix (number))
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};\n'
    s += 'inline void operator<<= (Rapicorn::Aida::FieldBuffer &fb,  %s  e) ' % nm
    s += '{ fb <<= Rapicorn::Aida::EnumValue (e); }\n'
    s += 'inline void operator>>= (Rapicorn::Aida::FieldReader &frr, %s &e) ' % nm
    s += '{ e = %s (frr.pop_evalue()); }\n' % nm
    if type_info.combinable: # enum as flags
      s += 'inline %s  operator&  (%s  s1, %s s2) { return %s (s1 & Rapicorn::Aida::uint64 (s2)); }\n' % (nm, nm, nm, nm)
      s += 'inline %s& operator&= (%s &s1, %s s2) { s1 = s1 & s2; return s1; }\n' % (nm, nm, nm)
      s += 'inline %s  operator|  (%s  s1, %s s2) { return %s (s1 | Rapicorn::Aida::uint64 (s2)); }\n' % (nm, nm, nm, nm)
      s += 'inline %s& operator|= (%s &s1, %s s2) { s1 = s1 | s2; return s1; }\n' % (nm, nm, nm)
    s += '/// @endcond\n'
    return s
  def generate_enum_info_specialization (self, type_info):
    s = '\n'
    classFull = '::'.join (self.type_relative_namespaces (type_info) + [ type_info.name ])
    s += 'template<> const EnumValue* enum_value_list<%s> ();\n' % classFull
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
      m = (re.match ('(includes):\s*(//.*)?$', line) or
           re.match ('(class_scope:\w+):\s*(//.*)?$', line) or
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
    w = re.findall (r'(\b[a-zA-Z_][a-zA-Z_0-9$:]*)(?:\()', txt)
    self.skip_symbols.update (set (w))
  def idl_path (self):
    apath = os.path.abspath (self.idl_file)
    if self.strip_path:
      for prefix in (self.strip_path, os.path.abspath (self.strip_path)):
        if apath.startswith (prefix):
          apath = apath[len (prefix):]
          if apath[0] == '/':
            apath = apath[1:]
          break
    return apath
  def generate_impl_types (self, implementation_types):
    def text_expand (txt):
      txt = txt.replace ('$AIDA_iface_base$', self.iface_base)
      return txt
    self.gen_mode = G4SERVANT if self.gen_serverhh or self.gen_servercc else G4STUB
    s = '// --- Generated by AidaCxxStub ---\n'
    # CPP guard
    sc_macro_prefix = '__SRVT__' if self.gen_mode == G4SERVANT else '__CLNT__'
    if self.gen_serverhh or self.gen_clienthh:
      s += '#ifndef %s\n#define %s\n\n' % (sc_macro_prefix + self.cppmacro, sc_macro_prefix + self.cppmacro)
    # inclusions
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#ifndef __AIDA_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_clienthh:
      s += clienthh_boilerplate
    if self.gen_serverhh:
      s += serverhh_boilerplate
      s += rapicornsignal_boilerplate
    if self.gen_servercc:
      s += text_expand (TmplFiles.CxxStub_server_cc) + '\n'
    if self.gen_clientcc:
      s += text_expand (TmplFiles.CxxStub_client_cc) + '\n'
    self.tab_stop (30)
    s += self.open_namespace (None)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      self.gen_mode = G4SERVANT if self.gen_serverhh else G4STUB
      s += '\n// --- Interfaces (class declarations) ---\n'
      spc_enums, class_name_list = [], []
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          s += 'class %s;\n' % self.C (tp)
        elif tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          s += self.open_namespace (tp)
          s += self.generate_recseq_decl (tp)
        elif tp.storage == Decls.ENUM and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp)
          spc_enums += [ tp ]
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += self.generate_interface_class (tp, class_name_list)     # Class remote handle
      if spc_enums:
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_info_specialization (tp)
      s += self.open_namespace (None)
      if self.gen_serverhh and class_name_list:
        s += '\n#define %s_INTERFACE_LIST' % self.cppmacro
        for i in class_name_list:
          s += ' \\\n\t  %s_INTERFACE_NAME (%s)' % (self.cppmacro, i)
        s += '\n'
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      self.gen_mode = G4SERVANT if self.gen_servercc else G4STUB
      s += '\n// --- Implementations ---\n'
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          continue
        if tp.storage == Decls.RECORD and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp)
        elif tp.storage == Decls.SEQUENCE and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp)
        elif tp.storage == Decls.ENUM and self.gen_mode == G4STUB:
          spc_enums += [ tp ]
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            s += self.open_namespace (tp)
            s += self.generate_server_class_methods (tp)
            s += self.generate_server_list_properties (tp)
          if self.gen_clientcc:
            s += self.open_namespace (tp)
            for sg in tp.signals:
              s += self.generate_client_signal_def (tp, sg)
            s += self.generate_client_class_methods (tp)
            for fl in tp.fields:
              s += self.generate_client_property_stub (tp, fl[0], fl[1])
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
      if spc_enums:
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_impl (tp)
    # generate unmarshalling server calls
    if self.gen_servercc:
      self.gen_mode = G4SERVANT
      s += '\n// --- Method Dispatchers & Registry ---\n'
      reglines = []
      for tp in types:
        if tp.is_forward:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE:
          s += self.generate_server_list_types (tp, reglines)
          for fl in tp.fields:
            s += self.generate_server_property_getter (tp, fl[0], fl[1], reglines)
            s += self.generate_server_property_setter (tp, fl[0], fl[1], reglines)
          for m in tp.methods:
            s += self.generate_server_method_stub (tp, m, reglines)
          for sg in tp.signals:
            s += self.generate_server_signal_dispatcher (tp, sg, reglines)
          s += '\n'
      s += self.open_namespace (None)
      s += self.generate_server_method_registry (reglines) + '\n'
    # generate interface method skeletons
    if self.gen_server_skel:
      s += self.open_namespace (None)
      self.gen_mode = G4SERVANT
      s += '\n// --- Interface Skeletons ---\n'
      for tp in types:
        if tp.is_forward:
          continue
        elif tp.storage == Decls.INTERFACE:
          s += self.generate_interface_skel (tp)
    s += self.open_namespace (None) # close all namespaces
    s += '\n'
    s += self.insertion_text ('global_scope')
    # CPP guard
    if self.gen_serverhh or self.gen_clienthh:
      s += '#endif /* %s */\n' % (sc_macro_prefix + self.cppmacro)
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  global I_prefix_postfix
  config = {}
  config.update (args)
  idlfiles = config['files']
  if len (idlfiles) != 1:
    raise RuntimeError ("CxxStub: exactly one IDL input file is required")
  gg = Generator (idlfiles[0])
  all = config['backend-options'] == []
  gg.gen_serverhh = all or 'serverhh' in config['backend-options']
  gg.gen_servercc = all or 'servercc' in config['backend-options']
  gg.gen_server_skel = 'server-skel' in config['backend-options']
  gg.gen_clienthh = all or 'clienthh' in config['backend-options']
  gg.gen_clientcc = all or 'clientcc' in config['backend-options']
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt.startswith ('macro='):
      gg.cppmacro = opt[6:]
    if opt.startswith ('strip-path='):
      gg.strip_path += opt[11:]
    if opt.startswith ('iface-postfix='):
      I_prefix_postfix = (I_prefix_postfix[0], opt[14:])
    if opt.startswith ('iface-prefix='):
      I_prefix_postfix = (opt[13:], I_prefix_postfix[1])
    if opt.startswith ('iface-base='):
      gg.iface_base = opt[11:]
    if opt.startswith ('property-list=') and opt[14:].lower() in ('0', 'no', 'none', 'false'):
      gg.property_list = ""
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  for ssfile in config['skip-skels']:
    gg.symbol_file (ssfile)
  ns_rapicorn = Decls.Namespace ('Rapicorn', None, [])
  gg.ns_aida = ( ns_rapicorn, Decls.Namespace ('Aida', ns_rapicorn, []) ) # Rapicorn::Aida namespace tuple for open_namespace()
  textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
__Aida__.set_default_backend (__file__)
