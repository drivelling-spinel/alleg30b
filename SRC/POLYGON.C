/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      The polygon rasteriser.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
#include <sys/farptr.h>

#include "allegro.h"
#include "internal.h"


typedef struct POLYGON_EDGE      /* an active edge */
{
   int top;                      /* top y position */
   int bottom;                   /* bottom y position */
   fixed x, dx;                  /* fixed point x position and gradient */
   fixed w;                      /* width of line segment */
   POLYGON_SEGMENT dat;          /* texture/gouraud information */
   struct POLYGON_EDGE *prev;    /* doubly linked list */
   struct POLYGON_EDGE *next;
} POLYGON_EDGE;


#define POLYGON_FIX_SHIFT     18



/* fill_edge_structure:
 *  Polygon helper function: initialises an edge structure for the 2d
 *  rasteriser.
 */
static void fill_edge_structure(POLYGON_EDGE *edge, int *i1, int *i2)
{
   if (i2[1] < i1[1]) {
      int *it;

      it = i1;
      i1 = i2;
      i2 = it;
   }

   edge->top = i1[1];
   edge->bottom = i2[1] - 1;
   edge->dx = ((i2[0] - i1[0]) << POLYGON_FIX_SHIFT) / (i2[1] - i1[1]);
   edge->x = (i1[0] << POLYGON_FIX_SHIFT) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
   edge->prev = NULL;
   edge->next = NULL;

   if (edge->dx < 0)
      edge->x += MIN(edge->dx+(1<<POLYGON_FIX_SHIFT), 0);

   edge->w = MAX(ABS(edge->dx)-(1<<POLYGON_FIX_SHIFT), 0);
}



/* add_edge:
 *  Adds an edge structure to a linked list, returning the new head pointer.
 */
static POLYGON_EDGE *add_edge(POLYGON_EDGE *list, POLYGON_EDGE *edge, int sort_by_x)
{
   POLYGON_EDGE *pos = list;
   POLYGON_EDGE *prev = NULL;

   if (sort_by_x) {
      while ((pos) && ((pos->x + (pos->w + pos->dx) / 2) < 
		       (edge->x + (edge->w + edge->dx) / 2))) {
	 prev = pos;
	 pos = pos->next;
      }
   }
   else {
      while ((pos) && (pos->top < edge->top)) {
	 prev = pos;
	 pos = pos->next;
      }
   }

   edge->next = pos;
   edge->prev = prev;

   if (pos)
      pos->prev = edge;

   if (prev) {
      prev->next = edge;
      return list;
   }
   else
      return edge;
}



/* remove_edge:
 *  Removes an edge structure from a list, returning the new head pointer.
 */
static POLYGON_EDGE *remove_edge(POLYGON_EDGE *list, POLYGON_EDGE *edge)
{
   if (edge->next) 
      edge->next->prev = edge->prev;

   if (edge->prev) {
      edge->prev->next = edge->next;
      return list;
   }
   else
      return edge->next;
}



/* polygon:
 *  Draws a filled polygon with an arbitrary number of corners. Pass the 
 *  number of vertices, then an array containing a series of x, y points 
 *  (a total of vertices*2 values).
 */
