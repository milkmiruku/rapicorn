/* Rapicorn region handling
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_REGION_IMPL_H__
#define __RAPICORN_REGION_IMPL_H__

#include <stdbool.h>

#ifdef  __cplusplus
#define RAPICORN_EXTERN_C_BEGIN() extern "C" {
#define RAPICORN_EXTERN_C_END()              }
#else
#define RAPICORN_EXTERN_C_BEGIN()
#define RAPICORN_EXTERN_C_END()
#endif

RAPICORN_EXTERN_C_BEGIN();

/* --- compat stuff --- */
typedef signed int      RapicornRegionInt64 __attribute__ ((__mode__ (__DI__)));
#ifndef FALSE
#define FALSE           false
#endif
#ifndef TRUE
#define TRUE            true
#endif

/* --- types & macros --- */
typedef enum {
  RAPICORN_REGION_OUTSIDE = 0,
  RAPICORN_REGION_INSIDE  = 1,
  RAPICORN_REGION_PARTIAL = 2
} RapicornRegionCType;
typedef struct _RapicornRegion    RapicornRegion;
typedef struct {
  RapicornRegionInt64 x1, y1, x2, y2;
} RapicornRegionBox;
typedef struct {
  RapicornRegionInt64 x, y;
} RapicornRegionPoint;

/* --- functions --- */
RapicornRegion*		_rapicorn_region_create 	(void);
void			_rapicorn_region_free 		(RapicornRegion 	   *region);
void			_rapicorn_region_init		(RapicornRegion            *region,
							 int                        region_size);
void			_rapicorn_region_uninit		(RapicornRegion            *region);
void           		_rapicorn_region_copy 		(RapicornRegion		   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_clear 		(RapicornRegion 	   *region);
bool			_rapicorn_region_empty 		(const RapicornRegion 	   *region);
bool			_rapicorn_region_equal 		(const RapicornRegion 	   *region,
							 const RapicornRegion 	   *region2);
int			_rapicorn_region_cmp 		(const RapicornRegion 	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_swap 		(RapicornRegion 	   *region,
							 RapicornRegion 	   *region2);
void			_rapicorn_region_extents 	(const RapicornRegion 	   *region,
							 RapicornRegionBox    	   *rect);
bool			_rapicorn_region_point_in 	(const RapicornRegion 	   *region,
							 const RapicornRegionPoint *point);
RapicornRegionCType	_rapicorn_region_rect_in 	(const RapicornRegion      *region,
							 const RapicornRegionBox   *rect);
RapicornRegionCType	_rapicorn_region_region_in 	(const RapicornRegion      *region,
							 const RapicornRegion      *region2);
int			_rapicorn_region_get_rects 	(const RapicornRegion 	   *region,
							 int                  	    n_rects,
							 RapicornRegionBox    	   *rects);
void			_rapicorn_region_union_rect	(RapicornRegion            *region,
							 const RapicornRegionBox   *rect);
void			_rapicorn_region_union 		(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_subtract 	(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_intersect 	(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_xor 		(RapicornRegion       	   *region,
							 const RapicornRegion 	   *region2);
void			_rapicorn_region_debug 		(const char 		   *format,
							 ...) __attribute__ ((__format__ (__printf__, 1, 2)));

RAPICORN_EXTERN_C_END();
#endif /* __RAPICORN_REGION_IMPL_H__ */
