/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include "sfiprimitives.h"
#include "sfimemory.h"
#include "sfiparams.h"


/* --- SfiBBlock primitive type --- */
SfiBBlock*
sfi_bblock_new (void)
{
  SfiBBlock *bblock = sfi_new_struct (SfiBBlock, 1);

  bblock->ref_count = 1;
  bblock->n_bytes = 0;
  bblock->bytes = NULL;
  return bblock;
}

SfiBBlock*
sfi_bblock_new_sized (guint size)
{
  SfiBBlock *bblock = sfi_bblock_new ();
  sfi_bblock_resize (bblock, size);
  return bblock;
}

SfiBBlock*
sfi_bblock_ref (SfiBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, NULL);
  g_return_val_if_fail (bblock->ref_count > 0, NULL);

  bblock->ref_count++;
  return bblock;
}

void
sfi_bblock_unref (SfiBBlock *bblock)
{
  g_return_if_fail (bblock != NULL);
  g_return_if_fail (bblock->ref_count > 0);

  bblock->ref_count--;
  if (bblock->ref_count == 0)
    {
      g_free (bblock->bytes);
      sfi_delete_struct (SfiBBlock, bblock);
    }
}

void
sfi_bblock_resize (SfiBBlock *bblock,
		   guint      size)
{
  guint i;

  g_return_if_fail (bblock != NULL);

  i = bblock->n_bytes;
  bblock->n_bytes = size;
  bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
  if (size > i)
    memset (bblock->bytes + i, 0, sizeof (bblock->bytes[0]) * (size - i));
}

SfiBBlock*
sfi_bblock_copy_deep (const SfiBBlock *bblock)
{
  SfiBBlock *fb;

  g_return_val_if_fail (bblock != NULL, NULL);
  g_return_val_if_fail (bblock->ref_count > 0, NULL);

  fb = sfi_bblock_new ();
  fb->n_bytes = bblock->n_bytes;
  fb->bytes = g_memdup (bblock->bytes, bblock->n_bytes * sizeof (bblock->bytes[0]));
  return fb;
}

void
sfi_bblock_append (SfiBBlock    *bblock,
		   guint         n_bytes,
		   const guint8 *bytes)
{
  g_return_if_fail (bblock != NULL);

  if (n_bytes)
    {
      guint i;

      g_return_if_fail (bytes != NULL);

      i = bblock->n_bytes;
      bblock->n_bytes += n_bytes;
      bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
      memcpy (bblock->bytes + i, bytes, n_bytes * sizeof (bblock->bytes[0]));
    }
}

void
sfi_bblock_append1 (SfiBBlock *bblock,
		    guint8     byte0)
{
  guint i;

  g_return_if_fail (bblock != NULL);

  i = bblock->n_bytes++;
  bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
  bblock->bytes[i] = byte0;
}

guint
sfi_bblock_length (const SfiBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, 0);
  return bblock->n_bytes;
}

guint8*
sfi_bblock_get (const SfiBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, NULL);
  return bblock->bytes;
}


/* --- SfiFBlock primitive type --- */
SfiFBlock*
sfi_fblock_new (void)
{
  SfiFBlock *fblock = sfi_new_struct (SfiFBlock, 1);

  fblock->ref_count = 1;
  fblock->n_values = 0;
  fblock->values = NULL;
  return fblock;
}

SfiFBlock*
sfi_fblock_new_sized (guint size)
{
  SfiFBlock *fblock = sfi_fblock_new ();
  sfi_fblock_resize (fblock, size);
  return fblock;
}

SfiFBlock*
sfi_fblock_ref (SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fblock->ref_count++;
  return fblock;
}

void
sfi_fblock_unref (SfiFBlock *fblock)
{
  g_return_if_fail (fblock != NULL);
  g_return_if_fail (fblock->ref_count > 0);

  fblock->ref_count--;
  if (fblock->ref_count == 0)
    {
      g_free (fblock->values);
      sfi_delete_struct (SfiFBlock, fblock);
    }
}

void
sfi_fblock_resize (SfiFBlock *fblock,
		   guint      size)
{
  guint i;

  g_return_if_fail (fblock != NULL);

  i = fblock->n_values;
  fblock->n_values = size;
  fblock->values = g_renew (gfloat, fblock->values, fblock->n_values);
  if (size > i)
    memset (fblock->values + i, 0, sizeof (fblock->values[0]) * (size - i));
}

SfiFBlock*
sfi_fblock_copy_deep (const SfiFBlock *fblock)
{
  SfiFBlock *fb;

  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fb = sfi_fblock_new ();
  fb->n_values = fblock->n_values;
  fb->values = g_memdup (fblock->values, fblock->n_values * sizeof (fblock->values[0]));
  return fb;
}