void polygon(BITMAP *bmp, int vertices, int *points, int color)
{
   int c;
   int top = INT_MAX;
   int bottom = INT_MIN;
   int *i1, *i2;
   POLYGON_EDGE *edge, *next_edge;
   POLYGON_EDGE *active_edges = NULL;
   POLYGON_EDGE *inactive_edges = NULL;

   /* allocate some space and fill the edge table */
   _grow_scratch_mem(sizeof(POLYGON_EDGE) * vertices);

   edge = (POLYGON_EDGE *)_scratch_mem;
   i1 = points;
   i2 = points + (vertices-1) * 2;

   for (c=0; c<vertices; c++) {
      if (i1[1] != i2[1]) {
	 fill_edge_structure(edge, i1, i2);

	 if (edge->bottom >= edge->top) {

	    if (edge->top < top)
	       top = edge->top;

	    if (edge->bottom > bottom)
	       bottom = edge->bottom;

	    inactive_edges = add_edge(inactive_edges, edge, FALSE);
	    edge++;
	 }
      }
      i2 = i1;
      i1 += 2;
   }

   if (bottom >= bmp->cb)
      bottom = bmp->cb-1;

   /* for each scanline in the polygon... */
   for (c=top; c<=bottom; c++) {

      /* check for newly active edges */
      edge = inactive_edges;
      while ((edge) && (edge->top == c)) {
	 next_edge = edge->next;
	 inactive_edges = remove_edge(inactive_edges, edge);
	 active_edges = add_edge(active_edges, edge, TRUE);
	 edge = next_edge;
      }

      /* draw horizontal line segments */
      edge = active_edges;
      while ((edge) && (edge->next)) {
	 hline(bmp, edge->x>>POLYGON_FIX_SHIFT, c, 
	       (edge->next->x+edge->next->w)>>POLYGON_FIX_SHIFT, color);
	 edge = edge->next->next;
      }

      /* update edges, sorting and removing dead ones */
      edge = active_edges;
      while (edge) {
	 next_edge = edge->next;
	 if (c >= edge->bottom) {
	    active_edges = remove_edge(active_edges, edge);
	 }
	 else {
	    edge->x += edge->dx;
	    while ((edge->prev) && 
		   (edge->x+edge->w/2 < edge->prev->x+edge->prev->w/2)) {
	       if (edge->next)
		  edge->next->prev = edge->prev;
	       edge->prev->next = edge->next;
	       edge->next = edge->prev;
	       edge->prev = edge->prev->prev;
	       edge->next->prev = edge;
	       if (edge->prev)
		  edge->prev->next = edge;
	       else
		  active_edges = edge;
	    }
	 }
	 edge = next_edge;
      }
   }
}



/* triangle:
 *  Draws a filled triangle between the three points. Note: this depends 
 *  on a dodgy assumption about parameter passing conventions. I assume that 
 *  the point coordinates are all on the stack in consecutive locations, so 
 *  I can pass that block of stack memory as the array for polygon() without 
 *  bothering to copy the data to a temporary location.
 */
void triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
   polygon(bmp, 3, &x1, color);
}



/* bitfield specifying which polygon attributes need interpolating */
#define INTERP_FLAT           1
#define INTERP_1COL           2
#define INTERP_3COL           4
#define INTERP_FIX_UV         8
#define INTERP_Z              16
#define INTERP_FLOAT_UV       32


/* prototype for the scanline filler functions */
typedef void (*SCANLINE_FILLER)(unsigned long addr, int w, POLYGON_SEGMENT *info);



/* fill_3d_edge_structure:
 *  Polygon helper function: initialises an edge structure for the 3d 
 *  rasterising code, using fixed point vertex structures.
 */
static void fill_3d_edge_structure(POLYGON_EDGE *edge, V3D *v1, V3D *v2, int flags)
{
   int h;

   /* swap vertices if they are the wrong way up */
   if (v2->y < v1->y) {
      V3D *vt;

      vt = v1;
      v1 = v2;
      v2 = vt;
   }

   /* set up screen rasterising parameters */
   edge->top = fixtoi(v1->y);
   edge->bottom = fixtoi(v2->y) - 1;
   h = edge->bottom - edge->top + 1;
   edge->dx = ((v2->x - v1->x) << (POLYGON_FIX_SHIFT-16)) / h;
   edge->x = (v1->x << (POLYGON_FIX_SHIFT-16)) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
   edge->prev = NULL;
   edge->next = NULL;
   edge->w = 0;

   if (flags & INTERP_1COL) {
      /* single color shading interpolation */
      edge->dat.c = itofix(v1->c);
      edge->dat.dc = itofix(v2->c - v1->c) / h;
   }

   if (flags & INTERP_3COL) {
      /* RGB shading interpolation */
      int r1 = (v1->c >> 16) & 0xFF;
      int r2 = (v2->c >> 16) & 0xFF;
      int g1 = (v1->c >> 8) & 0xFF;
      int g2 = (v2->c >> 8) & 0xFF;
      int b1 = v1->c & 0xFF;
      int b2 = v2->c & 0xFF;

      edge->dat.r = itofix(r1);
      edge->dat.g = itofix(g1);
      edge->dat.b = itofix(b1);
      edge->dat.dr = itofix(r2 - r1) / h;
      edge->dat.dg = itofix(g2 - g1) / h;
      edge->dat.db = itofix(b2 - b1) / h;
   }

   if (flags & INTERP_FIX_UV) {
      /* fixed point (affine) texture interpolation */
      edge->dat.u = v1->u;
      edge->dat.v = v1->v;
      edge->dat.du = (v2->u - v1->u) / h;
      edge->dat.dv = (v2->v - v1->v) / h;
   }

   if (flags & INTERP_Z) {
      /* Z (depth) interpolation */
      float z1 = 1.0 / fixtof(v1->z);
      float z2 = 1.0 / fixtof(v2->z);

      edge->dat.z = z1;
      edge->dat.dz = (z2 - z1) / h;

      if (flags & INTERP_FLOAT_UV) {
	 /* floating point (perspective correct) texture interpolation */
	 float fu1 = fixtof(v1->u + (1<<15)) * z1 * 65536;
	 float fv1 = fixtof(v1->v + (1<<15)) * z1 * 65536;
	 float fu2 = fixtof(v2->u + (1<<15)) * z2 * 65536;
	 float fv2 = fixtof(v2->v + (1<<15)) * z2 * 65536;

	 edge->dat.fu = fu1;
	 edge->dat.fv = fv1;
	 edge->dat.dfu = (fu2 - fu1) / h;
	 edge->dat.dfv = (fv2 - fv1) / h;
      }
   }
}



