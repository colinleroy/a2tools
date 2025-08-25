/*
 * Copyright (C) 2023 Colin Leroy-Mira <colin@colino.net>
 *
 * This file draws heavily from the following projects:
 * - http://wsxyz.net/tohgr.html
 *   No author/copyright info found
 * - https://www.appleoldies.ca/bmp2dhr/
 *   Copyright Bill Buckels 2014
 * - https://github.com/eiroca/apple2graphic
 *   Copyright (C) 1996-2019 eIrOcA Enrico Croce & Simona Burzio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Thanks to http://wsxyz.net/tohgr.html */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <SDL_image.h>

#include "../surl_protocol.h"
#include "jpeglib.h"
#include "png.h"
#include "../log.h"

static unsigned baseaddr[192];
static unsigned char grbuf[65536];

// static ImageRef testimage;

static void init_base_addrs (void)
{
  unsigned int i, group_of_eight, line_of_eight, group_of_sixtyfour;

  for (i = 0; i < 192; ++i)
  {
    line_of_eight = i % 8;
    group_of_eight = (i % 64) / 8;
    group_of_sixtyfour = i / 64;

    baseaddr[i] = line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }
}

static void sdl_set_pixel32(SDL_Surface *surface, int x, int y, Uint32 p) {
  if (x >= 0 && x < surface->w && y >= 0 && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = p;
}

static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  sdl_set_pixel32(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
}

static Uint32 *sdl_get_pixel32_ref(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p;

    if (x >= surface->w || y >= surface->h)
      return NULL;

    p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    return (Uint32 *)p;
}

static Uint32 sdl_get_pixel32(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p;

  if (x >= surface->w || y >= surface->h)
    return 0x00;

  p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
    case 1:
      return *p;

    case 2:
      return *(Uint16 *)p;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
      return *(Uint32 *)p;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
}

static void sdl_get_pixel(SDL_Surface *surface, int x, int y, Uint8 *r, Uint8 *g, Uint8 *b) {
  Uint32 data = sdl_get_pixel32(surface, x, y);
  SDL_GetRGB(data, surface->format, r, g, b);
}

static int sdl_pixel_dist (SDL_Surface *s, Uint32 p1, Uint32 p2)
{
  int d, pd;
  Uint8 p1r, p1g, p1b, p2r, p2g, p2b;
  SDL_GetRGB(p1, s->format, &p1r, &p1g, &p1b);
  SDL_GetRGB(p2, s->format, &p2r, &p2g, &p2b);
  d = (int)p1r - (int)p2r;
  pd = d * d;
  d = (int)p1g - (int)p2g;
  pd = pd + d * d;
  d = (int)p1b - (int)p2b;
  return pd + d * d;
}

static int sdl_pixel_dist7 (SDL_Surface *s, Uint32 *pv1, Uint32 *pv2)
{
  int i, d = 0;
  for (i = 0; i < 7; ++i)
    d = d + sdl_pixel_dist(s, pv1[i], pv2[i]);
  return d;
}

static void sdl_alter_pixel (Uint8 *r, Uint8 *g, Uint8 *b, int dr, int dg, int db)
{
  dr += *r;
  dg += *g;
  db += *b;
  if (dr < 0) dr = 0; else if (dr > 255) dr = 255;
  if (dg < 0) dg = 0; else if (dg > 255) dg = 255;
  if (db < 0) db = 0; else if (db > 255) db = 255;
  *r = dr;
  *g = dg;
  *b = db;
}

enum DitherType { EDIFF, ATKIN };

static void sdl_dither7 (SDL_Surface *im, enum DitherType alg, Uint32 *buf, Uint32 *pal, int numPal)
{
  int z,i,d,bd,bi,dr,dg,db;
  Uint8 r, g, b, pr, pg, pb;
  for (z = 0; z < 7; z += 1) {
    bd = bi = 0x7fffffff; // big number
    for (i = 0; i < numPal; ++i) {
      d = sdl_pixel_dist(im, buf[z], pal[i]);
      if (d < bd) {
        bd = d;
        bi = i;
      }
    }

    SDL_GetRGB(buf[z], im->format, &r, &g, &b);
    SDL_GetRGB(pal[bi], im->format, &pr, &pg, &pb);
    dr = (int)r - (int)pr;
    dg = (int)g - (int)pg;
    db = (int)b - (int)pb;

    if (alg == ATKIN)
    {
      SDL_GetRGB(buf[z+1], im->format, &r, &g, &b);
      sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
      buf[z+1] = SDL_MapRGB(im->format, r, g, b);

      SDL_GetRGB(buf[z+2], im->format, &r, &g, &b);
      sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
      buf[z+2] = SDL_MapRGB(im->format, r, g, b);
    }
    else
    {
      SDL_GetRGB(buf[z+1], im->format, &r, &g, &b);
      sdl_alter_pixel(&r, &g, &b, dr*7/16, dg*7/16, db*7/16);
      buf[z+1] = SDL_MapRGB(im->format, r, g, b);
    }

    buf[z] = pal[bi];
  }
}

static void sdl_dither_from_buf (enum DitherType alg, Uint32 *p, SDL_Surface *im, unsigned x, unsigned y)
{
  int i,j,k,dr,dg,db;
  Uint8 ipr, ipg, ipb;
  Uint8 imr, img, imb;
  Uint8 r, g, b;

  for (i = 0; i < 7; ++i)
  {
    sdl_get_pixel(im, x+i, y, &imr, &img, &imb);

    SDL_GetRGB(p[i], im->format, &ipr, &ipg, &ipb);
    dr = (int)imr - (int)ipr;
    dg = (int)img - (int)ipg;
    db = (int)imb - (int)ipb;
    sdl_set_pixel32(im, x+i, y, p[i]);

    if (i == 6) {
      if (alg == ATKIN) {
        sdl_get_pixel(im, x+7, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(im, x+7, y, r, g, b);
        sdl_get_pixel(im, x+8, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(im, x+8, y, r, g, b);
      }
      else
      {
        sdl_get_pixel(im, x+7, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr*7/16, dg*7/16, db*7/16);
        sdl_set_pixel(im, x+7, y, r, g, b);
      }
    }

    if (y < im->h)
    {
      if (alg == ATKIN)
      {
        sdl_get_pixel(im, x+i, y+2, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(im, x+i, y+2, r, g, b);
        for (j = 0; j < 3; j += 1)
        {
          k = x + i + j - 1;
          sdl_get_pixel(im, k, y+1, &r, &g, &b);
          sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
          sdl_set_pixel(im, k, y+1, r, g, b);
        }
      }
      else
      {
        static int f[3] = {3,5,1};
        for (j = 0; j < 3; j += 1)
        {
          k = x + i + j - 1;
          sdl_get_pixel(im, k, y+1, &r, &g, &b);
          sdl_alter_pixel(&r, &g, &b, dr*f[j]/16, dg*f[j]/16, db*f[j]/16);
          sdl_set_pixel(im, k, y+1, r, g, b);
        }
      }
    }
  }
}

static void sdl_hgr_bytes (SDL_Surface *src, int startx, int starty, unsigned char *hgr)
{
  int x, y, s, i;
  unsigned char b, h;
  Uint8 pr, pg, pb;
  Uint32 *p = sdl_get_pixel32_ref(src, startx, starty);

  i = s = 0;
  for (y = 0; y < 2; ++y)
  {
    b = h = 0;
    for (x = 0; x < 7; ++x)
    {
      if (s == 0)
      {
        s = 1;
        SDL_GetRGB(p[i], src->format, &pr, &pg, &pb);
        switch (pr)
        {
        case 0x34:  /* blue */
          h = 0x80;
          /* continue */
        case 0xe8:  /* violet */
        case 0xff:  /* white */
          b = (b >> 1) | 0x80;
          break;
        case 0x1d:  /* green */
        case 0xd0:  /* orange */
        default:  /* black */
          b = b >> 1;
          break;
        }
      }
      else /* s == 1 */
      {
        s = 0;
        i = i + 1;
        if (startx + i < src->w) {
          SDL_GetRGB(p[i], src->format, &pr, &pg, &pb);
          switch (pr)
          {
          case 0xd0:  /* orange */
            h = 0x80;
            /* continue */
          case 0x1d:  /* green */
          case 0xff:  /* white */
            b = (b >> 1) | 0x80;
            break;
          case 0xe8:  /* violet */
          case 0x34:  /* blue */
          default:  /* black */
            b = b >> 1;
            break;
          }
        } else {
          b = h = 0;
        }
      }
    }

    hgr[y] = (b >> 1) | h;
  }
}

static void hgr_color_dither (SDL_Surface *src)
{
  static Uint32 pal1[4];
  static Uint32 pal2[4];
  enum DitherType alg = EDIFF;
  int y,x,z,d1,d2;
  Uint32 *pp;
  Uint32 buf1[9], buf2[9];
  static int pal_init = 0;
  if (!pal_init) {
    pal1[0] = SDL_MapRGB(src->format, 0, 0, 0);        /* black */
    pal1[1] = SDL_MapRGB(src->format, 0xe8,0x2c,0xf8); /* violet */
    pal1[2] = SDL_MapRGB(src->format, 0x1d,0xd6,0x09); /* green */
    pal1[3] = SDL_MapRGB(src->format, 0xff,0xff,0xff); /* white */

    pal2[0] = SDL_MapRGB(src->format, 0, 0, 0);        /* black */
    pal2[1] = SDL_MapRGB(src->format, 0x34,0x85,0xfc); /* blue */
    pal2[2] = SDL_MapRGB(src->format, 0xd0,0x81,0x01); /* orange */
    pal2[3] = SDL_MapRGB(src->format, 0xff,0xff,0xff); /* white */
    pal_init = 1;
  }
  for (y = 0; y < src->h; y += 1) {
    for (x = 0; x < src->w; x += 7) {
      for (z = 0; z < 9; z += 1) {
        buf1[z] = sdl_get_pixel32(src, x+z, y);
        buf2[z] = sdl_get_pixel32(src, x+z, y);
      }
      sdl_dither7(src, alg, buf1, pal1, 4);
      sdl_dither7(src, alg, buf2, pal2, 4);
      pp = sdl_get_pixel32_ref(src, x, y);
      d1 = sdl_pixel_dist7(src, pp, buf1);
      d2 = sdl_pixel_dist7(src, pp, buf2);
      if (d1 < d2)
        sdl_dither_from_buf(alg, buf1, src, x, y);
      else
        sdl_dither_from_buf(alg, buf2, src, x, y);
    }
  }
}

#define RGBTOUINT32(r,g,b) ((r)<< 16 | (g) <<8 | b)

static Uint32 dhgr_pal_in[16];
static Uint32 dhgr_pal_out[16];

void init_dhgr_pal(void) {
    /* Modern => DHGR */
    dhgr_pal_in[0] = RGBTOUINT32(1, 4, 8);
    dhgr_pal_in[1] = RGBTOUINT32(32, 54, 212);
    dhgr_pal_in[2] = RGBTOUINT32(51, 111,   0);
    dhgr_pal_in[3] = RGBTOUINT32(7, 168, 225);
    dhgr_pal_in[4] = RGBTOUINT32(99,  77,   0);
    dhgr_pal_in[5] = RGBTOUINT32(126, 126, 126);
    dhgr_pal_in[6] = RGBTOUINT32(67, 200,   0);
    dhgr_pal_in[7] = RGBTOUINT32(93, 248, 133);
    dhgr_pal_in[8] = RGBTOUINT32(148,  12, 125);
    dhgr_pal_in[9] = RGBTOUINT32(188,  55, 255);
    dhgr_pal_in[10] = RGBTOUINT32(126, 126, 126);
    dhgr_pal_in[11] = RGBTOUINT32(158, 172, 255);
    dhgr_pal_in[12] = RGBTOUINT32(249,  86,  29);
    dhgr_pal_in[13] = RGBTOUINT32(255, 129, 236);
    dhgr_pal_in[14] = RGBTOUINT32(221, 206,  23);
    dhgr_pal_in[15] = RGBTOUINT32(248, 250, 244);

    /* DHGR => Modern */
    dhgr_pal_out[0] = RGBTOUINT32(0x00, 0x00, 0x00);    // $0 black
    dhgr_pal_out[1] = RGBTOUINT32(0xdd, 0x00, 0x33);    // $1 red (magenta)
    dhgr_pal_out[2] = RGBTOUINT32(0x88, 0x55, 0x00);    // $8 brown
    dhgr_pal_out[3] = RGBTOUINT32(0xff, 0x66, 0x00);    // $9 orange
    dhgr_pal_out[4] = RGBTOUINT32(0x00, 0x77, 0x22);    // $4 dark green
    dhgr_pal_out[5] = RGBTOUINT32(0x55, 0x55, 0x55);    // $5 grey1 (dark)
    dhgr_pal_out[6] = RGBTOUINT32(0x11, 0xdd, 0x00);    // $C green (a/k/a light green)
    dhgr_pal_out[7] = RGBTOUINT32(0xff, 0xff, 0x00);    // $D yellow
    dhgr_pal_out[8] = RGBTOUINT32(0x99, 0x00, 0x00);    // $2 dark blue
    dhgr_pal_out[9] = RGBTOUINT32(0xdd, 0x22, 0xdd);    // $3 purple (violet)
    dhgr_pal_out[10] = RGBTOUINT32(0xaa, 0xaa, 0xaa);   // $A grey2 (light)
    dhgr_pal_out[11] = RGBTOUINT32(0xff, 0x99, 0x88);   // $B pink
    dhgr_pal_out[12] = RGBTOUINT32(0x22, 0x22, 0xff);   // $6 medium blue
    dhgr_pal_out[13] = RGBTOUINT32(0x66, 0xaa, 0xff);   // $7 light blue
    dhgr_pal_out[14] = RGBTOUINT32(0x44, 0xff, 0x99);   // $E aqua
    dhgr_pal_out[15] = RGBTOUINT32(0xff, 0xff, 0xff);   // $F white
}

static void dhgr_color_dither (SDL_Surface *src)
{
  Uint32 *p;
  enum DitherType alg = EDIFF;
  int y,x,d,i,bd,bi,dr,dg,db;
  Uint8 pr,pg,pb,r,g,b;

  init_dhgr_pal();

  for (y = 0; y < src->h; y += 1) {
    for (x = 0; x < src->w; x += 1) {
      bd = bi = 0x7fffffff;
      p = sdl_get_pixel32_ref(src, x, y);

      for (i = 0; i < 16; ++i) {
        d = sdl_pixel_dist(src, *p, dhgr_pal_in[i]);
        if (d < bd) {
          bd = d;
          bi = i;
        }
      }

      /* pixel val */
      SDL_GetRGB(*p, src->format, &r, &g, &b);
      /* palette val */
      SDL_GetRGB(dhgr_pal_in[bi], src->format, &pr, &pg, &pb);
      /* distance */
      dr = (int)r - (int)pr;
      dg = (int)g - (int)pg;
      db = (int)b - (int)pb;

      sdl_set_pixel(src, x, y, pr, pg, pb);
      if (alg == ATKIN)
      {
        sdl_get_pixel(src, x+1, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x+1, y, r, g, b);

        sdl_get_pixel(src, x+2, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x+2, y, r, g, b);

        sdl_get_pixel(src, x-1, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x-1, y+1, r, g, b);

        sdl_get_pixel(src, x, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x, y+1, r, g, b);

        sdl_get_pixel(src, x+1, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x+1, y+1, r, g, b);

        sdl_get_pixel(src, x, y+2, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/8, dg/8, db/8);
        sdl_set_pixel(src, x, y+2, r, g, b);
      }
      else
      {
        sdl_get_pixel(src, x+1, y, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr*7/16, dg*7/16, db*7/16);
        sdl_set_pixel(src, x+1, y, r, g, b);

        sdl_get_pixel(src, x-1, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr*3/16, dg*3/16, db*3/16);
        sdl_set_pixel(src, x-1, y+1, r, g, b);

        sdl_get_pixel(src, x, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr*5/16, dg*5/16, db*5/16);
        sdl_set_pixel(src, x, y+1, r, g, b);

        sdl_get_pixel(src, x+1, y+1, &r, &g, &b);
        sdl_alter_pixel(&r, &g, &b, dr/16, dg/16, db/16);
        sdl_set_pixel(src, x+1, y+1, r, g, b);
      }
    }
  }
}


static void sdl_image_scale (SDL_Surface *src, SDL_Surface *dst, int w, int h, int dhgr, float asprat, int resizeto)
{
  int sw, sh, bx, sx, sy;
  Uint8 red, green, blue;
  float srcw, srch, dstw, dsth, r, g, b;
  float xfactor, yfactor, scalefactor, x, y, nx, ny, ix, iy, nix, niy, area;
  int fast = 0;
  srcw = (float)src->w;
  srch = (float)src->h;
  dstw = (float)w;
  dsth = (float)h;


  if (srcw == dstw && srch < dsth && !dhgr) {
    SDL_Rect outrect;
    outrect.x = 0;
    outrect.y = (dsth - srch) / 2;
    outrect.w = dstw;
    outrect.h = dsth;
    SDL_BlitSurface(src, NULL, dst, &outrect);
  } else if (srcw < dstw && srch == dsth && !dhgr) {
    SDL_Rect outrect;
    outrect.x = (dstw - srcw) / 2;
    outrect.y = 0;
    outrect.w = dstw;
    outrect.h = dsth;
    SDL_BlitSurface(src, NULL, dst, &outrect);
  } else if (srch != dsth || srcw != dstw) {
    SDL_LockSurface(dst);
    xfactor = srcw / dstw / asprat;
    yfactor = srch / dsth;

    scalefactor = xfactor;
    if (yfactor > scalefactor) scalefactor = yfactor;
    if (srcw < dstw && srch < dsth) {
      scalefactor = 1.0L;
    }

    sw = (int)(srcw / scalefactor / asprat);
    sh = (int)(srch / scalefactor);
    sx = (int)(((float)dst->w - sw) / 2);
    sy = (int)(((float)dst->h - sh) / 2);
    if (resizeto == HGR_SCALE_MIXHGR) {
      sy -= (192-160)/2;
      if (sy < 0) {
        sy = 0;
      }
    }

    xfactor = scalefactor * asprat;

    // LOG("HGR: xfact:%g yfact:%g\n", xfactor, yfactor);
    // LOG("HGR: srcw:%g srch:%g sw:%d sh:%d\n", srcw, srch, sw, sh);
    // LOG("HGR: dstw:%g dsth:%g sx:%d sy:%d\n", dstw, dsth, sx, sy);
    bx = sx;

    if (fast) {
      for (h = sy; h < dsth; h ++)
      {
        for (w = sx; w < dstw; w ++)
        {
          sdl_get_pixel(src, w * xfactor, h * yfactor, &red, &green, &blue);
          sdl_set_pixel(dst, w, h, red, green, blue);
        }
      }
    } else {
      for (y = 0.0; y < srch; y = ny)
      {
        ny = y + scalefactor;
        for (x = 0.0; x < srcw; x = nx)
        {
          nx = x + xfactor;
          r = g = b = 0;
          for (iy = y; iy < ny; iy = niy)
          {
            niy = (int)(iy + 1.0);
            for (ix = x; ix < nx; ix = nix)
            {
              nix = (int)(ix + 1.0);
              area = 1.0;

              if ((iy - (int)iy) > 0.00001) {
                area = niy - iy;
              }
              else if (iy + 1.0 >= ny) {
                area = ny - iy;
              }

              if ((ix - (int)ix) > 0.00001) {
                area = area * (nix - ix);
              }
              else if (ix + 1.0 >= nx) {
                area = area * (nx - ix);
              }

              sdl_get_pixel(src, (int)ix, (int)iy, &red, &green, &blue);

              r += (area * (float)red);
              g += (area * (float)green);
              b += (area * (float)blue);
            }
          }

          r = r / xfactor / scalefactor;
          g = g / xfactor / scalefactor;
          b = b / xfactor / scalefactor;

          if (r < 0.0) r = 0.0; else if (r >= 256.0) r = 255.0;
          if (g < 0.0) g = 0.0; else if (g >= 256.0) g = 255.0;
          if (b < 0.0) b = 0.0; else if (b >= 256.0) b = 255.0;

          r = (int)(r+0.1);
          g = (int)(g+0.1);
          b = (int)(b+0.1);

          sdl_set_pixel(dst, sx, sy, r, g, b);
          sx += 1;
        }
        sx = bx;
        sy += 1;
      }
    }

    SDL_UnlockSurface(dst);
  } else {
    LOG("No resizing needed.\n");
    SDL_BlitSurface(src, NULL, dst, NULL);
  }
}

static inline void setRGB(png_byte *ptr, int value) {
  ptr[0] = (value >> 16) & 255;
  ptr[1] = (value >> 8) & 255;
  ptr[2] = value & 255;
}

char *hgr_to_png(char *hgr_buf, size_t hgr_len, char monochrome, size_t *len)
{
  int x,y;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_bytep row = NULL;
  int width = 280;
  int height = 192;
  FILE *tmpfp = NULL;
  int v;
  char v1, v2;
  char c, fl1, fl2;
  int ox;
  int oc1, oc2;
  char solid = 1;
  char *out_buf = NULL;
  char is_dhgr = 0;

  int hgr_col[8] = {
    0x000000,
    0xdd22dd,
    0x00dd11,
    0xffffff,
    0x000000,
    0xff2222,
    0x0066ff,
    0xffffff
  };


  init_base_addrs();

  LOG("HGR: Converting %zu bytes\n", hgr_len);
  if (hgr_len != 8192 && hgr_len != 16384) {
    LOG("HGR: Wrong HGR size %zu\n", hgr_len);
    return NULL;
  }
  is_dhgr = hgr_len == 16384;
  if (is_dhgr) {
    width = 560;
    height = 384;
  }
 
  /* create file */
  /* coverity[secure_temp] */
  tmpfp = tmpfile();
  if (!tmpfp) {
    LOG("HGR: [write_png_file] File could not be opened for writing");
    goto cleanup;
  }

  /* initialize stuff */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    LOG("HGR: [write_png_file] png_create_write_struct failed");
    goto cleanup;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    LOG("HGR: [write_png_file] png_create_info_struct failed");
    goto cleanup;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    LOG("HGR: [write_png_file] Error during init_io");
    goto cleanup;
  }

  png_init_io(png_ptr, tmpfp);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr))) {
    LOG("HGR: [write_png_file] Error during writing header");
    goto cleanup;
  }

  png_set_IHDR(png_ptr, info_ptr, width, height,
    8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  row = (png_bytep) malloc(3 * width);

  if (!monochrome) {
    if (!is_dhgr) {
      for (y=0; y < height; y++){
        char *ord_hgr = hgr_buf + baseaddr[y];
        x = 0;
        do {
          v1 = *(ord_hgr++);
          v2 = *(ord_hgr++);

          fl1 = (v1 & 0x80) ? 4: 0;
          fl2 = (v2 & 0x80) ? 4: 0;

          v = (v1 & 0x7f) + ((v2 & 0x7f) << 7);

          for (ox = 0; ox < 7; ox++) {
            c = v & 3;
            v = v >> 2;

            if (ox < 3) {
              oc2 = c + fl1;
              oc1 = oc2;
            } else if (ox > 3) {
              oc2 = c + fl2;
              oc1 = oc2;
            } else if (ox == 3) {
              oc1 = c + fl1;
              oc2 = c + fl2;
            }

            oc1 = (solid | (c & 1)) ? hgr_col[oc1] : 0;
            oc2 = (solid | (c & 2)) ? hgr_col[oc2] : 0;

            setRGB(&(row[(x++)*3]), oc1);
            setRGB(&(row[(x++)*3]), oc2);
          }
        } while (x < width);
        png_write_row(png_ptr, row);
      }
    } else {
      int pixel_bits[4+560+4];
      int color_buf[560];
      /* Color DHGR */
      for (y=0; y < height; y++){
        char *line_data = hgr_buf + baseaddr[y];
        int *bitptr = pixel_bits + 4;

        bzero(pixel_bits, sizeof(pixel_bits));

        for (int byte = 0; byte < 560/7; byte++) {
          Uint8 val;

          if (byte % 2 == 1) {
            val = *(line_data+0x2000);
            line_data++;
          } else {
            val = *line_data;
          }

          for (int bit = 0; bit < 7; bit++) {
            *bitptr++ = val & 0x01;
            val >>= 1;
          }
        }

        bitptr = pixel_bits + 4;

        for (int idx = 0; idx < 140; idx++) {
          Uint8 pixval = *bitptr++;

          pixval = (pixval << 1) | (*bitptr++);
          pixval = (pixval << 1) | (*bitptr++);
          pixval = (pixval << 1) | (*bitptr++);

          color_buf[idx*4] =
            color_buf[idx*4 +1] =
            color_buf[idx*4 +2] =
            color_buf[idx*4 +3] = pixval;
        }

        x = 0;
        init_dhgr_pal();
        for (int pix = 0; pix < 280; pix++) {
          int bufpos = pix * 2;
          int pixel1, pixel2;

          pixel1 = dhgr_pal_out[color_buf[bufpos]];
          pixel2 = dhgr_pal_out[color_buf[bufpos+1]];
          setRGB(&(row[(x++)*3]), pixel1);
          setRGB(&(row[(x++)*3]), pixel2);
        }
        
        png_write_row(png_ptr, row);
        png_write_row(png_ptr, row);
      }
    }
  } else {
    for (y=0; y < 192; y++){
      char *ord_hgr = hgr_buf + baseaddr[y];
      char sx;
      int pass = 0;
      x = 0;
      ord_hgr = hgr_buf + baseaddr[y];

row_pass:
      for (sx = 0; sx < width / (is_dhgr?14:7); sx++) {
        v1 = *(ord_hgr++);

        for (ox = 0; ox < 7; ox++) {
          setRGB(&(row[(x++)*3]), (v1 & 1) ? 0xffffff : 0);
          v1 = v1 >> 1;
        }
        if (is_dhgr)
          x+=7;
      }
      if (is_dhgr && pass == 0) {
        x = 7;
        ord_hgr = hgr_buf + baseaddr[y]+0x2000;
        pass = 1;
        goto row_pass;
      }
      png_write_row(png_ptr, row);
      if (is_dhgr)
        png_write_row(png_ptr, row);
    }
  }

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))) {
    LOG("HGR: [write_png_file] Error during end of write");
    goto cleanup;
  }

  png_write_end(png_ptr, NULL);

  *len = ftell(tmpfp);
  rewind(tmpfp);
  out_buf = malloc(*len);
  if (fread(out_buf, 1, *len, tmpfp) < *len) {
    free(out_buf);
    out_buf = NULL;
    *len = 0;
  }