void
sfi_fblock_append (SfiFBlock    *fblock,
		   guint         n_values,
		   const gfloat *values)
{
  g_return_if_fail (fblock != NULL);

  if (n_values)
    {
      guint i;

      g_return_if_fail (values != NULL);

      i = fblock->n_values;
      fblock->n_values += n_values;
      fblock->values = g_renew (gfloat, fblock->values, fblock->n_values);
      memcpy (fblock->values + i, values, n_values * sizeof (fblock->values[0]));
    }
}

void
sfi_fblock_append1 (SfiFBlock *fblock,
		    gfloat     float0)
{
  guint i;

  g_return_if_fail (fblock != NULL);

  i = fblock->n_values++;
  fblock->values = g_renew (gfloat, fblock->values, fblock->n_values);
  fblock->values[i] = float0;
}

guint
sfi_fblock_length (const SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, 0);
  return fblock->n_values;
}

gfloat*
sfi_fblock_get (const SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  return fblock->values;
}


/* --- SfiSeq primitive type --- */
SfiSeq*
sfi_seq_new (void)
{
  SfiSeq *s;

  s = sfi_new_struct (SfiSeq, 1);
  s->ref_count = 1;
  s->n_elements = 0;
  s->elements = NULL;
  return s;
}

SfiSeq*
sfi_seq_ref (SfiSeq *seq)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  seq->ref_count++;
  return seq;
}

void
sfi_seq_clear (SfiSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  while (seq->n_elements)
    g_value_unset (seq->elements + --seq->n_elements);
  g_free (seq->elements);
  seq->elements = NULL;
}

void
sfi_seq_unref (SfiSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  seq->ref_count--;
  if (seq->ref_count == 0)
    {
      while (seq->n_elements)
	g_value_unset (seq->elements + --seq->n_elements);
      g_free (seq->elements);
      sfi_delete_struct (SfiSeq, seq);
    }
}

static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

static void
sfi_seq_append_copy (SfiSeq       *seq,
		     const GValue *value,
		     gboolean      deep_copy)
{
  guint i, l, n;

  g_return_if_fail (seq != NULL);
  g_return_if_fail (SFI_IS_VALUE (value));

  l = upper_power2 (seq->n_elements);
  i = seq->n_elements++;
  n = upper_power2 (seq->n_elements);
  if (n > l)
    {
      seq->elements = g_realloc (seq->elements, n * sizeof (seq->elements[0]));
      memset (seq->elements + l, 0, (n - l) * sizeof (seq->elements[0]));
    }
  g_value_init (seq->elements + i, G_VALUE_TYPE (value));
  if (deep_copy)
    sfi_value_copy_deep (value, seq->elements + i);
  else
    g_value_copy (value, seq->elements + i);
}

SfiSeq*
sfi_seq_copy_deep (const SfiSeq *seq)
{
  SfiSeq *s;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  s = sfi_seq_new ();
  for (i = 0; i < seq->n_elements; i++)
    sfi_seq_append_copy (s, seq->elements + i, TRUE);
  return s;
}

void
sfi_seq_append (SfiSeq       *seq,
		const GValue *value)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  sfi_seq_append_copy (seq, value, FALSE);
}

guint
sfi_seq_length (const SfiSeq *seq)
{
  return seq ? seq->n_elements : 0;
}

GValue*
sfi_seq_get (const SfiSeq *seq,
	     guint         index)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (index < seq->n_elements, NULL);
  
  return seq->elements + index;
}

gboolean
sfi_seq_check (SfiSeq *seq,
	       GType   element_type)
{
  guint i;
  
  g_return_val_if_fail (seq != NULL, FALSE);
  
  for (i = 0; i < seq->n_elements; i++)
    if (!G_VALUE_HOLDS (seq->elements + i, element_type))
      return FALSE;
  return TRUE;
}

void
sfi_seq_append_bool (SfiSeq      *seq,
		     SfiBool      v_bool)
{
  GValue *value = sfi_value_bool (v_bool);
  sfi_seq_append (seq, value);
  sfi_value_free (value);
}

void
sfi_seq_append_int (SfiSeq      *seq,
		    SfiInt       v_int)
{
  GValue *value = sfi_value_int (v_int);
  sfi_seq_append (seq, value);
  sfi_value_free (value);
}

void
sfi_seq_append_num (SfiSeq      *seq,
		    SfiNum       v_num)
{
  GValue *value = sfi_value_num (v_num);
  sfi_seq_append (seq, value);
  sfi_value_free (value);
}