/* fill_3d_edge_structure_f:
 *  Polygon helper function: initialises an edge structure for the 3d 
 *  rasterising code, using floating point vertex structures.
 */
static void fill_3d_edge_structure_f(POLYGON_EDGE *edge, V3D_f *v1, V3D_f *v2, int flags)
{
   int h;

   /* swap vertices if they are the wrong way up */
   if (v2->y < v1->y) {
      V3D_f *vt;

      vt = v1;
      v1 = v2;
      v2 = vt;
   }

   /* set up screen rasterising parameters */
   edge->top = (int)(v1->y+0.5);
   edge->bottom = (int)(v2->y+0.5) - 1;
   h = edge->bottom - edge->top + 1;
   edge->dx = ((v2->x - v1->x) * (float)(1<<POLYGON_FIX_SHIFT)) / h;
   edge->x = (v1->x * (float)(1<<POLYGON_FIX_SHIFT)) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
   edge->prev = NULL;
   edge->next = NULL;
   edge->w = 0;

   if (flags & INTERP_1COL) {
      /* single color shading interpolation */
      edge->dat.c = itofix(v1->c);
      edge->dat.dc = itofix(v2->c - v1->c) / h;
   }

   if (flags & INTERP_3COL) {
      /* RGB shading interpolation */
      int r1 = (v1->c >> 16) & 0xFF;
      int r2 = (v2->c >> 16) & 0xFF;
      int g1 = (v1->c >> 8) & 0xFF;
      int g2 = (v2->c >> 8) & 0xFF;
      int b1 = v1->c & 0xFF;
      int b2 = v2->c & 0xFF;

      edge->dat.r = itofix(r1);
      edge->dat.g = itofix(g1);
      edge->dat.b = itofix(b1);
      edge->dat.dr = itofix(r2 - r1) / h;
      edge->dat.dg = itofix(g2 - g1) / h;
      edge->dat.db = itofix(b2 - b1) / h;
   }

   if (flags & INTERP_FIX_UV) {
      /* fixed point (affine) texture interpolation */
      edge->dat.u = ftofix(v1->u);
      edge->dat.v = ftofix(v1->v);
      edge->dat.du = ftofix(v2->u - v1->u) / h;
      edge->dat.dv = ftofix(v2->v - v1->v) / h;
   }

   if (flags & INTERP_Z) {
      /* Z (depth) interpolation */
      float z1 = 1.0 / v1->z;
      float z2 = 1.0 / v2->z;

      edge->dat.z = z1;
      edge->dat.dz = (z2 - z1) / h;

      if (flags & INTERP_FLOAT_UV) {
	 /* floating point (perspective correct) texture interpolation */
	 float fu1 = (v1->u + 0.5) * z1 * 65536;
	 float fv1 = (v1->v + 0.5) * z1 * 65536;
	 float fu2 = (v2->u + 0.5) * z2 * 65536;
	 float fv2 = (v2->v + 0.5) * z2 * 65536;

	 edge->dat.fu = fu1;
	 edge->dat.fv = fv1;
	 edge->dat.dfu = (fu2 - fu1) / h;
	 edge->dat.dfv = (fv2 - fv1) / h;
      }
   }
}



