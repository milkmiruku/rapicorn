#!/usr/bin/env python
# Aida - Abstract Interface Definition Architecture            -*-mode:python-*-
# Licensed GNU GPL v3 or later: http://www.gnu.org/licenses/gpl.html
import sys, os, Decls
true, false, length = (True, False, len)

import yapps2runtime as runtime
import AuxData

reservedwords = ('class', 'signal', 'void', 'self')
collectors = ('void', 'sum', 'last', 'until0', 'while0')
keywords = ('TRUE', 'True', 'true', 'FALSE', 'False', 'false',
            'namespace', 'enum', 'Const', 'typedef', 'interface',
            'record', 'sequence', 'bool', 'String', 'Any',
            # golang types
            'uint8', 'uint16', 'uint32', 'uint64', 'int8', 'int16', 'int32', 'int64',
            'float32', 'float64', 'complex64', 'complex128', 'byte', 'rune')
reservedkeywords = set (keywords + reservedwords)

class YYGlobals (object):
  def __init__ (self):
    self.reset()
  def reset (self):
    self.config = {}
    self.ecounter = None
    self.impl_list = [] # ordered impl types list
    self.impl_includes = false
    self.impl_rpaths = []
    self.parsed_files = []
    self.global_namespace = Decls.Namespace ('', None, self.impl_list)
    self.namespaces = [ self.global_namespace ] # currently open namespaces
  def configure (self, confdict, implfiles):
    self.config = {}
    self.config.update (confdict)
    self.impl_rpaths = list (implfiles)
    self.parsed_files = []
  def nsadd_const (self, name, value):
    if not isinstance (value, (int, long, float, str)):
      raise TypeError ('constant expression does not yield string or number: ' + repr (typename))
    self.namespaces[-1].add_const (name, value, yy.impl_includes)
  def nsadd_typedef (self, fielddecl):
    typename,srctype,auxinit = fielddecl
    AIn (typename)
    typeinfo = srctype.clone (typename, yy.impl_includes)
    typeinfo.typedef_origin = srctype
    self.parse_assign_auxdata ([ (typename, typeinfo, auxinit) ])
    self.namespaces[-1].add_type (typeinfo)
  def nsadd_evalue (self, evalue_ident, evalue_label, evalue_blurb, evalue_number = None):
    AS (evalue_ident)
    if evalue_number == None:
      evalue_number = yy.ecounter
      yy.ecounter += 1
    else:
      AN (evalue_number)
      yy.ecounter = 1 + evalue_number
    yy.nsadd_const (evalue_ident, evalue_number)
    return (evalue_ident, evalue_label, evalue_blurb, evalue_number)
  def nsadd_enum (self, enum_name, enum_values, as_flags):
    if len (enum_values) < 1:
      raise AttributeError ('invalid empty enumeration: %s' % enum_name)
    enum = Decls.TypeInfo (enum_name, Decls.ENUM, yy.impl_includes)
    if as_flags:
      enum.set_combinable (True)
    for ev in enum_values:
      enum.add_option (*ev)
    self.namespaces[-1].add_type (enum)
  def nsadd_record (self, name, rfields):
    AIn (name)
    if (self.config.get ('system-typedefs', 0) and self.namespaces[-1].name == 'Aida' and
        len (rfields) == 1 and rfields[0][0] == '__builtin__deleteme__'):
      rfields = []
    elif len (rfields) < 1:
      raise AttributeError ('invalid empty record: %s' % name)
    rec = Decls.TypeInfo (name, Decls.RECORD, yy.impl_includes)
    self.parse_assign_auxdata (rfields)
    fdict = {}
    for field in rfields:
      if fdict.has_key (field[0]):
        raise NameError ('duplicate field name: ' + field[0])
      fdict[field[0]] = true
      rec.add_field (field[0], field[1])
    self.namespaces[-1].add_type (rec)
  def nsadd_interface (self, name, addproto = False):
    iface = yy.namespace_lookup (name, astype = True)
    if not iface or (iface.storage == Decls.INTERFACE and
                     iface.is_forward and not addproto):
      iface = Decls.TypeInfo (name, Decls.INTERFACE, yy.impl_includes)
      if addproto:
        iface.is_forward = True
      self.namespaces[-1].add_type (iface)
    elif (iface.storage != Decls.INTERFACE or # exists as different identifier
          (not iface.is_forward and not addproto)): # interface redefinition
      AIn (name)
    return iface
  def interface_fill (self, iface, prerequisites, ifields, imethods, isignals):
    for prq in prerequisites:
      iface.add_prerequisite (yy.namespace_lookup (prq, astype = True))
    self.parse_assign_auxdata (ifields)
    mdict = {}
    for field in ifields:
      if field[0] in reservedkeywords:
        raise NameError ('invalid property name: ' + field[0])
      if mdict.has_key (field[0]):
        raise NameError ('duplicate member name: ' + field[0])
      mdict[field[0]] = true
      iface.add_field (field[0], field[1])
    sigset = set ([sig[0] for sig in isignals])
    for method in imethods + isignals:
      if mdict.has_key (method[0]):
        raise NameError ('duplicate member name: ' + method[0])
      mdict[method[0]] = true
      method_args = method[3]
      # self.parse_assign_auxdata (method_args)
      mtype = Decls.TypeInfo (method[0], Decls.FUNC, yy.impl_includes)
      mtype.set_rtype (method[1])
      mtype.set_pure (method[4])
      adict = {}
      need_default = false
      for arg in method_args:
        need_default = need_default or arg[2] != None
        if arg[0] in reservedkeywords:
          raise NameError ('invalid method arg name: ' + method[0] + ' (...' + arg[0] + '...)')
        if adict.has_key (arg[0]):
          raise NameError ('duplicate method arg name: ' + method[0] + ' (...' + arg[0] + '...)')
        if need_default and arg[2] == None:
          raise AttributeError ('missing subsequent default initializer: ' + method[0] + ' (...' + arg[0] + '...)')
        adict[arg[0]] = true
        mtype.add_arg (arg[0], arg[1], arg[2])
      iface.add_method (mtype, method[0] in sigset)
  def parse_assign_auxdata (self, fieldlist):
    for field in fieldlist:
      if not field[2]:
        continue
      name,typeinfo,(auxident,auxargs) = field
      try:
        adict = AuxData.parse2dict (typeinfo.storage, auxident, auxargs)
      except AuxData.Error, ex:
        raise TypeError (str (ex))
      typeinfo.update_auxdata (adict)
  def argcheck (self, aident, atype, adef):
    if adef == None:
      pass # no default arg
    elif atype.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64):
      if not isinstance (adef, (bool, int, long, float)):
        raise AttributeError ('expecting numeric initializer: %s = %s' % (aident, adef))
    elif atype.storage in (Decls.RECORD, Decls.SEQUENCE, Decls.FUNC, Decls.INTERFACE):
      if adef != 0:
        raise AttributeError ('expecting null initializer for structured argument: %s = %s' % (aident, adef))
    elif atype.storage == Decls.STRING:
      if not TS (adef):
        raise AttributeError ('expecting string initializer: %s = %s' % (aident, adef))
    elif atype.storage == Decls.ENUM:
      if not atype.has_option (number = adef):
        raise AttributeError ('encountered invalid enum initializer: %s = %s' % (aident, adef))
    else:
      raise AttributeError ('invalid default initializer: %s = %s' % (aident, adef))
    return (aident, atype, adef)
  def nsadd_sequence (self, name, sfields):
    AIn (name)
    if len (sfields) < 1:
      raise AttributeError ('invalid empty sequence: %s' % name)
    if len (sfields) > 1:
      raise AttributeError ('invalid multiple fields in sequence: %s' % name)
    self.parse_assign_auxdata (sfields)
    seq = Decls.TypeInfo (name, Decls.SEQUENCE, yy.impl_includes)
    seq.set_elements (sfields[0][0], sfields[0][1])
    self.namespaces[-1].add_type (seq)
  def namespace_match (self, nspace, ns_words, ident, flags):
    if ns_words:        # nested namespace lookup
      ns_child = nspace.ns_nested.get (ns_words[0], None)
      if ns_child:
        return self.namespace_match (ns_child, ns_words[1:], ident, flags)
    else:               # identifier lookup
      if flags.get ('asnamespace', 0):
        ns_child = nspace.ns_nested.get (ident, None)
        if ns_child:
          return ns_child
      if flags.get ('astype', 0):
        type_info = nspace.find_type (ident)
        if type_info:
          return type_info
      if flags.get ('asconst', 0):
        cvalue = nspace.find_const (ident)
        if cvalue:
          return cvalue
    return None
  def namespace_lookup (self, full_identifier, **flags):
    current_namespace = self.namespaces[-1]
    words = full_identifier.split ('::')
    isabs = words[0] == ''      # ::PrefixedName
    if isabs: words = words[1:]
    nswords, ident = words[:-1], words[-1] # namespace words, identifier
    prefix, identifier = '::'.join (words[:-1]), words[-1]
    # collect ancestor namespaces for lookups
    if not isabs:
      ns = current_namespace
      candidates = []
      while ns:
        candidates += [ ns ]
        if flags.get ('withusing', 1):
          candidates += ns.ns_using
        ns = ns.namespace
    else:
      candidates = [ self.global_namespace ]
      if flags.get ('withusing', 1):
        candidates += self.global_namespace.ns_using
    # try candidates in order
    for ns in candidates:
      result = self.namespace_match (ns, nswords, ident, flags)
      if result:
        return result
    return None
  def clone_type (self, typename, errorident, **flags):
    if not flags.get ('stream', 0):
      ANOSTREAM (typename, errorident)
    type_info = self.resolve_type (typename, flags.get ('void', 0))
    return type_info.clone (type_info.name, yy.impl_includes)
  def resolve_type (self, typename, void = False):
    type_info = self.namespace_lookup (typename, astype = True)
    if not type_info:   # builtin types
      type_info = Decls.TypeInfo.builtin_type (typename)
    if type_info and type_info.storage == Decls.VOID and not void:
      type_info = None
    if not type_info:
      raise TypeError ('unknown type: ' + repr (typename))
    return type_info
  def namespace_using (self, ident):
    ns = self.namespace_lookup (ident, asnamespace = True)
    if not ns:
      raise NameError ('not a namespace-name: ' + ident)
    current_namespace = self.namespaces[-1]
    if ns != current_namespace and not ns in current_namespace.ns_using:
      current_namespace.ns_using += [ ns ]
  def namespace_open (self, ident):
    if not self.config.get ('system-typedefs', 0) and ident.find ('$') >= 0:
      raise NameError ('invalid characters in namespace: ' + ident)
    current_namespace = self.namespaces[-1]
    namespace = current_namespace.ns_nested.get (ident, None)
    if not namespace:
      namespace = Decls.Namespace (ident, current_namespace, self.impl_list)
    self.namespaces += [ namespace ]
    return
  def namespace_close (self):
    assert len (self.namespaces)
    self.namespaces = self.namespaces[:-1]
  def handle_include (self, includefilename, origscanner, implinc):
    ddir = os.path.dirname (origscanner.filename) # directory for source relative includes
    filepath = os.path.join (ddir, includefilename)
    if not os.path.exists (filepath):
      for dd in self.config.get ('includedirs'):
        testpath = os.path.join (dd, includefilename)
        if os.path.exists (testpath):
          filepath = testpath
          break
    f = open (filepath)
    input = f.read()
    try:
      result = parse_try (filepath, input, implinc)
    except Error, ex:
      pos_file, pos_line, pos_col = origscanner.get_pos()
      if self.config.get ('anonymize-filepaths', 0):
        pos_file = re.sub (r'.*/([^/]+)$', r'.../\1', '/' + pos_file)
      ix = Error ('%s:%d: note: included "%s" from here' % (pos_file, pos_line, includefilename))
      ix.exception = ex.exception
      ex.exception = ix
      raise ex
    return result
