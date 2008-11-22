/* Rapicorn
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "types.hh"

namespace Rapicorn {

#define IS_RAPICORN_TYPE_STORAGE(strg)  (strchr (RAPICORN_TYPE_STORAGE_CHARS, strg))

const char*
Type::type_name (Type::Storage storage)
{
  switch (storage)
    {
    case Type::NUM:             return "NUM";
    case Type::FLOAT:           return "FLOAT";
    case Type::STRING:          return "STRING";
    case Type::ARRAY:           return "ARRAY";
    case Type::STRING_VECTOR:   return "STRING_VECTOR";
    case Type::OBJECT:          return "OBJECT";
    }
  return NULL;
}

static const char*
parse_hexnum (const char *input,
              uint       &pos,
              int64      &result,
              uint        n_digits)
{
  static const char *NFE = "hexnum parse error";
  result = 0;
  while (n_digits--)
    {
      if (!strchr ("ABCDEFabcdef0123456789", input[pos]))
        return NFE;
      result <<= 4;
      result += input[pos] >= 'a' ? input[pos] - 'a' + 10 : input[pos] >= 'A' ? input[pos] - 'A' + 10 : input[pos] > '0' ? input[pos] - '0' : 0;
      pos++;
    }
  return NULL;
}

static const char*
parse_string (const char *input,
              uint       &pos,
              String     &result,
              uint        n_digits)
{
  static const char *SSE = "short string encountered";
  int64 n;
  const char *err = parse_hexnum (input, pos, n, n_digits);
  if (err)
    return err;
  while (n--)
    {
      if (!input[pos])
        return SSE;
      result += input[pos++];
    }
  return NULL;
}

static const char*
parse_assignment (const char *assignment,
                  uint       *offset)
{
  static const char *IKA = "invalid key=value assignment syntax";
  if (!((assignment[0] >= 'A' && assignment[0] <= 'Z') ||
        (assignment[0] >= 'a' && assignment[0] <= 'z') ||
        (assignment[0] >= '0' && assignment[0] <= '9') ||
        assignment[0] == '_'))
    return IKA;
  uint i = 1;
  while (strchr ("abcdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ", assignment[i]))
    i++;
  if (assignment[i] != '=')
    return IKA;
  *offset = i + 1;
  return NULL;
}

static const char*
type_parser (char const *input,
             Type::Info &ti)
{
  static const char *ITD = "format error";
  /* format:
   * <StorageChar>US
   */
  uint pos = 0;
  // parse magic indicator
  if (input[pos++] != 'R')
    return ITD;
  if (input[pos++] != 'c')
    return ITD;
  if (input[pos++] != 'T')
    return ITD;
  if (input[pos++] != 'i')
    return ITD;
  if (input[pos++] != ';')
    return ITD;
  // parse type name
  const char *err = parse_string (input, pos, ti.name, 4);
  if (err)
    return err;
  if (input[pos++] != ';')
    return ITD;
  // parse storage type
  ti.storage = Type::Storage (input[pos++]);
  if (!ti.storage || !IS_RAPICORN_TYPE_STORAGE (ti.storage))
    return ITD;
  if (input[pos++] != ';')
    return ITD;
  // parse aux data
  while (input[pos] && input[pos] != ';')
    {
      uint offset = 0;
      String astr;
      err = parse_string (input, pos, astr, 4);
      if (err)
        return err;
      err = parse_assignment (astr.c_str(), &offset);
      if (err)
        return err;
      String name = String (astr, 0, offset - 1);
      String value = String (astr, offset);
      ti.auxdata.push_back (name + "=" + value);
    }
  if (input[pos++] != ';')
    return ITD;
  return NULL;
}

Type::Type (char const *_typedef) :
  m_info()
{
  const char *err = type_parser (_typedef, m_info);
  if (err)
    RAPICORN_ERROR ("invalid type definition (%s): \"%s\"", err, string_to_cquote (_typedef).c_str());
}

