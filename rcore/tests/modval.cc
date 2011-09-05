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
#include <rcore/testutils.hh>
#include <rapicorn-core.hh>

namespace {
using namespace Rapicorn;

struct TestObject : public virtual BaseObject {};

#if 0

static void
test_array_auto_value ()
{
  Array a1, a2;
  TestObject *to = new TestObject();
  ref_sink (to);
  a1.push_head (AutoValue ("first")); // demand create AutoValue
  a1.push_head (String ("second")); // implicit AutoValue creation
  a1.push_head ("third"); // implicit AutoValue creation from C string
  // implicit AutoValue creations
  a1.push_head (0.1);
  a1.push_head (a2);
  a1.push_head (*to);
  unref (to);
}
REGISTER_TEST ("Modval/Array/AutoValue", test_array_auto_value);

static void
test_array ()
{
  Array a;
  // [0] == head
  a.push_head (AnyValue (INT, 0));
  assert (a[0].asint() == 0);
  // [-1] == tail
  assert (a[-1].asint() == 0);
  a.push_head (AnyValue (INT, 1));
  assert (a[0].asint() == 1);
  assert (a[-1].asint() == 0);
  assert (a.count() == 2);
  // out-of-range-int => string-key conversion:
  a[99] = 5;
  assert (a.key (2) == "99"); // count() was 2 before inserting
  assert (a[2].asint() == 5);
  assert (a[-1].asint() == 5);
  assert (5 == (int) a[-1]);
  assert (a.count() == 3);
  a.push_tail (AnyValue (INT, 2));
  assert (a[-1].asint() == 2);
  assert (a.count() == 4);

  a.clear();
  a["ape"] = AnyValue (STRING, "affe");
  a["beard"] = "bart";
  assert (a[-1].string() == "bart");
  a["anum"] = 5;
  assert (a.key (-3) == "ape");
  assert (a.pop_head().string() == "affe");
  assert (a.pop_head().string() == "bart");
  assert (a.key (-1) == "anum");
  assert (a.pop_head().asint() == 5);
  assert (a.count() == 0);

  const char *board[][8] = {
    { "R","N","B","Q","K","B","N","R" },
    { "P","P","P","P","P","P","P","P" },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { " "," "," "," "," "," "," "," " },
    { "p","p","p","p","p","p","p","p" },
    { "r","n","b","q","k","b","n","r" }
  };
  for (size_t i = 0; i < RAPICORN_ARRAY_SIZE (board); i++)
    a.push_tail (Array::FromCArray<String> (board[i]));
  if (Test::verbose())
    printout ("chess0:\n%s\n", a.to_string ("\n").c_str());

  a[3].array()[4] = a[1].array()[4];
  a[1].array()[4] = " ";

  if (Test::verbose())
    printout ("\nchess1:\n%s\n", a.to_string ("\n").c_str());
}
REGISTER_TEST ("Modval/Array/Chess", test_array);

static void
variable_changed (Variable *self)
{
  if (Test::verbose())
    printout ("Variable::changed: %s\n", self->type().name().c_str());
}

static void
test_variable ()
{
  Plic::TypeCode somestring = Plic::TypeMap::lookup ("string");
  Variable &v1 = *new Variable (somestring);
  ref_sink (v1);
  v1.sig_changed += slot (variable_changed, &v1);
  v1 = "foohoo";
  String as_string = v1;
  assert (as_string == "foohoo");
  v1 = 5;
  assert (v1.get<String>() == "5");
  v1.set (6.0);
  assert (v1.get<String>() == "6");
  bool b = v1;
  int i = v1;
  uint ui = v1;
  double d = v1;
  (void) (b + i + ui + d); // silence compiler
  unref (v1);
}
REGISTER_TEST ("Modval/Variable/notification", test_variable);

static void
test_store1_string_row ()
{
  Plic::TypeCode t1 = Plic::TypeMap::lookup ("string");
  assert (t1.kind() == Plic::STRING);
  Store1 *s1 = Store1::create_memory_store ("models/mod1tst/string-row", t1, SELECTION_NONE);
  assert (s1 != NULL);
  /* assert model/store identity (for memory stores) */
  Model1 &m1 = s1->model();
  assert (s1 == m1.store());
  /* basic store assertions */
  assert (s1->count() == 0);
  /* insert first row */
  Array ra;
  ra.push_head (AutoValue ("first"));
  s1->insert (0, ra);
  assert (s1->count() == 1);
}
REGISTER_TEST ("Modval/Store-string-row", test_store1_string_row);

static void
test_store1_array ()
{
  /* create storage model with row type */
  Plic::TypeCode t1 = Plic::TypeMap::lookup ("string"); // FIXME: use "record" ?
  Store1 *s1 = Store1::create_memory_store ("models/mod1tst/array", t1, SELECTION_NONE);
  assert (s1 != NULL);
  assert (s1->count() == 0);
  /* insert first row */
  Array row;
  row[-1] = "a C string";
  row[-1] = 0.5; // double
  row[-1] = 17; // integer
  s1->insert (0, row);
  assert (s1->count() == 1);
}
REGISTER_TEST ("Modval/Store-array-row", test_store1_array);

static Store1&
create_dummy_store1 ()
{
  Store1 &store = *Store1::create_memory_store ("models/mod1tst/dummy",
                                                Plic::TypeMap::lookup ("string"),
                                                SELECTION_NONE);
  for (uint i = 0; i < 4; i++)
    {
      Array row;
      for (uint j = 0; j < 4; j++)
        row[j] = string_printf ("%u-%u", i + 1, j + 1);
      store.insert (-1, row);
    }
  return store;
}

static String
stringify_model (Model1 &model)
{
  String s = "[\n";
  for (uint i = 0; i < model.count(); i++)
    {
      Array row = model.get (i);
      s += "  (";
      for (uint j = 0; j < row.count(); j++)
        s += string_printf ("%s,", row[j].string().c_str());
      s += "),\n";
    }
  s += "]";
  return s;
}

static void
test_store1_row_ops ()
{
  Store1 &store = create_dummy_store1();
  Model1 &model = store.model();
  Array row;

  // store operations
  row[0] = "Newly_appended_last_row";
  store.insert (-1, row);
  row[0] = "Newly_prepended_first_row";
  store.insert (0, row);

  row = model.get (1);
  row[4] = "Extra_text_added_to_row_1";
  store.update (1, row);

  row = model.get (2);
  store.remove (2);
  row.clear();
  row[0] = "Replacement_for_removed_row_2";
  store.insert (2, row);

  // test verification
  String e = "[\n  (Newly_prepended_first_row,),\n  (1-1,1-2,1-3,1-4,Extra_text_added_to_row_1,),\n"
             "  (Replacement_for_removed_row_2,),\n  (3-1,3-2,3-3,3-4,),\n  (4-1,4-2,4-3,4-4,),\n"
             "  (Newly_appended_last_row,),\n]";
  String s = stringify_model (model);
  // printout ("\nMODEL: %s\n", s.c_str());
  assert (e == s);

  // cleanup
  unref (store);
}
REGISTER_TEST ("Modval/Store-row-ops", test_store1_row_ops);

static void
test_store1_row_test ()
{
  /* creating a model 1D test:
   * 1: create array type with fixed field count
   * 2: create list store from type
   * 3: create row with random field values
   * 4: add row to list
   * 5: fetc row
   * 6: verify altered field values
   */
}
REGISTER_TEST ("Modval/Store-row-test", test_store1_row_test);
#endif

} // Anon