cleanup:
  if (tmpfp)
    fclose(tmpfp);
  if (row)
    free(row);
  if (info_ptr) {
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    free(info_ptr);
  }
  if (png_ptr)
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

  return out_buf;
}

static void mono_dither_bayer(SDL_Surface* s) {
  Uint32 x, y;

  // Ordered dither kernel
  Uint16 map[8][8] = {
    { 1, 49, 13, 61, 4, 52, 16, 64 },
    { 33, 17, 45, 29, 36, 20, 48, 32 },
    { 9, 57, 5, 53, 12, 60, 8, 56 },
    { 41, 25, 37, 21, 44, 28, 40, 24 },
    { 3, 51, 15, 63, 2, 50, 14, 62 },
    { 25, 19, 47, 31, 34, 18, 46, 30 },
    { 11, 59, 7, 55, 10, 58, 6, 54 },
    { 43, 27, 39, 23, 42, 26, 38, 22 }
  };

  for(y = 0; y < s->h; ++y) {
    for(x = 0; x < s->w; ++x) {
      Uint8 r, g, b;
      sdl_get_pixel(s, x, y, &r, &g, &b);

      // Convert the pixel value to grayscale i.e. intensity
      int val = (299 * r + 587 * g + 114 * b) / 1000;

      // Apply the ordered dither kernel
      val += val * map[y % 8][x % 8] / 63;

      // If >= 192 choose white, else choose black
      if(val >= 128)
        val = 255;
      else
        val = 0;

      // Put the pixel back in the image
      sdl_set_pixel(s, x, y, val, val, val);
    }
  }
}