/* get_scanline_filler:
 *  Helper function for deciding which rasterisation function and 
 *  interpolation flags we should use for a specific polygon type.
 */
static SCANLINE_FILLER get_scanline_filler(int type, int *flags, POLYGON_SEGMENT *info, BITMAP *texture, BITMAP *bmp)
{
   typedef struct POLYTYPE_INFO {
      SCANLINE_FILLER filler;
      int flags; 
   } POLYTYPE_INFO;

   static POLYTYPE_INFO polytype_info[] =
   {
      {  _poly_scanline_flat,       INTERP_FLAT                              },
      {  _poly_scanline_gcol,       INTERP_1COL                              },
      {  _poly_scanline_grgb,       INTERP_3COL                              },
      {  _poly_scanline_atex,       INTERP_FIX_UV                            },
      {  _poly_scanline_ptex,       INTERP_Z | INTERP_FLOAT_UV               },
      {  _poly_scanline_atex_mask,  INTERP_FIX_UV                            },
      {  _poly_scanline_ptex_mask,  INTERP_Z | INTERP_FLOAT_UV               },
      {  _poly_scanline_atex_lit,   INTERP_FIX_UV | INTERP_1COL              },
      {  _poly_scanline_ptex_lit,   INTERP_Z | INTERP_FLOAT_UV | INTERP_1COL }
   };

   type = MID(0, type, (int)(sizeof(polytype_info)/sizeof(POLYTYPE_INFO)-1));

   *flags = polytype_info[type].flags;

   if (texture) {
      info->texture = texture->line[0];
      info->umask = texture->w - 1;
      info->vmask = texture->h - 1;
      info->vshift = 0;
      while ((1 << info->vshift) < texture->w)
	 info->vshift++;
   }
   else {
      info->texture = NULL;
      info->umask = info->vmask = info->vshift = 0;
   }

   info->seg = bmp->seg;
   _farsetsel(bmp->seg);

   return polytype_info[type].filler;
}



/* clip_polygon_segment:
 *  Updates interpolation state values when skipping several places, eg.
 *  clipping the first part of a scanline.
 */
static void clip_polygon_segment(POLYGON_SEGMENT *info, int gap, int flags)
{
   if (flags & INTERP_1COL)
      info->c += info->dc * gap;

   if (flags & INTERP_3COL) {
      info->r += info->dr * gap;
      info->g += info->dg * gap;
      info->b += info->db * gap;
   }

   if (flags & INTERP_FIX_UV) {
      info->u += info->du * gap;
      info->v += info->dv * gap;
   }

   if (flags & INTERP_Z) {
      info->z += info->dz * gap;

      if (flags & INTERP_FLOAT_UV) {
	 info->fu += info->dfu * gap;
	 info->fv += info->dfv * gap;
      }
   }
}



/* draw_polygon_segment: 
 *  Polygon helper function to fill a scanline. Calculates deltas for 
 *  whichever values need interpolating, clips the segment, and then calls
 *  the lowlevel scanline filler.
 */