void
sfi_seq_append_real (SfiSeq          *seq,
		     SfiReal         v_real)
{
  GValue *value = sfi_value_real (v_real);
  sfi_seq_append (seq, value);
  sfi_value_free (value);
}

void
sfi_seq_append_string (SfiSeq      *seq,
		       const gchar *string)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_STRING);
  g_value_set_static_string (&value, string);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_choice (SfiSeq      *seq,
		       const gchar *choice)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_CHOICE);
  g_value_set_static_string (&value, choice);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_bblock (SfiSeq      *seq,
		       SfiBBlock   *bblock)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_BBLOCK);
  g_value_set_static_boxed (&value, bblock);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_fblock (SfiSeq      *seq,
		       SfiFBlock   *fblock)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_FBLOCK);
  g_value_set_static_boxed (&value, fblock);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_pspec (SfiSeq      *seq,
		      GParamSpec  *pspec)
{
  GValue *value = sfi_value_pspec (pspec);
  sfi_seq_append (seq, value);
  sfi_value_free (value);
}

void
sfi_seq_append_seq (SfiSeq      *seq,
		    SfiSeq      *v_seq)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_SEQ);
  g_value_set_static_boxed (&value, v_seq);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_rec (SfiSeq      *seq,
		    SfiRec      *rec)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_REC);
  g_value_set_static_boxed (&value, rec);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

void
sfi_seq_append_proxy (SfiSeq      *seq,
		      SfiProxy     proxy)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_PROXY);
  sfi_value_set_proxy (&value, proxy);
  sfi_seq_append (seq, &value);
  g_value_unset (&value);
}

static inline SfiNum
value_as_num (GValue *v)
{
  if (v)
    {
      if (SFI_VALUE_HOLDS_BOOL (v))
	return sfi_value_get_bool (v);
      else if (SFI_VALUE_HOLDS_INT (v))
	return sfi_value_get_int (v);
      else if (SFI_VALUE_HOLDS_REAL (v))
	return sfi_value_get_real (v);
      else if (SFI_VALUE_HOLDS_NUM (v))
	return sfi_value_get_num (v);
    }
  return 0;
}

static inline SfiReal
value_as_real (GValue *v)
{
  if (v)
    {
      if (SFI_VALUE_HOLDS_BOOL (v))
	return sfi_value_get_bool (v);
      else if (SFI_VALUE_HOLDS_INT (v))
	return sfi_value_get_int (v);
      else if (SFI_VALUE_HOLDS_REAL (v))
	return sfi_value_get_real (v);
      else if (SFI_VALUE_HOLDS_NUM (v))
	return sfi_value_get_num (v);
    }
  return 0;
}

static inline const gchar*
value_as_string (GValue *v)
{
  if (v)
    {
      if (SFI_VALUE_HOLDS_STRING (v))
	return sfi_value_get_string (v);
      else if (SFI_VALUE_HOLDS_CHOICE (v))
	return sfi_value_get_choice (v);
    }
  return NULL;
}

SfiBool
sfi_seq_get_bool (SfiSeq *seq,
		  guint   index)
{
  return value_as_num (sfi_seq_get (seq, index)) != 0;
}

SfiInt
sfi_seq_get_int (SfiSeq *seq,
		 guint   index)
{
  return value_as_num (sfi_seq_get (seq, index));
}

SfiNum
sfi_seq_get_num (SfiSeq *seq,
		 guint   index)
{
  return value_as_num (sfi_seq_get (seq, index));
}

SfiReal
sfi_seq_get_real (SfiSeq *seq,
		  guint   index)
{
  return value_as_real (sfi_seq_get (seq, index));
}

const gchar*
sfi_seq_get_string (SfiSeq *seq,
		    guint   index)
{
  return value_as_string (sfi_seq_get (seq, index));
}

const gchar*
sfi_seq_get_choice (SfiSeq *seq,
		    guint   index)
{
  return value_as_string (sfi_seq_get (seq, index));
}

SfiBBlock*
sfi_seq_get_bblock (SfiSeq *seq,
		    guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_BBLOCK (v))
    return sfi_value_get_bblock (v);
  return NULL;
}

SfiFBlock*
sfi_seq_get_fblock (SfiSeq *seq,
		    guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_FBLOCK (v))
    return sfi_value_get_fblock (v);
  return NULL;
}

GParamSpec*
sfi_seq_get_pspec (SfiSeq *seq,
		   guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_PSPEC (v))
    return sfi_value_get_pspec (v);
  return NULL;
}

SfiSeq*
sfi_seq_get_seq (SfiSeq *seq,
		 guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_SEQ (v))
    return sfi_value_get_seq (v);
  return NULL;
}