yy = YYGlobals() # globals

def constant_lookup (variable):
  value = yy.namespace_lookup (variable, asconst = True)
  if not value:
    raise NameError ('undeclared constant: ' + variable)
  return value[0]
def quote (qstring):
  import rfc822
  return '"' + rfc822.quote (qstring) + '"'
def unquote (qstring):
  assert (qstring[0] == '"' and qstring[-1] == '"')
  import rfc822
  return rfc822.unquote (qstring)
def TN (number_candidate):  # test number
  return isinstance (number_candidate, (int, long)) or isinstance (number_candidate, float)
def TS (string_candidate):  # test string
  return isinstance (string_candidate, str) and len (string_candidate) >= 2
def TSp (string_candidate): # test plain string
  return TS (string_candidate) and string_candidate[0] == '"'
def TSi (string_candidate): # test i18n string
  return TS (string_candidate) and string_candidate[0] == '_'
def AN (number_candidate):  # assert number
  if not TN (number_candidate): raise TypeError ('invalid number: ' + repr (number_candidate))
def AS (string_candidate):  # assert string
  if not TS (string_candidate): raise TypeError ('invalid string: ' + repr (string_candidate))
def ASp (string_candidate, constname = None):   # assert plain string
  if not TSp (string_candidate):
    if constname:   raise TypeError ("invalid untranslated string (constant '%s'): %s" % (constname, repr (string_candidate)))
    else:           raise TypeError ('invalid untranslated string: ' + repr (string_candidate))