static void greyscale_image(SDL_Surface *s) {
  Uint32 x, y;
  for(y = 0; y < s->h; ++y) {
    for(x = 0; x < s->w; ++x) {
      Uint8 r, g, b;
      float in;
      sdl_get_pixel(s, x, y, &r, &g, &b);

      // Convert the pixel value to grayscale i.e. intensity
      in = .299 * r + .587 * g + .114 * b;
      sdl_set_pixel(s, x, y, (Uint8)in, 0, 0);
    }
  }
}
static void mono_dither_burkes(SDL_Surface* s) {
  Uint32 x, y;
  float **error_table;
  int threshold = 180;
#define ERR_X_OFF 2 //avoid special cases at start/end of line
  error_table = malloc(sizeof(float *) * (s->h + 5));
  for (y = 0; y < s->h + 5; y++) {
    error_table[y] = malloc(sizeof(float) * (s->w + 5));
    memset(error_table[y], 0x00, sizeof(float) * (s->w + 5));
  }

  greyscale_image(s);

  for(y = 0; y < s->h; ++y) {
    for(x = 0; x < s->w; ++x) {
      Uint8 g, b;
      Uint8 in;
      float current_error;
      sdl_get_pixel(s, x, y, &in, &g, &b); /* g & b ignored */

      if (threshold > in + error_table[y][x + ERR_X_OFF]) {
        sdl_set_pixel(s, x, y, 0, 0, 0);
        current_error = in + error_table[y][x + ERR_X_OFF];
      } else {
        sdl_set_pixel(s, x, y, 255, 255, 255);
        current_error = in + error_table[y][x + ERR_X_OFF] - 255;
      }
      error_table[y][x + 1 + ERR_X_OFF] += (int)(8.0L / 32.0L * current_error);
      error_table[y][x + 2 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x + ERR_X_OFF] += (int)(8.0L / 32.0L * current_error);
      error_table[y + 1][x + 1 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x + 2 + ERR_X_OFF] += (int)(2.0L / 32.0L * current_error);
      error_table[y + 1][x - 1 + ERR_X_OFF] += (int)(4.0L / 32.0L * current_error);
      error_table[y + 1][x - 2 + ERR_X_OFF] += (int)(2.0L / 32.0L * current_error);

    }
  }

  for (y = 0; y < s->h + 5; y++) {
    free(error_table[y]);
  }
  free(error_table);
}