String
Type::ident () const
{
  return m_info.name;
}

String
Type::label () const
{
  String l = aux_string ("label");
  if (l.size() == 0)
    return ident();
  return l;
}

String
Type::blurb () const
{
  return aux_string ("blurb");
}

String
Type::hints () const
{
  String h = aux_string ("hints");
  if (h.size() == 0 || h[0] != ':')
    h = ":" + h;
  if (h[h.size()-1] != ':')
    h = h + ":";
  return h;
}

Type::Storage
Type::storage () const
{
  return m_info.storage;
}

String
Type::aux_string (const String &auxname) const
{
  if (!strchr (auxname.c_str(), '='))
    {
      String prefix = auxname + "=";
      for (vector<String>::const_iterator it = m_info.auxdata.begin(); it != m_info.auxdata.end(); it++)
        if ((*it).compare (0, prefix.size(), prefix) == 0)
          return (*it).c_str() + prefix.size();
    }
  return "";
}

long double
Type::aux_float (const String &auxname) const
{
  return string_to_double (aux_string (auxname));
}

int64
Type::aux_num (const String &auxname) const
{
  return string_to_int (aux_string (auxname));
}

#define return_if(cond,string)     do { if (cond) return string; } while (0)

struct TypeInfo : public virtual ReferenceCountImpl {
  const char   *type_string;
  uint          type_string_length;
  Type::Storage type_storage;
  uint          name_offset, name_length;
  vector<uint>  auxkey_offsets; // last member points after last aux key
};

struct Typ2::Info : public TypeInfo {
  String type_string_mem;
};

String
Typ2::name () const
{
  return String (m_info->type_string + m_info->name_offset, m_info->name_length);
}

static String
parse_int (const char **tsp,
           const char  *boundary,
           uint        *ui)
{
  return_if (*tsp + 4 > boundary, "premature end");
  uint8 *us = (uint8*) *tsp, u0 = us[0], u1 = us[1], u2 = us[2], u3 = us[3];
  if (u0 < 128 || u1 < 128 || u2 < 128 || u3 < 128)
    return "invalid integer encoding";
  *ui = (u3 & 0x7f) + ((u2 & 0x7f) >> 1) + ((u1 & 0x7f) >> 2) + ((u0 & 0x7f) >> 3);
  *tsp += 4;
  return "";
}

static String
parse_strings (uint          count,
               const char   *tstart,
               const char  **tsp,
               const char   *boundary,
               vector<uint> *offsets,
               const String &errpart)
{
  uint ui = 0;
  for (uint i = 0; i < count; i++)
    {
      String err = parse_int (tsp, boundary, &ui);
      return_if (err != "", string_printf ("%s while parsing %s", err.c_str(), errpart.c_str()));
      return_if (*tsp + ui > boundary, string_printf ("premature end while parsing %s", errpart.c_str()));
      offsets->push_back (*tsp - tstart);
      *tsp += ui;
    }
  offsets->push_back (*tsp - tstart);
  assert (offsets->size() == count + 1); // paranoid
  return "";
}