def ASi (string_candidate): # assert i18n string
  if not TSi (string_candidate): raise TypeError ('invalid translated string: ' + repr (string_candidate))
def AIn (identifier):   # assert new identifier
  if (yy.namespace_lookup (identifier, astype = True, asconst = True, asnamespace = True, withusing = False) or
      (identifier in reservedkeywords)):
    raise TypeError ('redefining existing identifier: %s' % identifier)
def AIi (identifier):   # assert interface identifier
  ti = yy.namespace_lookup (identifier, astype = True)
  if ti and ti.storage == Decls.INTERFACE:
    return True
  raise TypeError ('no such interface type: %s' % identifier)
def ATN (typename):     # assert a typename
  yy.resolve_type (typename) # raises exception
def ANOSIG (issignal, identifier): # assert non-signal decl
  if issignal:
    raise TypeError ('non-method invalidly declared as \'signal\': %s' % identifier)
def TSTREAM (typename):
  return typename in ('IStream', 'OStream', 'JStream')
def ANOSTREAM (typename, identifier): # assert non-stream decl
  if TSTREAM (typename):
    raise TypeError ('stream type used in wrong context: %s %s' % (typename, identifier))
def ANP (isfunc, identifier): # assert pure non-func decl
  if not isfunc:
    raise TypeError ('non-method invalidly declared as pure: %s' % identifier)
