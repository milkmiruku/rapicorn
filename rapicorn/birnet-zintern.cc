/* birnet-zintern - small C source compression utility
 * Copyright (C) 2003-2006 Tim Janik
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
#include <rapicorn/birnet.hh>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>

namespace Birnet {

static void     zintern_error  (const char     *format,
                                ...) BIRNET_PRINTF (1, 2);

static void
zintern_error  (const char     *format,
                ...)
{
  gchar *buffer;
  va_list args;
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  g_printerr ("\nERROR: %s", buffer);
  _exit (1);
  g_free (buffer);
}

static bool use_compression = FALSE;
static bool use_base_name = FALSE;

typedef struct {
  uint pos;
  bool pad;
} Config;
static Config config_init = { 0, 0 };

static inline void
print_uchar (Config *config,
	     uint8 d)
{
  if (config->pos > 70)
    {
      printf ("\"\n  \"");
      config->pos = 3;
      config->pad = FALSE;
    }
  if (d < 33 || d > 126 || d == '?')
    {
      printf ("\\%o", d);
      config->pos += 1 + 1 + (d > 7) + (d > 63);
      config->pad = d < 64;
      return;
    }
  if (d == '\\')
    {
      printf ("\\\\");
      config->pos += 2;
    }
  else if (d == '"')
    {
      printf ("\\\"");
      config->pos += 2;
    }
  else if (config->pad && d >= '0' && d <= '9')
    {
      printf ("\"\"");
      printf ("%c", d);
      config->pos += 3;
    }
  else
    {
      printf ("%c", d);
      config->pos += 1;
    }
  config->pad = FALSE;
  return;
}

#define to_upper(c)     ((c) >='a' && (c) <='z' ? (c) - 'a' + 'A' : (c))
#define is_alnum(c)     (((c) >='A' && (c) <='Z') || ((c) >='a' && (c) <='z') || ((c) >='0' && (c) <='9'))
static String
to_cupper (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    if (is_alnum (s[i]))
      s[i] = to_upper (s[i]);
    else
      s[i] = '_';
  return s;
}

static void
gen_zfile (const char *name,
	   const char *file)
{
  FILE *f = fopen (file, "r");
  uint8 *data = NULL;
  uint i, dlen = 0, mlen = 0;
  Bytef *cdata;
  uLongf clen;
  gchar *basefile = g_path_get_basename (file);
  String fname = use_base_name ? basefile : file;
  g_free (basefile); basefile = NULL;
  Config config;
  if (!f)
    zintern_error ("failed to open \"%s\": %s", file, g_strerror (errno));
  do
    {
      if (mlen <= dlen + 1024)
	{
	  mlen += 8192;
	  data = g_renew (uint8, data, mlen);
	}
      dlen += fread (data + dlen, 1, mlen - dlen, f);
    }
  while (!feof (f));

  if (ferror (f))
    zintern_error ("failed to read from \"%s\": %s", file, g_strerror (errno));

  if (use_compression)
    {
      int result;
      const char *err;
      clen = dlen + dlen / 100 + 64;
      cdata = g_new (uint8, clen);
      result = compress2 (cdata, &clen, data, dlen, Z_BEST_COMPRESSION);
      switch (result)
	{
	case Z_OK:
	  err = NULL;
	  break;
	case Z_MEM_ERROR:
	  err = "out of memory";
	  break;
	case Z_BUF_ERROR:
	  err = "insufficient buffer size";
	  break;
	default:
	  err = "unknown error";
	  break;
	}
      if (err)
	zintern_error ("while compressing \"%s\": %s", file, err);
    }
  else
    {
      clen = dlen;
      cdata = data;
    }

  g_print ("/* birnet-zintern file dump of %s */\n", file);

  config = config_init;
  printf ("#define %s_NAME \"", to_cupper (name).c_str());
  for (i = 0; i < fname.size(); i++)
    print_uchar (&config, fname[i]);
  printf ("\"\n");

  printf ("#define %s_SIZE (%u)\n", to_cupper (name).c_str(), dlen);

  config = config_init;
  printf ("static const unsigned char %s_DATA[%lu + 1] =\n", to_cupper (name).c_str(), clen);
  printf ("( \"");
  for (i = 0; i < clen; i++)
    print_uchar (&config, cdata[i]);
  printf ("\");\n");

  fclose (f);
  g_free (data);
  if (cdata != data)
    g_free (cdata);
}

static int
help (char *arg)
{
  g_printerr ("usage: birnet-zintern [-h] [-b] [-z] [[name file]...]\n");
  g_printerr ("  -h  Print usage information\n");
  g_printerr ("  -b  Strip directories from file names\n");
  g_printerr ("  -z  Compress data blocks with libz\n");
  g_printerr ("Parse (name, file) pairs and generate C source\n");
  g_printerr ("containing inlined data blocks of the files given.\n");
  return arg != NULL;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  GSList *plist = NULL;

  InitValue ivalues[] = {
    { "stand-alone", "true" },
    { NULL }
  };
  // birnet_init (&argc, &argv, NULL, ivalues);

  for (int i = 1; i < argc; i++)
    {
      if (strcmp ("-z", argv[i]) == 0)
	{
	  use_compression = TRUE;
	}
      else if (strcmp ("-b", argv[i]) == 0)
	{
	  use_base_name = TRUE;
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	plist = g_slist_append (plist, argv[i]);
    }
  
  if (argc <= 1)
    return help (NULL);

  while (plist && plist->next)
    {
      const char *name = (char*) plist->data;
      GSList *tmp = plist;
      plist = tmp->next;
      g_slist_free_1 (tmp);
      const char *file = (char*) plist->data;
      tmp = plist;
      plist = tmp->next;
      g_slist_free_1 (tmp);
      gen_zfile (name, file);
    }

  return 0;
}

} // Birnet