static int sdl_color_hgr(SDL_Surface *src, unsigned char *hgr) {
  int x, y;
  unsigned int ad, i;

  for (y = 0; y < src->h; y += 1) {
    i = 0;
    ad = baseaddr[y];
    for (x = 0; x < src->w; x += 7) {
      sdl_hgr_bytes(src, x, y, hgr+ad+i);
      i += 2;
    }
  }

  return 0x2000;
}

static int sdl_color_dhgr(SDL_Surface *src, unsigned char *hgr) {
  int x, y;
  unsigned int ad, i, d;
  Uint8 r,g,b;

	for (y = 0; y < src->h; y += 1) {
		ad = baseaddr[y];
		for (x = 0; x < src->w; x += 7) {
			d = 0;
			for (i = 0; i < 7; i += 1) {
        sdl_get_pixel(src, x + i, y, &r, &g, &b);
				switch (r) {
					/**/
				case   1: d = (d >> 4) | (0x00 << 24); break;
				case  32: d = (d >> 4) | (0x01 << 24); break;
				case  51: d = (d >> 4) | (0x02 << 24); break;
				case   7: d = (d >> 4) | (0x03 << 24); break;
				case  99: d = (d >> 4) | (0x04 << 24); break;

				case  67: d = (d >> 4) | (0x06 << 24); break;
				case  93: d = (d >> 4) | (0x07 << 24); break;
				case 148: d = (d >> 4) | (0x08 << 24); break;
				case 188: d = (d >> 4) | (0x09 << 24); break;

				case 158: d = (d >> 4) | (0x0b << 24); break;
				case 249: d = (d >> 4) | (0x0c << 24); break;
				case 255: d = (d >> 4) | (0x0d << 24); break;
				case 221: d = (d >> 4) | (0x0e << 24); break;
				case 248: d = (d >> 4) | (0x0f << 24); break;

				case 126: d = (d >> 4) | ((d & (1 << 23)) ? (0x0a << 24) : (0x05 << 24)); break;
					/**/
				default: if (r) printf("Unknown pixel (%2.2x)\n", r); d = d >> 4; break;
				}
			}
			
			for (i = 0; i < 4; ++i) {
				unsigned char bits;
				unsigned int  addr;
				addr = ad + i/2;
				bits = (unsigned char)(d & 0x7f);
				if (i & 1) {
					hgr[0x2000+addr] = bits;
				}
				else {
					hgr[addr] = bits;
				}
				d = d >> 7;
			}
			
			ad += 2;
		}
	}

  return 0x4000;
}