def ASC (collkind): # assert signal collector
  if not collkind in collectors:
    raise TypeError ('invalid signal collector: %s' % collkind)

class Error (Exception):
  def __init__ (self, msg, ecaret = None):
    Exception.__init__ (self, msg)
    self.ecaret = ecaret
    self.exception = None       # chain

def parse_try (filename, input_string, implinc):
  xscanner = IdlSyntaxParserScanner (input_string, filename = filename)
  xparser  = IdlSyntaxParser (xscanner)
  rpath = os.path.realpath (filename)
  if rpath in yy.parsed_files:
    return yy.impl_list # IdlSyntax result
  yy.parsed_files += [ rpath ]
  result, exmsg = (None, None)
  try:
    saved_impl_includes = yy.impl_includes
    yy.impl_includes = implinc or rpath in yy.impl_rpaths
    result = xparser.IdlSyntax () # returns yy.impl_list
    yy.impl_includes = saved_impl_includes
  except AssertionError: raise  # pass on language exceptions
  except Error: raise           # preprocessed parsing exception
  except runtime.SyntaxError, synex:
    exmsg = synex.msg
  except Exception, ex:
    exstr = str (ex).strip()
    if yy.config.get ('pass-exceptions', 0) or not exstr:
      raise                     # pass exceptions on when debugging
    exmsg = '%s: %s' % (ex.__class__.__name__, exstr)
  if exmsg:
    pos = xscanner.get_pos()
    file_name, line_number, column_number = pos
    if yy.config.get ('anonymize-filepaths', 0):        # FIXME: global yy reference
        file_name = re.sub (r'.*/([^/]+)$', r'.../\1', '/' + file_name)
    errstr = '%s:%d:%d: %s' % (file_name, line_number, column_number, exmsg)
    class WritableObject:
      def __init__ (self): self.content = []
      def write (self, string): self.content.append (string)
    wo = WritableObject()
    xscanner.print_line_with_pointer (pos, out = wo)
    ecaret = ''.join (wo.content).strip()
    raise Error (errstr, ecaret)
  return result

