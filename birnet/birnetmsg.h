/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_LOG_H__
#define __SFI_LOG_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS

/* --- standard logging --- */
static inline void sfi_error            (const char     *format,
                                         ...) G_GNUC_PRINTF (1, 2);
static inline void sfi_warning          (const char     *format,
                                         ...) G_GNUC_PRINTF (1, 2);
static inline void sfi_info             (const char     *format,
                                         ...) G_GNUC_PRINTF (1, 2);
static inline void sfi_diag             (const char     *format,
                                         ...) G_GNUC_PRINTF (1, 2);

/* --- debugging --- */
static inline void sfi_debug            (const char     *key,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
gboolean           sfi_debug_check      (const char     *key);
void               sfi_debug_allow      (const char     *key_list);
void               sfi_debug_deny       (const char     *key_list);
#define            sfi_nodebug(key, ...) do { /* nothing */ } while (0)

/* --- user interface messages --- */
static inline void sfi_error_msg        (const char     *config_blurb,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
static inline void sfi_warning_msg      (const char     *config_blurb,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
static inline void sfi_info_msg         (const char     *config_blurb,
                                         const char     *format,
                                         ...) G_GNUC_PRINTF (2, 3);

/* --- logging configuration --- */
typedef enum /*< skip >*/
{
  SFI_LOG_TO_STDERR     = 1,
  SFI_LOG_TO_STDLOG     = 2,
  SFI_LOG_TO_HANDLER    = 4,
} SfiLogFlags;
void    sfi_log_assign_level            (unsigned char   level,
                                         SfiLogFlags     channel_mask);
void    sfi_log_set_stdlog              (gboolean        stdlog_to_stderr,
                                         const char     *stdlog_filename,
                                         guint           syslog_priority); /* if != 0, stdlog to syslog */
typedef struct SfiLogMessage             SfiLogMessage;
typedef void (*SfiLogHandler)           (SfiLogMessage  *message);
void    sfi_log_set_thread_handler      (SfiLogHandler   handler);

/* --- logging internals --- */
#define	SFI_LOG_ERROR   ('E')
#define	SFI_LOG_WARNING	('W')
#define	SFI_LOG_INFO	('I')
#define	SFI_LOG_DIAG	('A')
#define	SFI_LOG_DEBUG	('D')
struct SfiLogMessage {
  const gchar  *log_domain;
  unsigned char level;
  const char   *key;            /* maybe generated */
  const char   *config_blurb;   /* translated */
  const char   *message;
};

void    sfi_log_valist                  (const char     *log_domain,
                                         unsigned char   level,
                                         const char     *key,
                                         const char     *config_blurb,
                                         const char     *format,
                                         va_list         args);
void    sfi_log_string                  (const char     *log_domain,
                                         unsigned char   level,
                                         const char     *key,
                                         const char     *config_blurb,
                                         const char     *string);
void    sfi_log_default_handler         (SfiLogMessage  *message);
void    _sfi_init_logging               (void);
#ifndef SFI_LOG_DOMAIN
#define SFI_LOG_DOMAIN  G_LOG_DOMAIN
#endif

/* --- implementations --- */
static inline void
sfi_error (const char *format,
           ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_ERROR, NULL, NULL, format, args);
  va_end (args);
}
static inline void
sfi_warning (const char *format,
             ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_WARNING, NULL, NULL, format, args);
  va_end (args);
}
static inline void
sfi_info (const char *format,
          ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_INFO, NULL, NULL, format, args);
  va_end (args);
}
static inline void
sfi_diag (const char *format,
          ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_DIAG, NULL, NULL, format, args);
  va_end (args);
}
static inline void
sfi_debug (const char *key,
           const char *format,
          ...)
{
  if (sfi_debug_check (key))
    {
      va_list args;
      va_start (args, format);
      sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_DEBUG, key, NULL, format, args);
      va_end (args);
    }
}
static inline void
sfi_error_msg (const char *config_blurb,
               const char *format,
               ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_ERROR, NULL, config_blurb, format, args);
  va_end (args);
}
static inline void
sfi_warning_msg (const char *config_blurb,
                 const char *format,
                 ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_WARNING, NULL, config_blurb, format, args);
  va_end (args);
}
static inline void
sfi_info_msg (const char *config_blurb,
              const char *format,
              ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (SFI_LOG_DOMAIN, SFI_LOG_INFO, NULL, config_blurb, format, args);
  va_end (args);
}

G_END_DECLS

#endif /* __SFI_LOG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