SfiRec*
sfi_seq_get_rec (SfiSeq *seq,
		 guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_REC (v))
    return sfi_value_get_rec (v);
  return NULL;
}

SfiProxy
sfi_seq_get_proxy (SfiSeq *seq,
		   guint   index)
{
  GValue *v = sfi_seq_get (seq, index);
  if (v && SFI_VALUE_HOLDS_PROXY (v))
    return sfi_value_get_proxy (v);
  return 0;
}

gchar**
sfi_seq_to_strv (SfiSeq *seq)
{
  GSList *slist = NULL;
  gchar **strv;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);

  for (i = 0; i < seq->n_elements; i++)
    if (G_VALUE_HOLDS_STRING (seq->elements + i))
      slist = g_slist_prepend (slist, sfi_value_get_string (seq->elements + i));
  slist = g_slist_reverse (slist);
  strv = g_strslistv (slist);
  g_slist_free (slist);
  return strv;
}

SfiSeq*
sfi_seq_from_strv (gchar **strv)
{
  SfiSeq *seq;
  guint i;
  if (!strv)
    return NULL;

  seq = sfi_seq_new ();
  for (i = 0; strv[i]; i++)
    sfi_seq_append_string (seq, strv[i]);
  return seq;
}


/* --- SfiRec primitive type --- */
SfiRec*
sfi_rec_new (void)
{
  SfiRec *rec = sfi_new_struct (SfiRec, 1);
  rec->ref_count = 1;
  rec->n_fields = 0;
  rec->sorted = TRUE;
  rec->fields = NULL;
  rec->field_names = NULL;
  return rec;
}

SfiRec*
sfi_rec_ref (SfiRec *rec)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);
  
  rec->ref_count++;
  
  return rec;
}

static void
sfi_rec_empty (SfiRec *rec)
{
  guint i;
  
  for (i = 0; i < rec->n_fields; i++)
    {
      g_value_unset (rec->fields + i);
      g_free (rec->field_names[i]);
    }
  g_free (rec->fields);
  g_free (rec->field_names);
  rec->n_fields = 0;
  rec->sorted = TRUE;
  rec->fields = NULL;
  rec->field_names = NULL;
}

void
sfi_rec_unref (SfiRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);
  
  rec->ref_count--;
  if (rec->ref_count == 0)
    {
      sfi_rec_empty (rec);
      sfi_delete_struct (SfiRec, rec);
    }
}

void
sfi_rec_clear (SfiRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);

  sfi_rec_empty (rec);
}

guint
sfi_rec_n_fields (const SfiRec *rec)
{
  g_return_val_if_fail (rec != NULL, 0);
  return rec ? rec->n_fields : 0;
}

GValue*
sfi_rec_field (const SfiRec *rec,
	       guint         index)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (index < rec->n_fields, NULL);
  
  return rec->fields + index;
}

static inline gchar*
dupcanon (const gchar *field_name)
{
  return g_strcanon (g_strdup (field_name),
		     G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
		     '-');
}

static inline guint
sfi_rec_lookup (SfiRec      *rec,
		const gchar *field_name)
{
  if (rec->sorted)
    {
      gchar **nodes = rec->field_names;
      guint n_nodes = rec->n_fields, offs = 0;
      gint cmp = 0;
      while (offs < n_nodes)
	{
	  guint i = (offs + n_nodes) >> 1;
	  cmp = strcmp (field_name, nodes[i]);
	  if (cmp == 0)
	    return i;
	  else if (cmp < 0)
	    n_nodes = i;
	  else /* (cmp > 0) */
	    offs = i + 1;
	}
    }
  else
    {
      guint i;
      for (i = 0; i < rec->n_fields; i++)
	if (strcmp (field_name, rec->field_names[i]) == 0)
	  return i;
    }
  return rec->n_fields; /* no match */
}

static void
sfi_rec_set_copy (SfiRec       *rec,
		  const gchar  *field_name,
		  const GValue *value,
		  gboolean      deep_copy)
{
  gchar *name;
  guint i;
  
  name = dupcanon (field_name);
  i = sfi_rec_lookup (rec, name);
  if (i >= rec->n_fields)
    {
      i = rec->n_fields++;
      rec->fields = g_realloc (rec->fields, rec->n_fields * sizeof (rec->fields[0]));
      memset (rec->fields + i, 0, sizeof (rec->fields[0]));
      rec->field_names = g_realloc (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      rec->field_names[i] = name;
      /* we don't sort upon insertion to speed up record creation */
      rec->sorted = FALSE;
    }
  else
    {
      g_value_unset (rec->fields + i);
      g_free (name);
    }
  g_value_init (rec->fields + i, G_VALUE_TYPE (value));
  if (deep_copy)
    sfi_value_copy_deep (value, rec->fields + i);
  else
    g_value_copy (value, rec->fields + i);
}

void
sfi_rec_set (SfiRec       *rec,
	     const gchar  *field_name,
	     const GValue *value)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (field_name != NULL);
  g_return_if_fail (SFI_IS_VALUE (value));
  
  sfi_rec_set_copy (rec, field_name, value, FALSE);
}

