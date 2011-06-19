/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#ifndef __RAPICORN_SVG_TWEAK_H__
#define __RAPICORN_SVG_TWEAK_H__
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int             svg_tweak_point_tweak   (double vx, double vy, double *px, double *py,
                                         const double affine[6], const double iaffine[6]);
int             svg_tweak_point_simple  (double *px, double *py, const double affine[6], const double iaffine[6]);
extern int      svg_tweak_debugging;

#ifdef __cplusplus
};
#endif /* __cplusplus */
#endif /* __RAPICORN_SVG_TWEAK_H__ */