static int sdl_mono_hgr(SDL_Surface *src, unsigned char *hgr) {
  int x, y, base, xoff, pixel;
  Uint32 color;
  unsigned char *ptr;
  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x1,0x2,0x4,0x8,0x10,0x20,0x40};

  memset(hgr, 0x00, 0x2000);

  for (y = 0; y < src->h; y++) {
    base = baseaddr[y];
    for (x = 0; x < src->w; x++) {
      xoff = base + x/7;
      pixel = x%7;
      ptr = hgr + xoff;
      color = sdl_get_pixel32(src, x, y);
      if (color != 0x00) {
        ptr[0] |= dhwmono[pixel];
      } else {
        ptr[0] &= dhbmono[pixel];
      }
    }
  }

  return 0x2000;
}

static int sdl_mono_dhgr(SDL_Surface *src, unsigned char *hgr) {
  int x, y, base, xoff, pixel;
  Uint32 color;
  unsigned char *ptr;
  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x1,0x2,0x4,0x8,0x10,0x20,0x40};

  memset(hgr, 0x00, 0x4000);
  printf("DHGR source %dx%d\n", src->w, src->h);
  for (y = 0; y < src->h; y++) {
    base = baseaddr[y];
    for (x = 0; x < src->w; x++) {
      xoff = base + ((x/7)/2);
      pixel = (x % 280)%7;
      ptr = hgr + xoff + (((x/7)%2)*0x2000);

      color = sdl_get_pixel32(src, x, y);
      if (color != 0x00) {
        ptr[0] |= dhwmono[pixel];
      } else {
        ptr[0] &= dhbmono[pixel];
      }
    }
  }

  return 0x4000;
}