GValue*
sfi_rec_get (SfiRec      *rec,
	     const gchar *field_name)
{
  gchar *name;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (field_name != NULL, NULL);

  if (!rec->sorted)
    sfi_rec_sort (rec);
  name = dupcanon (field_name);
  i = sfi_rec_lookup (rec, name);
  g_free (name);
  if (i < rec->n_fields)
    return rec->fields + i;
  return NULL;
}

SfiRec*
sfi_rec_copy_deep (SfiRec *rec)
{
  SfiRec *r;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);

  sfi_rec_sort (rec);
  r = sfi_rec_new ();
  for (i = 0; i < rec->n_fields; i++)
    sfi_rec_set_copy (r, rec->field_names[i], &rec->fields[i], TRUE);
  r->sorted = TRUE;
  return r;
}

gboolean
sfi_rec_check (SfiRec      *rec,
	       SfiRecFields rfields)
{
  guint i;

  g_return_val_if_fail (rec != NULL, FALSE);
  g_return_val_if_fail (rfields.n_fields > 0, FALSE);

  if (!rec->sorted)
    sfi_rec_sort (rec);
  for (i = 0; i < rfields.n_fields; i++)
    {
      guint n = sfi_rec_lookup (rec, rfields.fields[i]->name);
      GValue *value = n < rec->n_fields ? rec->fields + n : NULL;
      if (!value || !G_VALUE_HOLDS (value, G_PARAM_SPEC_VALUE_TYPE (rfields.fields[i])))
	return FALSE;
    }
  return TRUE;
}

static int
strpointercmp (const void *p1,
	       const void *p2)
{
  gchar *const *s1 = p1;
  gchar *const *s2 = p2;
  return strcmp (*s1, *s2);
}

void
sfi_rec_sort (SfiRec *rec)
{
  g_return_if_fail (rec != NULL);
  
  if (!rec->sorted && rec->n_fields > 1)
    {
      gchar **fnames = g_memdup (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      GValue *fields = g_new (GValue, rec->n_fields);
      guint i;
      /* sort field names */
      qsort (fnames, rec->n_fields, sizeof (fnames[0]), strpointercmp);
      /* sort fields */
      for (i = 0; i < rec->n_fields; i++)
	{
	  guint n = 0;
	  /* find corresponding field */
	  while (rec->field_names[n] != fnames[i])
	    n++;
	  /* relocate field */
	  memcpy (fields + i, rec->fields + n, sizeof (fields[0]));
	}
      g_free (rec->field_names);
      rec->field_names = fnames;
      g_free (rec->fields);
      rec->fields = fields;
    }
  rec->sorted = TRUE;
}

void
sfi_rec_swap_fields (SfiRec *rec,
		     SfiRec *swapper)
{
  guint n;
  GValue *fields;
  gchar **names;

  g_return_if_fail (rec != NULL);
  g_return_if_fail (swapper != NULL);
  
  sfi_rec_sort (rec);
  sfi_rec_sort (swapper);
  n = rec->n_fields;
  fields = rec->fields;
  names = rec->field_names;
  rec->n_fields = swapper->n_fields;
  rec->fields = swapper->fields;
  rec->field_names = swapper->field_names;
  swapper->n_fields = n;
  swapper->fields = fields;
  swapper->field_names = names;
}

gboolean
sfi_rec_validate (SfiRec      *rec,
		  SfiRecFields fields)
{
  GParamSpec *pspec;
  GValue *v;
  gboolean changed;

  g_return_val_if_fail (rec != NULL, FALSE);

  pspec = sfi_pspec_rec ("auto", NULL, NULL, fields, ":readwrite");
  v = sfi_value_rec (rec);
  changed = g_param_value_validate (pspec, v);
  sfi_value_free (v);
  g_param_spec_sink (pspec);
  return changed;
}

void
sfi_rec_set_bool (SfiRec      *rec,
		  const gchar *field_name,
		  SfiBool      v_bool)
{
  GValue *value = sfi_value_bool (v_bool);
  sfi_rec_set (rec, field_name, value);
  sfi_value_free (value);
}

void
sfi_rec_set_int	(SfiRec      *rec,
		 const gchar *field_name,
		 SfiInt	      v_int)
{
  GValue *value = sfi_value_int (v_int);
  sfi_rec_set (rec, field_name, value);
  sfi_value_free (value);
}

void
sfi_rec_set_num	(SfiRec      *rec,
		 const gchar *field_name,
		 SfiNum	      v_num)
{
  GValue *value = sfi_value_num (v_num);
  sfi_rec_set (rec, field_name, value);
  sfi_value_free (value);
}

void
sfi_rec_set_real (SfiRec          *rec,
		  const gchar     *field_name,
		  SfiReal	  v_real)
{
  GValue *value = sfi_value_real (v_real);
  sfi_rec_set (rec, field_name, value);
  sfi_value_free (value);
}

void
sfi_rec_set_string (SfiRec      *rec,
		    const gchar *field_name,
		    const gchar	*string)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_STRING);
  g_value_set_static_string (&value, string);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_choice (SfiRec      *rec,
		    const gchar *field_name,
		    const gchar	*choice)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_CHOICE);
  g_value_set_static_string (&value, choice);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_bblock (SfiRec      *rec,
		    const gchar *field_name,
		    SfiBBlock	*bblock)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_BBLOCK);
  g_value_set_static_boxed (&value, bblock);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_fblock (SfiRec      *rec,
		    const gchar *field_name,
		    SfiFBlock	*fblock)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_FBLOCK);
  g_value_set_static_boxed (&value, fblock);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_pspec (SfiRec      *rec,
		   const gchar *field_name,
		   GParamSpec  *pspec)
{
  GValue *value = sfi_value_pspec (pspec);
  sfi_rec_set (rec, field_name, value);
  sfi_value_free (value);
}