static void draw_polygon_segment(BITMAP *bmp, int y, POLYGON_EDGE *e1, POLYGON_EDGE *e2, SCANLINE_FILLER drawer, int flags, int color, POLYGON_SEGMENT *info)
{
   int x = e1->x >> POLYGON_FIX_SHIFT;
   int w = (e2->x >> POLYGON_FIX_SHIFT) - x;
   int gap;

   if ((w <= 0) || (x+w <= bmp->cl) || (x >= bmp->cr))
      return;

   if (flags & INTERP_FLAT) {
      info->c = color;
      info->dc = 0;
   }
   else if (flags & INTERP_1COL) {
      info->c = e1->dat.c;
      info->dc = (e2->dat.c - e1->dat.c) / w;
   }

   if (flags & INTERP_3COL) {
      info->r = e1->dat.r;
      info->g = e1->dat.g;
      info->b = e1->dat.b;
      info->dr = (e2->dat.r - e1->dat.r) / w;
      info->dg = (e2->dat.g - e1->dat.g) / w;
      info->db = (e2->dat.b - e1->dat.b) / w;
   }

   if (flags & INTERP_FIX_UV) {
      info->u = e1->dat.u + (1<<15);
      info->v = e1->dat.v + (1<<15);
      info->du = (e2->dat.u - e1->dat.u) / w;
      info->dv = (e2->dat.v - e1->dat.v) / w;
   }

   if (flags & INTERP_Z) {
      info->z = e1->dat.z;
      info->dz = (e2->dat.z - e1->dat.z) / w;

      if (flags & INTERP_FLOAT_UV) {
	 info->fu = e1->dat.fu;
	 info->fv = e1->dat.fv;
	 info->dfu = (e2->dat.fu - e1->dat.fu) / w;
	 info->dfv = (e2->dat.fv - e1->dat.fv) / w;
      }
   }

   if (x < bmp->cl) {
      gap = bmp->cl - x;
      x = bmp->cl;
      w -= gap;
      clip_polygon_segment(info, gap, flags);
   }

   if (x+w > bmp->cr)
      w = bmp->cr - x;

   drawer(bmp_write_line(bmp, y)+x, w, info);
}



/* do_polygon3d:
 *  Helper function for rendering 3d polygon, used by both the fixed point
 *  and floating point drawing functions.
 */
static void do_polygon3d(BITMAP *bmp, int top, int bottom, POLYGON_EDGE *inactive_edges, SCANLINE_FILLER drawer, int flags, int color, POLYGON_SEGMENT *info)
{
   int y;
   int old87 = 0;
   POLYGON_EDGE *edge, *next_edge;
   POLYGON_EDGE *active_edges = NULL;

   if (bottom >= bmp->cb)
      bottom = bmp->cb-1;

   /* set fpu to single-precision, truncate mode */
   if (flags & (INTERP_Z || INTERP_FLOAT_UV))
      old87 = _control87(PC_24 | RC_CHOP, MCW_PC | MCW_RC);

   /* get four copies of the color */
   color = (color | (color << 8) | (color << 16) | (color << 24));

   /* for each scanline in the polygon... */
   for (y=top; y<=bottom; y++) {

      /* check for newly active edges */
      edge = inactive_edges;
      while ((edge) && (edge->top == y)) {
	 next_edge = edge->next;
	 inactive_edges = remove_edge(inactive_edges, edge);
	 active_edges = add_edge(active_edges, edge, TRUE);
	 edge = next_edge;
      }

      /* fill the scanline */
      draw_polygon_segment(bmp, y, active_edges, active_edges->next, drawer, flags, color, info);

      /* update edges, removing dead ones */
      edge = active_edges;
      while (edge) {
	 next_edge = edge->next;
	 if (y >= edge->bottom) {
	    active_edges = remove_edge(active_edges, edge);
	 }
	 else {
	    edge->x += edge->dx;

	    if (flags & INTERP_1COL)
	       edge->dat.c += edge->dat.dc;

	    if (flags & INTERP_3COL) {
	       edge->dat.r += edge->dat.dr;
	       edge->dat.g += edge->dat.dg;
	       edge->dat.b += edge->dat.db;
	    }

	    if (flags & INTERP_FIX_UV) {
	       edge->dat.u += edge->dat.du;
	       edge->dat.v += edge->dat.dv;
	    }

	    if (flags & INTERP_Z) {
	       edge->dat.z += edge->dat.dz;

	       if (flags & INTERP_FLOAT_UV) {
		  edge->dat.fu += edge->dat.dfu;
		  edge->dat.fv += edge->dat.dfv;
	       }
	    }
	 }

	 edge = next_edge;
      }
   }

   /* reset fpu mode */
   if (flags & (INTERP_Z || INTERP_FLOAT_UV))
      _control87(old87, MCW_PC | MCW_RC);
}



/* polygon3d:
 *  Draws a 3d polygon in the specified mode. The vertices parameter should
 *  be followed by that many pointers to V3D structures, which describe each
 *  vertex of the polygon.
 */
