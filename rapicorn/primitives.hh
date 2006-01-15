/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_PRIMITIVES_HH__
#define __RAPICORN_PRIMITIVES_HH__

#include <list>
#include <math.h>
#include <values.h> /* MAXDOUBLE, etc. */
#include <rapicorn/enumdefs.hh>

namespace Rapicorn {

/* --- degree & radians --- */
#undef PI               /* some math packages define PI? */
static const double PI        = 3.141592653589793238462643383279502884197;
static const double LN_INV255 = -5.5412635451584261462455391880218; // ln(1/255)

inline double BIRNET_CONST degree  (double radians) { return radians * (180. / PI); }
inline double BIRNET_CONST radians (double degree)  { return degree * (PI / 180.); }

/* double to integer */
inline int BIRNET_CONST
ftoi (double d)
{
  /* this relies on the hardware default round-to-nearest */
#if defined __i386__ && defined __GNUC__
  int r;
  __asm__ ("fistl %0"
           : "=m" (r)
           : "t" (d));
  return r;
#endif
  return (int) d;
}
inline int BIRNET_CONST iround (double d) { return ftoi (round (d)); }
inline int BIRNET_CONST iceil  (double d) { return ftoi (ceil (d)); }
inline int BIRNET_CONST ifloor (double d) { return ftoi (floor (d)); }

/* --- enums --- */
typedef enum {
  COMBINE_NORMAL,       /* A OVER B */
  COMBINE_OVER,         /* A + (B & ~A) */
  COMBINE_UNDER,        /* B + (A & ~B) */
  COMBINE_ADD,          /* A + B */
  COMBINE_DEL,          /* B - alphaA */
  COMBINE_ATOP,         /* (A + B) & B */
  COMBINE_XOR,          /* A ^ B */
  COMBINE_BLEND,        /* B * (1 - alpha) + A * alpha */
  COMBINE_VALUE,        /* B.value = A.value */
} CombineType;

/* --- Point --- */
class Point {
public:
  double x, y;
  Point (double ax,
         double ay) :
    x (ax),
    y (ay)
  {}
  explicit Point () :
    x (0),
    y (0)
  {}
  inline double
  dist2 (double px, double py) const
  {
    double dx = px - x;
    double dy = py - y;
    return dx * dx + dy * dy;
  }
  inline double
  dist (double px, double py) const
  {
    double dx = px - x;
    double dy = py - y;
    return sqrt (dx * dx + dy * dy);
  }
  inline double dist2 (const Point &p = Point (0, 0)) const { return dist2 (p.x, p.y); }
  inline double dist  (const Point &p = Point (0, 0)) const { return dist (p.x, p.y); }
  Point  operator+  (const Point &p) const { return Point (x + p.x, y + p.y); }
  Point  operator+  (double delta)   const { return Point (x + delta, y + delta); }
  Point  operator-  (const Point &p) const { return Point (x - p.x, y - p.y); }
  Point  operator-  (double delta)   const { return Point (x - delta, y - delta); }
  Point& operator-  ()                     { x = -x; y = -y; return *this; }
  Point& operator+= (const Point &p)       { return *this = *this + p; }
  Point& operator+= (double delta)         { return *this = *this + delta; }
  Point& operator-= (const Point &p)       { return *this = *this - p; }
  Point& operator-= (double delta)         { return *this = *this - delta; }
  bool   operator== (const Point &p2)      { return x == p2.x && y == p2.y; }
  bool   operator!= (const Point &p2)      { return x != p2.x || y != p2.y; }
};
inline Point
min (const Point &p1, const Point &p2)
{
  return Point (Birnet::min (p1.x, p2.x), Birnet::min (p1.y, p2.y));
}
inline Point
max (const Point &p1, const Point &p2)
{
  return Point (Birnet::max (p1.x, p2.x), Birnet::max (p1.y, p2.y));
}
inline Point
floor (const Point &s)
{
  using ::floor;
  return Point (floor (s.x), floor (s.y));
}
inline Point
ceil (const Point &s)
{
  using ::ceil;
  return Point (ceil (s.x), ceil (s.y));
}
inline Point
round (const Point &s)
{
  using ::round;
  return Point (round (s.x), ::round (s.y));
}

/* --- Rect --- */
class Rect {
public:
  Point ll; /* lower-left */
  Point ur; /* upper-right */
  explicit Rect () :
    ll (0, 0), ur (-1, -1)
  {}
  Rect (Point cp0, Point cp1) :
    ll (min (cp0, cp1)), ur (max (cp0, cp1))
  {}
  Rect (Point p0, double width, double height) :
    ll (min (p0, Point (p0.x + width, p0.y + height))),
    ur (max (p0, Point (p0.x + width, p0.y + height)))
  {}
  Point upper_left      () const { return Point (ll.x, ur.y); }
  Point upper_right     () const { return ur; }
  Point lower_right     () const { return Point (ur.x, ll.y); }
  Point lower_left      () const { return ll; }
  Point center          () const { return Point ((ll.x + ur.x) / 2, (ll.y + ur.y) / 2); }
  Point north           () const { return Point ((ll.x + ur.x) / 2, ur.y); }
  Point north_east      () const { return Point (ur.x, ur.y); }
  Point east            () const { return Point (ur.x, (ll.y + ur.y) / 2); }
  Point south_east      () const { return Point (ur.x, ll.y); }
  Point south           () const { return Point ((ll.x + ur.x) / 2, ll.y); }
  Point south_west      () const { return Point (ll.x, ll.y); }
  Point west            () const { return Point (ll.x, (ll.y + ur.y) / 2); }
  Point north_west      () const { return Point (ll.x, ur.y); }
  Rect&
  rect_union (const Rect &r)
  {
    if (r.is_empty())
      return *this;
    if (is_empty())
      return *this = r;
    ll = min (ll, r.ll);
    ur = max (ur, r.ur);
    return *this;
  }
  Rect&
  rect_union (const Point &p)
  {
    if (is_empty())
      {
        ll = p;
        ur = p;
      }
    else
      {
        ll = min (ll, p);
        ur = max (ur, p);
      }
    return *this;
  }
  Rect&
  add_border (double b)
  {
    ll.x -= b;
    ll.y -= b;
    ur.x += b;
    ur.y += b;
    if (is_empty())
      *this = Rect();
    return *this;
  }
  Rect&
  intersect (const Rect &r)
  {
    ll = max (ll, r.ll);
    ur = min (ur, r.ur);
    return *this;
  }
  bool
  is_empty () const
  {
    return ll.x > ur.x || ll.y > ur.y;
  }
  double
  width() const
  {
    return ll.x >= ur.x ? 0 : ur.x - ll.x;
  }
  double
  height() const
  {
    return ll.y >= ur.y ? 0 : ur.y - ll.y;
  }
  Point
  anchor_point (AnchorType anchor)
  {
    switch (anchor)
      {
      default:
      case ANCHOR_CENTER:       return center();        break;
      case ANCHOR_NORTH:        return north();         break;
      case ANCHOR_NORTH_EAST:   return north_east();    break;
      case ANCHOR_EAST:         return east();          break;
      case ANCHOR_SOUTH_EAST:   return south_east();    break;
      case ANCHOR_SOUTH:        return south();         break;
      case ANCHOR_SOUTH_WEST:   return south_west();    break;
      case ANCHOR_WEST:         return west();          break;
      case ANCHOR_NORTH_WEST:   return north_west();    break;
      }
  }
  Rect&
  operator+ (const Point &p)
  {
    ll += p;
    ur += p;
    return *this;
  }
  Rect&
  operator- (const Point &p)
  {
    ll -= p;
    ur -= p;
    return *this;
  }
  String
  string()
  {
    char buffer[128];
    sprintf (buffer, "((%f,%f),(%f,%f))", ll.x, ll.y, ur.x, ur.y);
    return String (buffer);
  }
  static Rect
  create_anchored (AnchorType anchor,
                   double     width,
                   double     height)
  {
    Rect b (Point (0, 0), abs (width), abs (height)); /* SOUTH_WEST */
    Point delta = b.anchor_point (anchor);
    b.ll -= delta;
    b.ur -= delta;
    return b;
  }
};

/* --- Color --- */
class Color {
  uint32              argb_pixel;
public:
  static inline uint8 IMUL    (uint8 v, uint8 alpha)            { return (v * (uint32) 0x010102 * alpha) >> 24; }
  static inline uint8 IDIV    (uint8 v, uint8 alpha)            { return (v * (uint32) 0xff) / alpha; }
  static inline uint8 IDIV0   (uint8 v, uint8 alpha)            { if (!alpha) return 0xff; return IDIV (v, alpha); }
  static inline uint8 IMULDIV (uint8 v, uint8 amul, uint8 adiv) // IMUL (IDIV (v, amul), adiv)
  { return (((v * (uint32) 0x010102 * amul / adiv) >> 8) * 0xff) >> 16; }
  Color (uint32 c = 0) :
    argb_pixel (c)
  {}
  Color (uint8 red, uint8 green, uint8 blue, uint8 alpha = 0xff) :
    argb_pixel ((alpha << 24) | (red << 16) | (green << 8) | blue)
  {}
  static Color from_premultiplied (uint32 pargb)
  {
    Color c = (pargb);
    if (c.alpha())
      {
        c.red (IDIV (c.red(), c.alpha()));
        c.green (IDIV (c.green(), c.alpha()));
        c.blue (IDIV (c.blue(), c.alpha()));
      }
    else
      {
        c.red (0);
        c.green (0);
        c.blue (0);
      }
    return c;
  }
  uint32        premultiplied() const
  {
    /* extract alpha, red, green ,blue */
    uint32 a = alpha(), r = red(), g = green(), b = blue();
    /* bring into premultiplied form */
    r = IMUL (r, a);
    g = IMUL (g, a);
    b = IMUL (b, a);
    return Color (r, g, b, a);
  }
  void
  set (uint8 red, uint8 green, uint8 blue)
  {
    argb_pixel = (alpha() << 24) | (red << 16) | (green << 8) | blue;
  }
  void
  set (uint8 red, uint8 green, uint8 blue, uint8 alpha)
  {
    argb_pixel = (alpha << 24) | (red << 16) | (green << 8) | blue;
  }
  uint32        rgb    () const { return argb_pixel & 0x00ffffff; }
  uint32        argb   () const { return argb_pixel; }
  uint          alpha  () const { return argb_pixel >> 24; }
  uint          red    () const { return (argb_pixel >> 16) & 0xff; }
  uint          green  () const { return (argb_pixel >> 8) & 0xff; }
  uint          blue   () const { return argb_pixel & 0xff; }
  operator      uint32 () const { return argb(); }
  Color&        alpha  (uint8 v) { argb_pixel &= 0x00ffffff; argb_pixel |= v << 24; return *this; }
  Color&        red    (uint8 v) { argb_pixel &= 0xff00ffff; argb_pixel |= v << 16; return *this; }
  Color&        green  (uint8 v) { argb_pixel &= 0xffff00ff; argb_pixel |= v << 8; return *this; }
  Color&        blue   (uint8 v) { argb_pixel &= 0xffffff00; argb_pixel |= v << 0; return *this; }
  double        get_hsv_value () { return max (max (red(), green()), blue()) / 255.; }
  void          get_hsv (double *huep,          /* 0..360: 0=red, 120=green, 240=blue */
                         double *saturationp,   /* 0..1 */
                         double *valuep);       /* 0..1 */
  void          set_hsv (double hue,            /* 0..360: 0=red, 120=green, 240=blue */
                         double saturation,     /* 0..1 */
                         double value);         /* 0..1 */
  void
  set_hsv_value (double v)
  {
    double frac = v / get_hsv_value();
    double r = red() * frac, g = green() * frac, b = blue() * frac;
    set (iround (r), iround (g), iround (b));
  }
  uint8         channel (uint nth) const
  {
    switch (nth)
      {
      case 0: return alpha();
      case 1: return red();
      case 2: return green();
      case 3: return blue();
      }
    return 0;
  }
  void          channel (uint nth, uint8 v)
  {
    switch (nth)
      {
      case 0: alpha (v); break;
      case 1: red (v);   break;
      case 2: green (v); break;
      case 3: blue (v);  break;
      }
  }
  Color&        shade  (uint8 lucent)
  {
    uint32 a = argb_pixel >> 24;
    alpha ((a * lucent + a + lucent) >> 8);
    return *this;
  }
  Color&        combine (const Color c2, uint8 lucent)
  {
    /* extract alpha, red, green ,blue */
    uint32 Aa = c2.alpha(), Ar = c2.red(), Ag = c2.green(), Ab = c2.blue();
    uint32 Ba = alpha(), Br = red(), Bg = green(), Bb = blue();
    /* weight A layer */
    Aa = IMUL (Aa, lucent);
    /* bring into premultiplied form */
    Ar = IMUL (Ar, Aa);
    Ag = IMUL (Ag, Aa);
    Ab = IMUL (Ab, Aa);
    Br = IMUL (Br, Ba);
    Bg = IMUL (Bg, Ba);
    Bb = IMUL (Bb, Ba);
    /* A over B = colorA + colorB * (1 - alphaA) */
    uint32 Ai = 255 - Aa;
    uint8 Dr = Ar + IMUL (Br, Ai);
    uint8 Dg = Ag + IMUL (Bg, Ai);
    uint8 Db = Ab + IMUL (Bb, Ai);
    uint8 Da = Aa + IMUL (Ba, Ai);
    /* un-premultiply */
    if (Da)
      {
        Dr = IDIV (Dr, Da);
        Dg = IDIV (Dg, Da);
        Db = IDIV (Db, Da);
      }
    else
      Dr = Dg = Db = 0;
    /* assign */
    alpha (Da);
    red (Dr);
    green (Dg);
    blue (Db);
    return *this;
  }
  Color&        blend_premultiplied (const Color c2, uint8 lucent)
  {
    /* extract alpha, red, green ,blue */
    uint32 Aa = c2.alpha(), Ar = c2.red(), Ag = c2.green(), Ab = c2.blue();
    uint32 Ba = alpha(), Br = red(), Bg = green(), Bb = blue();
    /* A over B = colorA * alpha + colorB * (1 - alpha) */
    uint32 ilucent = 255 - lucent;
    uint8 Dr = IMUL (Ar, lucent) + IMUL (Br, ilucent);
    uint8 Dg = IMUL (Ag, lucent) + IMUL (Bg, ilucent);
    uint8 Db = IMUL (Ab, lucent) + IMUL (Bb, ilucent);
    uint8 Da = IMUL (Aa, lucent) + IMUL (Ba, ilucent);
    /* assign */
    alpha (Da);
    red (Dr);
    green (Dg);
    blue (Db);
    return *this;
  }
  Color&        blend (const Color c2, uint8 lucent)
  {
    Color pre1 = premultiplied(), pre2 = c2.premultiplied();
    /* A over B = colorA * alpha + colorB * (1 - alpha) */
    pre1.blend_premultiplied (pre2, lucent);
    /* un-premultiply */
    uint32 Da = pre1.alpha(), Dr = pre1.red(), Dg = pre1.green(), Db = pre1.blue();
    if (Da)
      {
        Dr = IDIV (Dr, Da);
        Dg = IDIV (Dg, Da);
        Db = IDIV (Db, Da);
      }
    else
      Dr = Dg = Db = 0;
    /* assign */
    alpha (Da);
    red (Dr);
    green (Dg);
    blue (Db);
    return *this;
  }
  String
  string()
  {
    char buffer[128];
    sprintf (buffer, "{.r=%u,.g=%u,.b=%u,.a=%u}", red(), green(), blue(), alpha());
    return String (buffer);
  }
};

/* --- Affine --- */
class Affine {
protected:
  /* ( xx yx )
   * ( xy yy )
   * ( xz yz )
   */
  double xx, xy, xz, yx, yy, yz;
public:
  Affine (double cxx = 1,
          double cxy = 0,
          double cxz = 0,
          double cyx = 0,
          double cyy = 1,
          double cyz = 0) :
    xx (cxx), xy (cxy), xz (cxz),
    yx (cyx), yy (cyy), yz (cyz)
  {}
  Affine&
  translate (double tx, double ty)
  {
    multiply (Affine (1, 0, tx, 0, 1, ty));
    return *this;
  }
  Affine&
  translate (Point p)
  {
    multiply (Affine (1, 0, p.x, 0, 1, p.y));
    return *this;
  }
  Affine&
  set_translation (double tx, double ty)
  {
    xz = tx;
    yz = ty;
    return *this;
  }
  Affine&
  hflip()
  {
    xx = -xx;
    yx = -yx;
    xz = -xz;
    return *this;
  }
  Affine&
  vflip()
  {
    xy = -xy;
    yy = -yy;
    yz = -yz;
    return *this;
  }
  Affine&
  rotate (double theta)
  {
    double s = sin (theta);
    double c = cos (theta);
    return multiply (Affine (c, -s, 0, s, c, 0));
  }
  Affine&
  rotate (double theta, Point anchor)
  {
    translate (anchor.x, anchor.y);
    rotate (theta);
    return translate (-anchor.x, -anchor.y);
  }
  Affine&
  scale (double sx, double sy)
  {
    xx *= sx;
    xy *= sy;
    yx *= sx;
    yy *= sy;
    return *this;
  }
  Affine&
  shear (double shearx, double sheary)
  {
    return multiply (Affine (1, shearx, 0, sheary, 1, 0));
  }
  Affine&
  shear (double theta)
  {
    return multiply (Affine (1, tan (theta), 0, 0, 1, 0));
  }
  Affine&
  multiply (const Affine &a2)
  {
    Affine dst; // dst * point = this * (a2 * point)
    dst.xx = a2.xx * xx + a2.yx * xy;
    dst.xy = a2.xy * xx + a2.yy * xy;
    dst.xz = a2.xz * xx + a2.yz * xy + xz;
    dst.yx = a2.xx * yx + a2.yx * yy;
    dst.yy = a2.xy * yx + a2.yy * yy;
    dst.yz = a2.xz * yx + a2.yz * yy + yz;
    return *this = dst;
  }
  Affine&
  multiply_swapped (const Affine &a2)
  {
    Affine dst; // dst * point = a2 * (this * point)
    dst.xx = xx * a2.xx + yx * a2.xy;
    dst.xy = xy * a2.xx + yy * a2.xy;
    dst.xz = xz * a2.xx + yz * a2.xy + a2.xz;
    dst.yx = xx * a2.yx + yx * a2.yy;
    dst.yy = xy * a2.yx + yy * a2.yy;
    dst.yz = xz * a2.yx + yz * a2.yy + a2.yz;
    return *this = dst;
  }
  Point
  point (const Point &s) const
  {
    Point d;
    d.x = xx * s.x + xy * s.y + xz;
    d.y = yx * s.x + yy * s.y + yz;
    return d;
  }
  Point point (double x, double y) const { return point (Point (x, y)); }
  double
  determinant() const
  {
    /* if this is != 0, the affine is invertible */
    return xx * yy - xy * yx;
  }
  double
  expansion() const
  {
    return sqrt (fabs (determinant()));
  }
  Affine&
  invert()
  {
    double rec_det = 1.0 / determinant();
    Affine dst (yy, -xy, 0, -yx, xx, 0);
    dst.xx *= rec_det;
    dst.xy *= rec_det;
    dst.yx *= rec_det;
    dst.yy *= rec_det;
    dst.xz = -(dst.xx * xz + dst.xy * yz);
    dst.yz = -(dst.yy * yz + dst.yx * xz);
    return *this = dst;
  }
  Point
  ipoint (const Point &s) const
  {
    double rec_det = 1.0 / determinant();
    Point d;
    d.x = yy * s.x - xy * s.y;
    d.x *= rec_det;
    d.x -= xz;
    d.y = xx * s.y - yx * s.x;
    d.y *= rec_det;
    d.y -= yz;
    return d;
  }
  Point ipoint (double x, double y) const { return ipoint (Point (x, y)); }
  Point
  operator* (const Point &p) const
  {
    return point (p);
  }
  Affine
  operator* (const Affine &a2) const
  {
    return Affine (*this).multiply (a2);
  }
  Affine&
  operator= (const Affine &a2)
  {
    xx = a2.xx;
    xy = a2.xy;
    xz = a2.xz;
    yx = a2.yx;
    yy = a2.yy;
    yz = a2.yz;
    return *this;
  }
  bool
  is_identity () const
  {
    return xx == 1 && xy == 0 && xz == 0 && yx == 0 && yy == 1 && yz == 0;
  }
  Affine
  create_inverse () const
  {
    Affine inv (*this);
    return inv.invert();
  }
  String        string() const;
  static Affine from_triangles (Point src_a, Point src_b, Point src_c,
                                Point dst_a, Point dst_b, Point dst_c);
  struct VectorReturn { double x, y, z; };
  VectorReturn x() const { VectorReturn v = { xx, xy, xz }; return v; }
  VectorReturn y() const { VectorReturn v = { yx, yy, yz }; return v; }
};
struct AffineIdentity : Affine {
  AffineIdentity() :
    Affine (1, 0, 0, 0, 1, 0)
  {}
};
struct AffineHFlip : Affine {
  AffineHFlip() :
    Affine (-1, 0, 0, 0, 1, 0)
  {}
};
struct AffineVFlip : Affine {
  AffineVFlip() :
    Affine (1, 0, 0, 0, -1, 0)
  {}
};
struct AffineTranslate : Affine {
  AffineTranslate (double tx, double ty)
  {
    xx = 1;
    xy = 0;
    xz = tx;
    yx = 0;
    yy = 1;
    yz = ty;
  }
  AffineTranslate (Point p)
  {
    xx = 1;
    xy = 0;
    xz = p.x;
    yx = 0;
    yy = 1;
    yz = p.y;
  }
};
struct AffineScale : Affine {
  AffineScale (double sx, double sy)
  {
    xx = sx;
    xy = 0;
    xz = 0;
    yx = 0;
    yy = sy;
    yz = 0;
  }
};
struct AffineRotate : Affine {
  AffineRotate (double theta)
  {
    double s = sin (theta);
    double c = cos (theta);
    xx = c;
    xy = -s;
    xz = 0;
    yx = s;
    yy = c;
    yz = 0;
  }
  AffineRotate (double theta, Point anchor)
  {
    rotate (theta, anchor);
  }    
};
struct AffineShear : Affine {
  AffineShear (double shearx, double sheary)
  {
    xx = 1;
    xy = shearx;
    xz = 0;
    yx = sheary;
    yy = 1;
    yz = 0;
  }
  AffineShear (double theta)
  {
    xx = 1;
    xy = tan (theta);
    xz = 0;
    yx = 0;
    yy = 1;
    yz = 0;
  }
};

/* --- Plane --- */
class Plane {
  int     m_x, m_y, m_stride, m_height;
  uint32 *m_pixel_buffer;
  uint    n_pixels() const                  { return height() * m_stride; }
  Color   peek_color (uint x, uint y) const { const uint32 *p = peek (x, y); return Color::from_premultiplied (*p); }
public:
  uint32*       peek (uint x, uint y)       { return &m_pixel_buffer[y * m_stride + x]; }
  const uint32* peek (uint x, uint y) const { return &m_pixel_buffer[y * m_stride + x]; }
  explicit      Plane (int x, int y, uint width, uint height);
  virtual       ~Plane();
  int           pixstride  () const { return m_stride * 4; }
  int           width      () const { return m_stride; } //FIXME
  int           height     () const { return m_height; }
  Rect          rect       () const { return Rect (Point (m_x, m_y), width(), height()); }
  Point         origin     () const { return Point (m_x, m_y); }
  int           xstart     () const { return m_x; }
  int           ystart     () const { return m_y; }
  int           xbound     () const { return m_x + width(); }
  int           ybound     () const { return m_y + height(); }
  int           channels   () const { return 4; }
  uint32*       poke_span  (int x, int y, uint len)
  {
    int ix = x - m_x, iy = y - m_y;
    if (ix >= 0 && iy >= 0 && len > 0 && ix + len <= (uint) width() && iy < height())
      return peek (ix, iy);
    return NULL;
  }
  void   fill              (Color c);
  void   combine           (const Plane &src, CombineType ct = COMBINE_NORMAL, uint8 lucent = 0xff);
  bool   rgb_convert       (uint cwidth, uint cheight, uint rowstride, uint8 *cpixels) const;
  void   set_premultiplied (int x, int y, Color c)
  {
    int ix = x - m_x, iy = y - m_y;
    if (ix >= 0 && iy >= 0 && ix < width() && iy < height())
      {
        uint32 *pix = peek (ix, iy);
        *pix = c;
      }
  }
  void   set         (int x, int y, Color c) { set_premultiplied (x, y, c.premultiplied()); }
  void   set_channel (int x, int y, uint channel, uint8 v)
  {
    int ix = x - m_x, iy = y - m_y;
    if (ix >= 0 && iy >= 0 && ix < width() && iy < height() && channel < 4)
      {
        uint32 *pix = peek (ix, iy);
        Color c = Color::from_premultiplied (*pix);
        c.channel (channel, v);
        *pix = c.premultiplied();
      }
  }
  void   set_alpha (int x, int y, uint8 v) { set_channel (x, y, 0, v); }
  void   set_red   (int x, int y, uint8 v) { set_channel (x, y, 1, v); }
  void   set_green (int x, int y, uint8 v) { set_channel (x, y, 2, v); }
  void   set_blue  (int x, int y, uint8 v) { set_channel (x, y, 3, v); }
  static Plane
  create_from_intersection (const Plane &master, Point p0, double pwidth, double pheight)
  {
    Rect m (master.origin(), master.width(), master.height());
    Rect b (p0, pwidth, pheight);
    b.intersect (m);
    if (b.is_empty())
      return Plane (iround (p0.x), iround (p0.y), 0, 0);
    else
      return Plane (iround (b.ll.x), iround (b.ll.y), iceil (b.width()), iceil (b.height()));
  }
  static Plane
  create_from_size (const Plane &master)
  {
    return create_from_intersection (master, master.origin(), master.width(), master.height());
  }
};

/* --- Display --- */
class Display {
  struct Layer {
    Plane      *plane;
    CombineType ctype;
    double      alpha;
  };
  std::list<Layer> layer_stack;
  std::list<Rect>  clip_stack;
public:
  explicit      Display();
  virtual       ~Display();
  void          push_clip_rect  (int x, int y, uint width, uint height) { push_clip_rect (Rect (Point (x, y), width, height)); }
  void          push_clip_rect  (const Rect &rect);
  Plane&        create_plane    (CombineType ctype = COMBINE_NORMAL,
                                 double      alpha = 1.0); /* 0..1 */
  bool          empty           () const;
  void          pop_clip_rect   ();
  void          render_combined (Plane &plane);
};

} // Rapicorn

#endif  /* __RAPICORN_PRIMITIVES_HH__ */