void
sfi_rec_set_seq	(SfiRec      *rec,
		 const gchar *field_name,
		 SfiSeq	     *seq)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_SEQ);
  g_value_set_static_boxed (&value, seq);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_rec (SfiRec      *rec,
		 const gchar *field_name,
		 SfiRec	     *v_rec)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_REC);
  g_value_set_static_boxed (&value, v_rec);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
sfi_rec_set_proxy (SfiRec      *rec,
		   const gchar *field_name,
		   SfiProxy     proxy)
{
  GValue value = { 0, };
  g_value_init (&value, SFI_TYPE_PROXY);
  sfi_value_set_proxy (&value, proxy);
  sfi_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

SfiBool
sfi_rec_get_bool (SfiRec      *rec,
		  const gchar *field_name)
{
  return value_as_num (sfi_rec_get (rec, field_name)) != 0;
}

SfiInt
sfi_rec_get_int (SfiRec      *rec,
		 const gchar *field_name)
{
  return value_as_num (sfi_rec_get (rec, field_name));
}

SfiNum
sfi_rec_get_num (SfiRec      *rec,
		 const gchar *field_name)
{
  return value_as_num (sfi_rec_get (rec, field_name));
}

SfiReal
sfi_rec_get_real (SfiRec      *rec,
		  const gchar *field_name)
{
  return value_as_real (sfi_rec_get (rec, field_name));
}

const gchar*
sfi_rec_get_string (SfiRec      *rec,
		    const gchar *field_name)
{
  return value_as_string (sfi_rec_get (rec, field_name));
}

const gchar*
sfi_rec_get_choice (SfiRec      *rec,
		    const gchar *field_name)
{
  return value_as_string (sfi_rec_get (rec, field_name));
}

SfiBBlock*
sfi_rec_get_bblock (SfiRec      *rec,
		    const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_BBLOCK (v))
    return sfi_value_get_bblock (v);
  return NULL;
}

SfiFBlock*
sfi_rec_get_fblock (SfiRec      *rec,
		    const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_FBLOCK (v))
    return sfi_value_get_fblock (v);
  return NULL;
}

GParamSpec*
sfi_rec_get_pspec (SfiRec      *rec,
		   const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_PSPEC (v))
    return sfi_value_get_pspec (v);
  return NULL;
}

SfiSeq*
sfi_rec_get_seq (SfiRec      *rec,
		 const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_SEQ (v))
    return sfi_value_get_seq (v);
  return NULL;
}

SfiRec*
sfi_rec_get_rec (SfiRec      *rec,
		 const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_REC (v))
    return sfi_value_get_rec (v);
  return NULL;
}

SfiProxy
sfi_rec_get_proxy (SfiRec      *rec,
		   const gchar *field_name)
{
  GValue *v = sfi_rec_get (rec, field_name);
  if (v && SFI_VALUE_HOLDS_PROXY (v))
    return sfi_value_get_proxy (v);
  return 0;
}