unsigned char *sdl_to_hgr(const char *filename, char monochrome, char save_preview, size_t *len, char bayer_dither, HGRScale size) {
  SDL_Surface *image, *resized;
  int dst_w, dst_h;
  init_base_addrs();

  if (filename == NULL) {
    return NULL;
  }

  /* Open the image file */
  image = IMG_Load(filename);
  if ( image == NULL) {
    if (!save_preview) {
      LOG("Couldn't load image %s: %s\n", filename, SDL_GetError());
    }
    *len = 0;
    return NULL;
  }

  dst_w = monochrome ? 280 : 140;
  dst_h = 192;
  resized = SDL_CreateRGBSurface (0, dst_w, dst_h, 32, 0, 0, 0, 0);

  if (size == HGR_SCALE_HALF) {
    dst_w /= 2;
    dst_h /= 2;
  } else if (size == HGR_SCALE_MIXHGR) {
    dst_h = 160;
  }
  sdl_image_scale(image, resized, dst_w, dst_h, 0, monochrome ? 0.952381 : 1.904762, size);
  if (monochrome) {

    if (bayer_dither)
      mono_dither_bayer(resized);
    else
      mono_dither_burkes(resized);

    *len = sdl_mono_hgr(resized, grbuf);
  } else {
    hgr_color_dither(resized);
    *len = sdl_color_hgr(resized, grbuf);
  }

  if (save_preview) {
    char preview_file[255];
    snprintf(preview_file, sizeof(preview_file), "%s.hgr-preview.png", filename);
    SDL_SaveBMP(resized, preview_file);
    LOG("Saved preview: %s\n", preview_file);
  }

  SDL_FreeSurface(resized);
  SDL_FreeSurface(image);

  // https://prodos8.com/docs/technote/ftn/08/
  if (monochrome) {
    grbuf[0x78] = 0;
  } else {
    grbuf[0x78] = 1;
  }
  return grbuf;
}