def parse_files (config, filepairs):
  implfiles = []
  for fp in filepairs:
    filename, fileinput = fp
    implfiles += [ os.path.realpath (filename) ]
  yy.configure (config, implfiles)
  try:
    result = ()
    for fp in filepairs:
      filename, fileinput = fp
      result = parse_try (filename, fileinput, True)
    return (result, None, None, [])
  except Error, ex:
    el = []
    cx = ex.exception
    while cx:
      el = [ str (cx) ] + el
      cx = cx.exception
    return (None, str (ex), ex.ecaret, el)

%%
parser IdlSyntaxParser:
        ignore:             r'\s+'                          # spaces
        ignore:             r'//.*?\r?(\n|$)'               # single line comments
        ignore:             r'/\*([^*]|\*[^/])*\*/'         # multi line comments
        token EOF:          r'$'
        token IDENT:        r'[a-zA-Z_][a-zA-Z_0-9]*'       # identifiers
        token NSIDENT:      r'[a-zA-Z_][a-zA-Z_0-9$]*'      # identifier + '$'
        token INTEGER:      r'[0-9]+'
        token HEXINT:       r'0[xX][0-9abcdefABCDEF]+'
        token FULLFLOAT:    r'([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-][0-9]+)?'
        token FRACTFLOAT:                     r'\.[0-9]+([eE][+-][0-9]+)?'
        token STRING:       r'"([^"\\]+|\\.)*"'             # double quotes string

rule IdlSyntax: ( ';'
                | namespace
                | topincludes
                )* EOF                          {{ return yy.impl_list; }}

rule namespace:
        'namespace' NSIDENT                     {{ yy.namespace_open (NSIDENT) }}
        '{' declaration* '}'                    {{ yy.namespace_close() }}
rule topincludes:
        'include' STRING                        {{ include_file = unquote (STRING); as_impl = false }}
        [ 'as' 'implementation'                 {{ as_impl = true }}
        ] ';'                                   {{ yy.handle_include (include_file, self._scanner, as_impl) }}
rule using_namespace:
        'using' 'namespace' NSIDENT ';'         {{ yy.namespace_using (NSIDENT) }}
rule declaration:
          ';'
        | const_assignment
        | enumeration
        | typedef
        | sequence
        | record
        | interface
        | namespace
        | using_namespace

rule enumeration:
        ( 'flags' ('enumeration' | 'enum')      {{ as_flags = True }}
        |         ('enumeration' | 'enum')      {{ as_flags = False }}
        )
        IDENT '{'                               {{ evalues = []; yy.ecounter = 1 }}
        enumeration_rest                        {{ evalues = enumeration_rest }}
        '}'                                     {{ AIn (IDENT); yy.nsadd_enum (IDENT, evalues, as_flags) }}
        ';'                                     {{ evalues = None; yy.ecounter = None }}
rule enumeration_rest:                          {{ evalues = [] }}
        ( ''                                    # empty
        | enumerator_decl                       {{ evalues = evalues + [ enumerator_decl ] }}
          [ ',' enumeration_rest                {{ evalues = evalues + enumeration_rest }}
          ]
        )                                       {{ return evalues }}
rule enumerator_decl:
        IDENT                                   {{ l = [IDENT, None, "", ""]; AIn (IDENT) }}
        [ '='
          ( enumerator_args                     {{ l = [ IDENT ] + enumerator_args }}
          | expression                          {{ if TS (expression): l = [ None, expression ]; }}
                                                {{ else:               l = [ expression, "" ] }}
                                                {{ l = [ IDENT ] + l + [ "" ] }}
          )
        ]                                       {{ return yy.nsadd_evalue (l[0], l[2], l[3], l[1]) }}
rule enumerator_args:
        'Enum' '\(' expression                  {{ l = [ expression ] }}
                                                {{ if TS (expression): l = [ None ] + l }}
        [   ',' expression                      {{ AS (expression); l.append (expression) }}
        ] [ ',' expression                      {{ if len (l) >= 3: raise OverflowError ("too many arguments") }}
                                                {{ AS (expression); l.append (expression) }}
        ] '\)'                                  {{ while len (l) < 3: l.append ("") }}
                                                {{ return l }}