/* --- ring (circular-list) --- */
static inline SfiRing*
sfi_ring_prepend_link_i (SfiRing *head,
			 SfiRing *ring)
{
  if (!head)
    {
      ring->prev = ring;
      ring->next = ring;
    }
  else
    {
      ring->prev = head->prev;
      ring->next = head;
      head->prev->next = ring;
      head->prev = ring;
    }
  return ring;
}

static inline SfiRing*
sfi_ring_prepend_i (SfiRing *head,
		    gpointer data)
{
  SfiRing *ring = sfi_new_struct (SfiRing, 1);
  
  ring->data = data;
  return sfi_ring_prepend_link_i (head, ring);
}

static inline SfiRing*
sfi_ring_append_link_i (SfiRing *head,
			SfiRing *ring)
{
  sfi_ring_prepend_link_i (head, ring);
  return head ? head : ring;
}

SfiRing*
sfi_ring_prepend (SfiRing  *head,
		  gpointer data)
{
  return sfi_ring_prepend_i (head, data);
}

SfiRing*
sfi_ring_prepend_uniq (SfiRing  *head,
		       gpointer data)
{
  SfiRing *walk;
  
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    if (walk->data == data)
      return head;
  return sfi_ring_prepend_i (head, data);
}

SfiRing*
sfi_ring_append (SfiRing  *head,
		 gpointer data)
{
  SfiRing *ring;
  
  ring = sfi_ring_prepend_i (head, data);
  
  return head ? head : ring;
}

SfiRing*
sfi_ring_copy (SfiRing *head)
{
  SfiRing *walk, *dest = NULL;
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    dest = sfi_ring_append (dest, walk->data);
  return dest;
}

SfiRing*
sfi_ring_copy_deep (SfiRing        *head,
		    SfiRingDataFunc copy,
		    gpointer        func_data)
{
  SfiRing *walk, *dest = NULL;
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    dest = sfi_ring_append (dest, copy (walk->data, func_data));
  return dest;
}

SfiRing*
sfi_ring_concat (SfiRing *head1,
		 SfiRing *head2)
{
  SfiRing *tail1, *tail2;
  
  if (!head1)
    return head2;
  if (!head2)
    return head1;
  tail1 = head1->prev;
  tail2 = head2->prev;
  head1->prev = tail2;
  tail2->next = head1;
  head2->prev = tail1;
  tail1->next = head2;
  
  return head1;
}

/**
 * sfi_ring_split
 * @head1:   a non-empty ring
 * @head2:   a ring node different from @head1 contained in @head1
 * @returns: @head2 for convenience
 * Split a ring into two parts, starting the second ring with @head2.
 * @head2 must therefore be non-NULL and must be contained in the ring
 * formed by @head1.
 */
SfiRing*
sfi_ring_split (SfiRing *head1,
		SfiRing *head2)
{
  SfiRing *tail1, *tail2;

  g_return_val_if_fail (head1 != NULL, NULL);
  g_return_val_if_fail (head2 != NULL, NULL);
  g_return_val_if_fail (head1 != head2, NULL);

  tail1 = head2->prev;
  tail2 = head1->prev;
  head2->prev = tail2;
  tail2->next = head2;
  head1->prev = tail1;
  tail1->next = head1;
  return head2;
}

