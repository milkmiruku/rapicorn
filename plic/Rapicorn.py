#!/usr/bin/env python
# plic - Pluggable IDL Compiler                                -*-mode:python-*-
# Copyright (C) 2008 Tim Janik
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
"""PLIC-Rapicorn - Rapicorn type generator for PLIC

More details at http://www.testbit.eu/.
"""
import Decls

def encode_int (int):
  assert int < 268435456
  chars = (128 + (int >> 21),
           128 + ((int >> 14) & 0x7f),
           128 + ((int >> 7) & 0x7f),
           128 + (int & 0x7f))
  return "%c%c%c%c" % chars
def encode_string (string):
  return encode_int (len (string)) + string

def strcquote (string):
  result = ''
  for c in string:
    oc = ord (c)
    if oc <= 31 or oc >= 127:
      result += '\\' + oct (oc)[-3:]
    elif c == '"':
      result += r'\"'
    else:
      result += c
  return '"' + result + '"'

class Generator:
  def aux_strings (self, auxlist):
    result = encode_int (len (auxlist))
    for ad in auxlist:
      result += encode_string (ad)
    return result
  def type_info (self, type_info, unfoldrefs = False):
    tp = type_info
    s = { Decls.NUM       : '___i',
          Decls.REAL      : '___f',
          Decls.STRING    : '___s',
          Decls.ENUM      : '___e',
          Decls.SEQUENCE  : '___q',
          Decls.RECORD    : '___r',
          Decls.INTERFACE : '___c',
          }[tp.storage]
    aux = []
    s += self.aux_strings (aux)
    if tp.storage == Decls.SEQUENCE:
      s += encode_string (tp.elements[0])
      s += self.type_info (tp.elements[1])
    elif tp.storage == Decls.RECORD:
      s += encode_int (len (tp.fields))
      for fl in tp.fields:
        s += encode_string (fl[0])
        s += self.type_info (fl[1])
    elif tp.storage == Decls.ENUM:
      s += encode_int (len (tp.options))
      for op in tp.options:
        s += encode_string (op[0]) # ident
        s += encode_string (op[1]) # label
        s += encode_string (op[2]) # blurb
    elif tp.storage == Decls.INTERFACE:
      tp.prerequisites = []
      s += encode_int (len (tp.prerequisites))
      for pr in prerequisites:
        s += encode_string (pr)
    return encode_int (len (s)) + s
  def type_string (self, type_info):
    s = 'Rpcrn001'      # magic
    s += encode_string (type_info.name)
    s += self.type_info (type_info, True)
    return s
  def generate (self, namespace):
    for tp in namespace.types():
      print "TYPE:", strcquote (self.type_string (tp))

def generate (namespace_list, *args):
  gg = Generator()
  for ns in namespace_list:
    gg.generate (ns)

# control module exports
__all__ = ['generate']