rule typename:                                  {{ plist = [] }}
        [ '::'                                  {{ plist += [ '' ] }}
        ] IDENT                                 {{ plist += [ IDENT ] }}
        ( '::' IDENT                            {{ plist.append (IDENT) }}
          )*                                    {{ id = "::".join (plist); ATN (id); return id }}

rule auxinit:
        IDENT                                   {{ tiident = IDENT }}
        '\('                                    {{ tiargs = [] }}
          [ expression                          {{ tiargs += [ expression ] }}
            ( ',' expression                    {{ tiargs += [ expression ] }}
            )*
          ]
        '\)'                                    {{ return (tiident, tiargs) }}

rule field_decl:
        typename IDENT                          {{ ftype = yy.clone_type (typename, IDENT); ftuple = (IDENT, ftype, () ) }}
        [ '=' auxinit                           {{ ftuple = (ftuple[0], ftuple[1], auxinit) }}
        ] ';'                                   {{ return [ ftuple ] }}

rule method_args:
        typename IDENT                          {{ aident = IDENT; adef = None; atype = yy.clone_type (typename, IDENT) }}
        [ '=' expression                        {{ adef = expression }}
        ]                                       {{ a = yy.argcheck (aident, atype, adef); args = [ a ] }}
        ( ',' typename IDENT                    {{ aident = IDENT; adef = None; atype = yy.clone_type (typename, IDENT) }}
          [ '=' expression                      {{ adef = expression }}
          ]                                     {{ a = yy.argcheck (aident, atype, adef); args += [ a ] }}
        ) *                                     {{ return args }}

rule field_stream_method_signal_decl:
                                                {{ signal = false; pure = 0; fargs = []; daux = () }}
        [ 'signal'                              {{ signal = true; coll = 'void' }}
          ]
        ( 'void'                                {{ dtname = 'void' }}
        | typename                              {{ dtname = typename }}
        )
        IDENT                                   {{ dident = IDENT; kind = 'field' }}
        ( [ '=' auxinit                         {{ daux = auxinit }}
          ]
        | '\('                                  {{ kind = signal and 'signal' or 'func' }}
              [ method_args                     {{ fargs = method_args }}
              ] '\)'                            # [ '=' auxinit {{ daux = auxinit }} ]
        ) [ '=' '0'                             {{ pure = 1; ANP (kind == 'func', dident) }}
          ] ';'                                 {{ if kind == 'field': ANOSIG (signal, dident) }}
                                                {{ if kind == 'field' and TSTREAM (dtname): kind = 'stream' }}
                                                {{ flags = { 'void' : kind in ('func', 'signal'), 'stream' : kind == 'stream' } }}
                                                {{ dtype = yy.clone_type (dtname, dident, **flags) }}
                                                {{ if kind == 'signal': dtype.set_collector (coll) }}
                                                {{ if kind == 'field': return (kind, (dident, dtype, daux)) }}
                                                {{ return (kind, (dident, dtype, daux, fargs, pure)) }}

rule typedef:
        'typedef' field_decl                    {{ yy.nsadd_typedef (field_decl[0]) }}

rule field_group:
               'group'                          {{ gfields = [] }}
               ('_' '\(' STRING '\)'            {{ gident = STRING }}
               |         STRING                 {{ gident = STRING }}
               ) '{' ( field_decl               {{ gfields += field_decl }}
                 )+ '}' ';'                     {{ return gfields }}
rule interface:
        'interface'                             {{ ipls = []; ifls = []; prq = [] }}
        IDENT                                   {{ iident = IDENT; isigs = [] }}
        ( ';'                                   {{ iface = yy.nsadd_interface (iident, True) }}
        |
          [ ':' typename                        {{ prq += [ typename ]; AIi (typename) }}
              ( ',' typename                    {{ prq += [ typename ]; AIi (typename) }}
              ) * ]
          '{'                                   {{ iface = yy.nsadd_interface (iident) }}
             ( field_group                      {{ ipls = ipls + field_group }}
             | info_assignment                  {{ }}
             | field_stream_method_signal_decl  {{ fmd = field_stream_method_signal_decl }}
                                                {{ if fmd[0] == 'field': ipls = ipls + [ fmd[1] ] }}
                                                {{ if fmd[0] == 'func': ifls = ifls + [ fmd[1] ] }}
                                                {{ if fmd[0] == 'signal': isigs = isigs + [ fmd[1] ] }}
             )*
          '}' ';'                               {{ yy.interface_fill (iface, prq, ipls, ifls, isigs) }}
        )