SfiRing*
sfi_ring_remove_node (SfiRing *head,
		      SfiRing *node)
{
  if (!head)
    g_return_val_if_fail (head == NULL && node == NULL, NULL);
  if (!head || !node)
    return NULL;
  
  /* special case one item ring */
  if (head->prev == head)
    {
      g_return_val_if_fail (node == head, head);
      
      sfi_delete_struct (SfiRing, node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  sfi_delete_struct (SfiRing, node);
  
  return head;
}

SfiRing*
sfi_ring_merge_sorted (SfiRing     *head1,
		       SfiRing     *head2,
		       GCompareFunc func)
{
  if (head1 && head2)
    {
      SfiRing *tail1 = head1->prev;
      SfiRing *tail2 = head2->prev;
      SfiRing *tmp, *ring = NULL;
      /* NULL terminate rings */
      tail1->next = NULL;
      tail2->next = NULL;
      while (head1 && head2)
	{
	  gint cmp = func (head1->data, head2->data);
	  if (cmp <= 0)
	    {
	      tmp = head1;
	      head1 = head1->next;
	    }
	  else
	    {
	      tmp = head2;
	      head2 = head2->next;
	    }
	  ring = sfi_ring_append_link_i (ring, tmp);
	}
      /* reform valid rings, concat sorted rest */
      if (head1)
	{
	  tail1->next = head1;
	  head1->prev = tail1;
	  return sfi_ring_concat (ring, head1);
	}
      if (head2)
	{
	  tail2->next = head2;
	  head2->prev = tail2;
	  return sfi_ring_concat (ring, head2);
	}
      return ring;
    }
  else
    return sfi_ring_concat (head1, head2);
}

SfiRing*
sfi_ring_sort (SfiRing     *head,
	       GCompareFunc func)
{
  g_return_val_if_fail (func != NULL, head);

  if (head && head->next != head)
    {
      SfiRing *ring, *tmp, *tail = head->prev;
      /* find middle node to get log2 recursion depth */
      ring = tmp = head->next;
      while (tmp != tail && tmp->next != tail)
	{
	  ring = ring->next;
	  tmp = tmp->next->next;
	}
      sfi_ring_split (head, ring);
      return sfi_ring_merge_sorted (sfi_ring_sort (head, func),
				    sfi_ring_sort (ring, func),
				    func);
    }
  return head;
}

SfiRing*
sfi_ring_reverse (SfiRing *head)
{
  if (head)
    {
      SfiRing *ring = head = head->prev;
      do
	{
	  SfiRing *tmp = ring;
	  ring = tmp->next;
	  tmp->next = tmp->prev;
	  tmp->prev = ring;
	}
      while (ring != head);
    }
  return head;
}

SfiRing*
sfi_ring_remove (SfiRing *head,
		 gpointer data)
{
  SfiRing *walk;

  if (!head)
    return NULL;
  
  /* make tail data removal an O(1) operation */
  if (head->prev->data == data)
    return sfi_ring_remove_node (head, head->prev);
  
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    if (walk->data == data)
      return sfi_ring_remove_node (head, walk);
  
  g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head);
  
  return head;
}

guint
sfi_ring_length (SfiRing *head)
{
  SfiRing *ring;
  guint i = 0;
  
  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    i++;

  return i;
}

SfiRing*
sfi_ring_find (SfiRing      *head,
	       gconstpointer data)
{
  SfiRing *ring;

  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    if (ring->data == (gpointer) data)
      return ring;

  return NULL;
}

SfiRing*
sfi_ring_nth (SfiRing *head,
	      guint    n)
{
  SfiRing *ring = head;

  while (n-- && ring)
    ring = sfi_ring_walk (ring, head);

  return ring;
}

gpointer
sfi_ring_nth_data (SfiRing *head,
		   guint    n)
{
  SfiRing *ring = head;

  while (n-- && ring)
    ring = sfi_ring_walk (ring, head);

  return ring ? ring->data : ring;
}

void
sfi_ring_free_deep (SfiRing        *head,
		    SfiRingDataFunc free_func,
		    gpointer        func_data)
{
  gpointer data = sfi_ring_pop_head (&head);
  while (data)
    {
      free_func (data, func_data);
      data = sfi_ring_pop_head (&head);
    }
}

void
sfi_ring_free (SfiRing *head)
{
  if (head)
    {
      head->prev->next = NULL;
      _sfi_free_node_list (head, sizeof (*head));
    }
}

gpointer
sfi_ring_pop_head (SfiRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->data;
  *head_p = sfi_ring_remove_node (*head_p, *head_p);
  
  return data;
}

gpointer
sfi_ring_pop_tail (SfiRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->prev->data;
  *head_p = sfi_ring_remove_node (*head_p, (*head_p)->prev);
  
  return data;
}

SfiRing*
sfi_ring_insert_sorted (SfiRing	    *head,
			gpointer     data,
			GCompareFunc func)
{
  gint cmp;

  g_return_val_if_fail (func != NULL, head);

  if (!head)
    return sfi_ring_prepend (head, data);

  /* typedef gint (*GCompareFunc) (gconstpointer a,
   *                               gconstpointer b);
   */
  cmp = func (data, head->data);

  if (cmp >= 0)	/* insert after head */
    {
      SfiRing *tmp, *tail = head->prev;
      
      /* make appending an O(1) operation */
      if (head == tail || func (data, tail->data) >= 0)
	return sfi_ring_append (head, data);

      /* walk forward while data >= tmp (skipping equal nodes) */
      for (tmp = head->next; tmp != tail; tmp = tmp->next)
	if (func (data, tmp->data) < 0)
	  break;

      /* insert before sibling which is greater than data */
      sfi_ring_prepend (tmp, data);	/* keep current head */
      return head;
    }
  else /* cmp < 0 */
    return sfi_ring_prepend (head, data);
}