unsigned char *sdl_to_dhgr(const char *filename, char monochrome, char save_preview, size_t *len, char bayer_dither, HGRScale size) {
  SDL_Surface *image, *resized;
  int dst_w, dst_h;
  init_base_addrs();

  if (filename == NULL) {
    return NULL;
  }

  /* Open the image file */
  image = IMG_Load(filename);
  if ( image == NULL) {
    if (!save_preview) {
      LOG("Couldn't load image %s: %s\n", filename, SDL_GetError());
    }
    *len = 0;
    return NULL;
  }

  dst_w = monochrome ? 560 : 140;
  dst_h = 192;
  resized = SDL_CreateRGBSurface (0, dst_w, dst_h, 32, 0, 0, 0, 0);

  if (size == HGR_SCALE_HALF) {
    dst_w /= 2;
    dst_h /= 2;
  } else if (size == HGR_SCALE_MIXHGR) {
    dst_h = 160;
  }
  sdl_image_scale(image, resized, dst_w, dst_h, 1, monochrome ? 0.952381/2 : 1.904762, size);
  if (monochrome) {

    if (bayer_dither)
      mono_dither_bayer(resized);
    else
      mono_dither_burkes(resized);

    *len = sdl_mono_dhgr(resized, grbuf);
  } else {
    dhgr_color_dither(resized);
    *len = sdl_color_dhgr(resized, grbuf);
  }

  if (save_preview) {
    char preview_file[255];
    snprintf(preview_file, sizeof(preview_file), "%s.hgr-preview.png", filename);
    SDL_SaveBMP(resized, preview_file);
    LOG("Saved preview: %s\n", preview_file);
  }

  SDL_FreeSurface(resized);
  SDL_FreeSurface(image);

  // https://prodos8.com/docs/technote/ftn/08/
  if (monochrome) {
    grbuf[0x78] = 2;
  } else {
    grbuf[0x78] = 3;
  }
  return grbuf;
}