void polygon3d(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[])
{
   int c;
   int flags;
   int top = INT_MAX;
   int bottom = INT_MIN;
   int v1y, v2y;
   V3D *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_EDGE *inactive_edges = NULL;
   POLYGON_SEGMENT info;
   SCANLINE_FILLER drawer;

   /* set up the drawing mode */
   drawer = get_scanline_filler(type, &flags, &info, texture, bmp);

   /* allocate some space for the active edge table */
   _grow_scratch_mem(sizeof(POLYGON_EDGE) * vc);
   edge = (POLYGON_EDGE *)_scratch_mem;

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];
      v1y = fixtoi(v1->y);
      v2y = fixtoi(v2->y);

      if ((v1y != v2y) && 
	  ((v1y >= bmp->ct) || (v2y >= bmp->ct)) &&
	  ((v1y < bmp->cb) || (v2y < bmp->cb))) {

	 fill_3d_edge_structure(edge, v1, v2, flags);

	 if (edge->top < bmp->ct) {
	    int gap = bmp->ct - edge->top;
	    edge->top = bmp->ct;
	    edge->x += edge->dx * gap;
	    clip_polygon_segment(&edge->dat, gap, flags);
	 }

	 if (edge->bottom >= edge->top) {

	    if (edge->top < top)
	       top = edge->top;

	    if (edge->bottom > bottom)
	       bottom = edge->bottom;

	    inactive_edges = add_edge(inactive_edges, edge, FALSE);
	    edge++;
	 }
      }
   }

   /* render the polygon */
   do_polygon3d(bmp, top, bottom, inactive_edges, drawer, flags, vtx[0]->c, &info);
}



/* polygon3d_f:
 *  Floating point version of polygon3d().
 */
void polygon3d_f(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[])
{
   int c;
   int flags;
   int top = INT_MAX;
   int bottom = INT_MIN;
   int v1y, v2y;
   V3D_f *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_EDGE *inactive_edges = NULL;
   POLYGON_SEGMENT info;
   SCANLINE_FILLER drawer;

   /* set up the drawing mode */
   drawer = get_scanline_filler(type, &flags, &info, texture, bmp);

   /* allocate some space for the active edge table */
   _grow_scratch_mem(sizeof(POLYGON_EDGE) * vc);
   edge = (POLYGON_EDGE *)_scratch_mem;

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];
      v1y = (int)(v1->y+0.5);
      v2y = (int)(v2->y+0.5);

      if ((v1y != v2y) && 
	  ((v1y >= bmp->ct) || (v2y >= bmp->ct)) &&
	  ((v1y < bmp->cb) || (v2y < bmp->cb))) {

	 fill_3d_edge_structure_f(edge, v1, v2, flags);

	 if (edge->top < bmp->ct) {
	    int gap = bmp->ct - edge->top;
	    edge->top = bmp->ct;
	    edge->x += edge->dx * gap;
	    clip_polygon_segment(&edge->dat, gap, flags);
	 }

	 if (edge->bottom >= edge->top) {

	    if (edge->top < top)
	       top = edge->top;

	    if (edge->bottom > bottom)
	       bottom = edge->bottom;

	    inactive_edges = add_edge(inactive_edges, edge, FALSE);
	    edge++;
	 }
      }
   }

   /* render the polygon */
   do_polygon3d(bmp, top, bottom, inactive_edges, drawer, flags, vtx[0]->c, &info);
}



/* triangle3d:
 *  Draws a 3d triangle. Dodgy assumption alert! See comments for triangle().
 */
void triangle3d(BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3)
{
   polygon3d(bmp, type, texture, 3, &v1);
}



/* triangle3d_f:
 *  Draws a 3d triangle. Dodgy assumption alert! See comments for triangle().
 */
void triangle3d_f(BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3)
{
   polygon3d_f(bmp, type, texture, 3, &v1);
}



/* quad3d:
 *  Draws a 3d quad. Dodgy assumption alert! See comments for triangle().
 */
void quad3d(BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4)
{
   polygon3d(bmp, type, texture, 4, &v1);
}



/* quad3d_f:
 *  Draws a 3d quad. Dodgy assumption alert! See comments for triangle().
 */
void quad3d_f(BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4)
{
   polygon3d_f(bmp, type, texture, 4, &v1);
}