static String
parse_type_info (TypeInfo    &self,
                 const char  *type_string_start,
                 const char **tsp,
                 const char  *boundary)
{
  /* parse type info:
   * - LLLL                   // length of upcoming type info data
   * - ___?                   // storage type
   * - xxxx                   // number of aux entries
   * - [ llll AuxEntry ]*     // xxxx times aux entry strings
   * - CHOICE:    yyyy [ llll Ident llll Label llll Blurb ]+
   * - RECORD:    yyyy [ llll FieldName _TYPE_INFO_ ]+
   * - SEQUENCE:  yyyy FieldName _TYPE_INFO_
   * - INTERFACE: yyyy [ llll InterfaceTypeName ]+
   */
  String err;
  uint ui = 0;
  // type length
  err = parse_int (tsp, boundary, &ui);
  return_if (err != "", err + " in type info length");
  return_if (*tsp + ui > boundary, "type info data too short");
  boundary = MIN (*tsp + ui, boundary);
  // storage type
  return_if (*tsp + 4 > boundary, "premature end at storage type");
  if (strncmp (*tsp, "___", 3) != 0 || !strchr (RAPICORN_TYP2_STORAGE_CHARS, (*tsp)[3]))
    return "invalid storage type";
  self.type_storage = Type::Storage ((*tsp)[3]);
  *tsp += 4;
  // number of aux entries
  err = parse_int (tsp, boundary, &ui);
  return_if (err != "", err + " at aux entries");
  return_if (*tsp + ui * (4 + 1) > boundary, "premature end in aux data");
  // parse aux info
  self.auxkey_offsets.clear();
  if (ui)
    {
      err = parse_strings (ui, type_string_start, tsp, boundary, &self.auxkey_offsets, "aux infos");
      return_if (err != "", err);
      *tsp += self.auxkey_offsets[self.auxkey_offsets.size() - 1];
    }
  // skip across unrecognized type data
  *tsp = boundary;
  // FIXME: 0xfe00000 0x1fc000 0x3f80 0x7f
  return "";
}

static String
parse_offsets (TypeInfo    &self,
               const char *ts,
               uint        _tsl)
{
  /* parse a type definition like:
   * - GTyp e001              // magic
   * - llll TypeNameString    // type name with length
   * - LLLL TypeInfo_data     // main type info with length
   */
  self.type_string = ts;
  self.type_string_length = _tsl;
  const char *tb = ts + _tsl;
  String err;
  if (!self.type_string || self.type_string_length == 0 || self.type_string[0] == 0)
    return "Empty type string";
  if (ts + 24 > tb)
    return "type info string too short";
  // check magic
  if (strncmp (self.type_string, "GType001", 8) != 0)
    return "Invalid/unknown type info string";
  ts += 8;
  // type name
  vector<uint> svo;
  err = parse_strings (1, self.type_string, &ts, tb, &svo, "type name");
  return_if (err != "", err);
  self.name_offset = svo[0];
  self.name_length = svo[1] - svo[0];
  return_if (self.name_length < 1, "Invalid type name");
  // parse main type info
  err = parse_type_info (self, self.type_string, &ts, tb);
  return_if (err != "", err);
  // done
  return "";
}

Typ2::Typ2 (Info &tinfo) :
  m_info (ref (&tinfo))
{}

Typ2::Typ2 (const Typ2 &src) :
  m_info (ref (src.m_info))
{}

Typ2::~Typ2 ()
{
  Info *old_info = m_info;
  m_info = NULL;
  if (old_info)
    unref (old_info);
}

Typ2
Typ2::from_type_info (const char *rom_type_info_string,
                      uint        type_info_string_length)
{
  Typ2::Info *type_info = new Typ2::Info();
  ref_sink (type_info);
  /* we rely on caller owned memory to stay around
   * // type_info.type_string_mem = String (rom_type_info_string, type_info_string_length);
   */
  String err = parse_offsets (*type_info, rom_type_info_string, type_info_string_length);
  if (err != "")
    error ("%s: %s", STRFUNC, err.c_str());
  Typ2 type (*type_info);
  unref (type_info);
  return type;
}

Typ2* // caller owns Typ2*
Typ2::from_type_info (const String &type_info_string,
                      String       &error)
{
  Typ2::Info *type_info = new Typ2::Info();
  ref_sink (type_info);
  type_info->type_string_mem = type_info_string; // copy and save type_info_string
  error = parse_offsets (*type_info, type_info->type_string_mem.data(), type_info->type_string_mem.size());
  Typ2 *tp = NULL;
  if (error == "")
    tp = new Typ2 (*type_info);
  return tp;
}

} // Rapicorn