rule record:
        'record' IDENT '{'                      {{ rfields = []; rident = IDENT }}
          ( field_decl                          {{ rfields = rfields + field_decl }}
          | field_group                         {{ rfields = rfields + field_group }}
          | info_assignment                     {{ }}
          )+
        '}' ';'                                 {{ yy.nsadd_record (rident, rfields) }}

rule sequence:
        'sequence' IDENT '{'                    {{ sfields = [] }}
          ( info_assignment                     {{ }}
          )*
          ( field_decl                          {{ if len (sfields): raise OverflowError ("too many fields in sequence") }}
                                                {{ sfields = sfields + field_decl }}
          )
          ( info_assignment                     {{ }}
          )*
        '}' ';'                                 {{ yy.nsadd_sequence (IDENT, sfields) }}

rule const_assignment:
        'Const' IDENT '=' expression ';'        {{ AIn (IDENT); yy.nsadd_const (IDENT, expression); }}
rule info_assignment:
        'Info' IDENT '=' expression ';'         {{ AIn (IDENT); }} # FIXME

# for operator precedence, see: http://docs.python.org/2/reference/expressions.html
rule expression: or_expr                        {{ return or_expr }}
rule or_expr:
          xor_expr                              {{ result = xor_expr }}
        ( '\|' or_expr                          {{ AN (result); result = result | or_expr }}
        )*                                      {{ return result }}
rule xor_expr:
          and_expr                              {{ result = and_expr }}
        ( '\^' xor_expr                         {{ AN (result); result = result ^ xor_expr }}
        )*                                      {{ return result }}
rule and_expr:
          shift_expr                            {{ result = shift_expr }}
        ( '&' and_expr                          {{ AN (result); result = result & and_expr }}
        )*                                      {{ return result }}
rule shift_expr:
          summation                             {{ result = summation }}
        ( '<<' shift_expr                       {{ AN (result); result = result << shift_expr }}
        | '>>' shift_expr                       {{ AN (result); result = result >> shift_expr }}
        )*                                      {{ return result }}
rule summation:
          factor                                {{ result = factor }}
        ( '\+' summation                        {{ AN (result); result = result + summation }}
        | '-'  summation                        {{ result = result - summation }}
        )*                                      {{ return result }}
rule factor:
          unary                                 {{ result = unary }}
        ( '\*' factor                           {{ result = result * factor }}
        | '/'  factor                           {{ result = result / factor }}
        | '%'  factor                           {{ AN (result); result = result % factor }}
        )*                                      {{ return result }}
rule unary:
          power                                 {{ return power }}
        | '\+' unary                            {{ return +unary }}
        | '-'  unary                            {{ return -unary }}
        | '~'  unary                            {{ return ~unary }}
rule power:
          term                                  {{ result = term }}
        ( '\*\*' unary                          {{ result = result ** unary }}
        )*                                      {{ return result }}
rule term:                                      # numerical/string term
          '(TRUE|True|true)'                    {{ return 1; }}
        | '(FALSE|False|false)'                 {{ return 0; }}
        | IDENT                                 {{ result = constant_lookup (IDENT); }}
          (string                               {{ ASp (result, IDENT); ASp (string); result += string }}
          )*                                    {{ return result }}
        | INTEGER                               {{ return int (INTEGER); }}
        | HEXINT                                {{ return int (HEXINT, 16); }}
        | FULLFLOAT                             {{ return float (FULLFLOAT); }}
        | FRACTFLOAT                            {{ return float (FRACTFLOAT); }}
        | '\(' expression '\)'                  {{ return expression; }}
        | string                                {{ return string; }}

rule string:
          '_' '\(' plain_string '\)'            {{ return '_(' + plain_string + ')' }}
        | plain_string                          {{ return plain_string }}
rule plain_string:
        STRING                                  {{ result = quote (eval (STRING)) }}
        ( ( STRING                              {{ result = quote (unquote (result) + eval (STRING)) }}
          | IDENT                               {{ con = constant_lookup (IDENT); ASp (con, IDENT) }}
                                                {{ result = quote (unquote (result) + unquote (con)) }}
          ) )*                                  {{ return result }}
%%

