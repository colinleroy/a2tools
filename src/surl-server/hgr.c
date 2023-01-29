/* Thanks to http://wsxyz.net/tohgr.html */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "jpeglib.h"
#include "png.h"

static unsigned baseaddr[192];
static unsigned char grbuf[65536];

struct pixel {
	unsigned char r, g, b;
};

struct image {
	unsigned int h, w;
	struct pixel * p;
};

typedef struct pixel Pixel;
typedef struct pixel * PixelRef;
typedef struct image * ImageRef;

// static ImageRef testimage;

static void initBaseAddrs (void)
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

static Pixel imageGetPixel (ImageRef i, unsigned int x, unsigned int y)
{
	Pixel z = {0,0,0};
	if (x >= i->w) return z;
	if (y >= i->h) return z;
	return i->p[(i->w * y) + x];
}

static void imagePutPixel (ImageRef i, unsigned int x, unsigned int y, Pixel p)
{
	if (x >= i->w) return;
	if (y >= i->h) return;
	i->p[(i->w * y) + x] = p;
}

static PixelRef imageGetPixelRef (ImageRef i, unsigned x, unsigned y)
{
	if (y >= i->h) return 0;
	if (x == i->w && y == 1+i->h) return 0;
	return i->p + (i->w * y) + x;
}

static void imageClear (ImageRef i)
{
	memset(i->p, 0, i->w * i->h * sizeof i->p[0]);
}

static int pixelDist (Pixel p1, Pixel p2)
{
	int d, pd;
	d = (int)p1.r - (int)p2.r;
	pd = d * d;
	d = (int)p1.g - (int)p2.g;
	pd = pd + d * d;
	d = (int)p1.b - (int)p2.b;
	return pd + d * d;
}

static int pixelDist7 (PixelRef pv1, PixelRef pv2)
{
	int i, d = 0;
	for (i = 0; i < 7; ++i)
	d = d + pixelDist(pv1[i], pv2[i]);
	return d;
}

static Pixel alterPixel (Pixel p, int dr, int dg, int db)
{
	dr += (int)p.r;
	dg += (int)p.g;
	db += (int)p.b;
	if (dr < 0) dr = 0; else if (dr > 255) dr = 255;
	if (dg < 0) dg = 0; else if (dg > 255) dg = 255;
	if (db < 0) db = 0; else if (db > 255) db = 255;
	p.r = dr;
	p.g = dg;
	p.b = db;
	return p;
}

struct obuf {
	unsigned char *p;
	unsigned char *n;
	unsigned int len, size;
};

static int initobuf (struct obuf *ob)
{
	ob->len = 0;
	ob->size = 4096;
	ob->n = ob->p = realloc(0, ob->size);
	return ob->p != 0;
}

static int ckobuf (struct obuf *ob, int len)
{
	unsigned char *p;
	if (ob->len + len <= ob->size) return 1;
	p = realloc(ob->p, ob->size + 4096);
	if (!p) return 0;
	ob->n = p + (ob->n - ob->p);
	ob->p = p;
	ob->size = ob->size + 4096;
	return 1;
}

static struct obuf * packBytes (void *vp, unsigned int len)
{
	unsigned int ilen, tlen, rlen, x;
	unsigned char *bp, *ip, *tp, *rp, b;
	static struct obuf ob;
	
	if (!initobuf(&ob)) return 0;
	
	bp = vp;
	ip = vp;
	ilen = len;
	
	while (ilen)
	{
		tp = ip;
		tlen = ilen;
		b = *tp++;
		while (--tlen && b == *tp) tp++;
		
		rlen = tp - ip;
		if (rlen > 2)
		{
			tlen = ip - bp;
			while (tlen > 0)
			{
				x = tlen;
				if (x >= 64) x = 64;
				tlen -= x;
				if (!ckobuf(&ob, x + 1)) return 0;
				ob.len += (x + 1);
				*(ob.n)++ = (unsigned char)(x - 1);
				while (x--)
				*(ob.n)++ = *bp++;
			}
			
			assert(bp == ip);
			
			if (rlen < 8 && rlen % 4)
			{
				if (!ckobuf(&ob, 2)) return 0;
				*(ob.n)++ = 0x40 | ((unsigned char)(rlen - 1));
				*(ob.n)++ = b;
				ob.len += 2;
				ilen -= rlen;
				ip += rlen;
			}
			else
			{
				rlen /= 4;
				if (rlen > 64) rlen = 64;
				if (!ckobuf(&ob, 2)) return 0;
				*(ob.n)++ = 0xC0 | ((unsigned char)(rlen - 1));
				*(ob.n)++ = b;
				ob.len += 2;
				ilen -= (rlen * 4);
				ip += (rlen * 4);
			}
			bp = ip;
			continue;
		}
		
		if (ilen >= 8)
		{
			rp = ip;
			tp = ip + 4;
			tlen = ilen - 4;
			while (tlen && *tp == *rp)
			{
				tp += 1;
				rp += 1;
				tlen -= 1;
			}
			
			rlen = tp - ip;
			if (rlen >= 8)
			{
				tlen = ip - bp;
				while (tlen > 0)
				{
					x = tlen;
					if (x >= 64) x = 64;
					tlen -= x;
					if (!ckobuf(&ob, x + 1)) return 0;
					ob.len += (x + 1);
					*(ob.n)++ = (unsigned char)(x - 1);
					while (x--)
					*(ob.n)++ = *bp++;
				}
				
				assert(bp == ip);
				
				rlen /= 4;
				if (rlen > 64) rlen = 64;
				if (!ckobuf(&ob, 5)) return 0;
				*(ob.n)++ = 0x80 | ((unsigned char)(rlen - 1));
				*(ob.n)++ = ip[0];
				*(ob.n)++ = ip[1];
				*(ob.n)++ = ip[2];
				*(ob.n)++ = ip[3];
				ob.len += 5;
				ilen -= (rlen * 4);
				ip += (rlen * 4);
				bp = ip;
				continue;
			}
		}
		
		ip += 1;
		ilen -= 1;
	}
	
	tlen = ip - bp;
	while (tlen > 0)
	{
		x = tlen;
		if (x >= 64) x = 64;
		tlen -= x;
		if (!ckobuf(&ob, x + 1)) return 0;
		ob.len += (x + 1);
		*(ob.n)++ = (unsigned char)(x - 1);
		while (x--)
		*(ob.n)++ = *bp++;
	}
	
	return &ob;
}

enum DitherType { EDIFF, ATKIN };

static void dither7 (enum DitherType alg, PixelRef buf, PixelRef pal)
{
	int z,i,d,bd,bi,dr,dg,db;
	
	for (z = 0; z < 7; z += 1) {
		bd = bi = 0x7fffffff; // big number
		for (i = 0; i < 4; ++i) {
			d = pixelDist(buf[z], pal[i]);
			if (d < bd) {
				bd = d;
				bi = i;
			}
		}
		
		dr = (int)buf[z].r - (int)pal[bi].r;
		dg = (int)buf[z].g - (int)pal[bi].g;
		db = (int)buf[z].b - (int)pal[bi].b;
		
		if (alg == ATKIN)
		{
			buf[z+1] = alterPixel(buf[z+1], dr/8, dg/8, db/8);
			buf[z+2] = alterPixel(buf[z+2], dr/8, dg/8, db/8);
		}
		else
		{
			buf[z+1] = alterPixel(buf[z+1], dr*7/16, dg*7/16, db*7/16);
		}
		
		buf[z] = pal[bi];
	}
}

static void ditherFromBuf (enum DitherType alg, PixelRef buf, ImageRef im, unsigned x, unsigned y)
{
	int i,j,k,dr,dg,db;
	Pixel ip;
	
	for (i = 0; i < 7; ++i)
	{
		ip = imageGetPixel(im, x+i, y);
		dr = (int)ip.r - (int)buf[i].r;
		dg = (int)ip.g - (int)buf[i].g;
		db = (int)ip.b - (int)buf[i].b;
		imagePutPixel(im, x+i, y, buf[i]);
		
		if (i == 6)
		{
			if (alg == ATKIN)
			{
				imagePutPixel(im, x+7, y, alterPixel(imageGetPixel(im, x+7, y), dr/8, dg/8, db/8));
				imagePutPixel(im, x+8, y, alterPixel(imageGetPixel(im, x+8, y), dr/8, dg/8, db/8));
			}
			else
			{
				imagePutPixel(im, x+7, y, alterPixel(imageGetPixel(im, x+7, y), dr*7/16, dg*7/16, db*7/16));
			}
		}
		
		if (y < im->h)
		{
			if (alg == ATKIN)
			{
				imagePutPixel(im, x+i, y+2, alterPixel(imageGetPixel(im, x+i, y+2), dr/8, dg/8, db/8));
				for (j = 0; j < 3; j += 1)
				{
					k = x + i + j - 1;
					imagePutPixel(im, k, y+1, alterPixel(imageGetPixel(im, k, y+1), dr/8, dg/8, db/8));
				}
			}
			else
			{
				static int f[3] = {3,5,1};
				for (j = 0; j < 3; j += 1)
				{
					k = x + i + j - 1;
					imagePutPixel(im, k, y+1, alterPixel(imageGetPixel(im, k, y+1), dr*f[j]/16, dg*f[j]/16, db*f[j]/16));
				}
			}
		}
	}
}

struct paletteEntry {
	Pixel p;
	int cnt;
	unsigned int smr, smg, smb;
};

struct paletteEntry grpal[] = {
	{{0x00,0x00,0x00},0,0,0,0},
	{{0xad,0x18,0x28},0,0,0,0},
	{{0x55,0x1b,0xe1},0,0,0,0},
	{{0xe8,0x2c,0xf8},0,0,0,0},
	{{0x01,0x73,0x63},0,0,0,0},
	{{0x7e,0x82,0x7f},0,0,0,0},
	{{0x34,0x85,0xfc},0,0,0,0},
	{{0xd1,0x95,0xff},0,0,0,0},
	{{0x33,0x6f,0x00},0,0,0,0},
	{{0xd0,0x81,0x01},0,0,0,0},
	{{0x7f,0x7e,0x77},0,0,0,0},
	{{0xfe,0x93,0xa3},0,0,0,0},
	{{0x1d,0xd6,0x09},0,0,0,0},
	{{0xae,0xea,0x22},0,0,0,0},
	{{0x5b,0xeb,0xd9},0,0,0,0},
	{{0xff,0xff,0xff},0,0,0,0}
};

struct colorSpace {
	unsigned int npixels;
	unsigned char min[3];
	unsigned char max[3];
	unsigned char del[3];
	unsigned char pvec[320][3];
	int ridx;
	int rval;
};

static Pixel greyPals[15][16];
static unsigned char lumiVals[15][16] = {
	{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
	{0x22, 0x22, 0x22, 0x22, 0x66, 0x66, 0x66, 0x66, 0xAA, 0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE, 0xEE},
	{0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x44, 0x66, 0x99, 0xBB, 0xDD, 0xDD, 0xDD, 0xFF, 0xFF, 0xFF},
	{0x00, 0x00, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77},
	{0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88},
	{0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99},
	{0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA},
	{0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB},
	{0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC},
	{0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD},
	{0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE},
	{0x88, 0x88, 0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE, 0xFF, 0xFF},
	{0x00, 0x22, 0x44, 0x66, 0x88, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE, 0xFF, 0xFF},
	{0x00, 0x00, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x66, 0x77, 0x99, 0xBB, 0xDD, 0xFF},
	{0x00, 0x22, 0x44, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xAA, 0xBB, 0xDD, 0xFF}
};

static void initGreyPals (void)
{
	int i,j;
	
	for (i = 0; i < 15; i += 1) {
		for (j = 0; j < 16; j += 1) {
			greyPals[i][j].r = lumiVals[i][j];
			greyPals[i][j].g = lumiVals[i][j];
			greyPals[i][j].b = lumiVals[i][j];
		}
	}
}

enum fileType { FTC00002, FTC10001, FTC10002 };

static void hgrBytes (Pixel *p, unsigned char *hgr)
{
	int x, y, s, i;
	unsigned char b, h;
	
	i = s = 0;
	for (y = 0; y < 2; ++y)
	{
		b = h = 0;
		for (x = 0; x < 7; ++x)
		{
			if (s == 0)
			{
				s = 1;
				switch (p[i].r)
				{
				case 0x34:	/* blue */
					h = 0x80;
					/* continue */
				case 0xe8:	/* violet */
				case 0xff:	/* white */
					b = (b >> 1) | 0x80;
					break;
				case 0x1d:	/* green */
				case 0xd0:	/* orange */
				default:	/* black */
					b = b >> 1;
					break;
				}
			}
			else /* s == 1 */
			{
				s = 0;
				i = i + 1;
				switch (p[i].r)
				{
				case 0xd0:	/* orange */
					h = 0x80;
					/* continue */
				case 0x1d:	/* green */
				case 0xff:	/* white */
					b = (b >> 1) | 0x80;
					break;
				case 0xe8:	/* violet */
				case 0x34:	/* blue */
				default:	/* black */
					b = b >> 1;
					break;
				}
			}
		}
		
		hgr[y] = (b >> 1) | h;
	}
}

static unsigned int hgrDither (enum DitherType alg, ImageRef src, unsigned char *hgr, int pack)
{
	static Pixel pal1[] = {{0,0,0},{0xe8,0x2c,0xf8},{0x1d,0xd6,0x09},{0xff,0xff,0xff}};
	static Pixel pal2[] = {{0,0,0},{0x34,0x85,0xfc},{0xd0,0x81,0x01},{0xff,0xff,0xff}};
	int y,x,z,d1,d2;
	unsigned int ad, i;
	PixelRef pp;
	Pixel buf1[9], buf2[9];
	struct obuf *ob;
	
	for (y = 0; y < src->h; y += 1) {
		for (x = 0; x < src->w; x += 7) {
			for (z = 0; z < 9; z += 1) {
				buf1[z] = imageGetPixel(src, x+z, y);
				buf2[z] = imageGetPixel(src, x+z, y);
			}
			dither7(alg, buf1, pal1);
			dither7(alg, buf2, pal2);
			pp = imageGetPixelRef(src, x, y);
			d1 = pixelDist7(pp, buf1);
			d2 = pixelDist7(pp, buf2);
			if (d1 < d2)
			ditherFromBuf(alg, buf1, src, x, y);
			else
			ditherFromBuf(alg, buf2, src, x, y);
		}
	}
	
	for (y = 0; y < src->h; y += 1) {
		i = 0;
		ad = baseaddr[y];
		for (x = 0; x < src->w; x += 7) {
			hgrBytes(imageGetPixelRef(src,x,y), hgr+ad+i);
			i += 2;
		}
	}
	
	if (!pack) return 0x2000;

	ob = packBytes(hgr, 0x2000);
	if (!ob) return 0;
	
	memcpy(hgr, ob->p, ob->len);
	free(ob->p);
	return ob->len;
}

static ImageRef imageNew (unsigned int w, unsigned int h)
{
	ImageRef ip;
	ip = malloc(sizeof *ip);
	if (!ip) return 0;
	ip->w = w;
	ip->h = h;
	ip->p = malloc(ip->h * ip->w * sizeof ip->p[0]);
	if (ip->p) return ip;
	free(ip);
	return 0;
}

static void imageFree (ImageRef im)
{
	free(im->p);
	free(im);
}

static void imageScale (ImageRef src, ImageRef dst, float asprat)
{
	int sw, sh, bx, sx, sy;
	float srcw, srch, dstw, dsth, r, g, b;
	float xfactor, yfactor, scalefactor, x, y, nx, ny, ix, iy, nix, niy, area;
	Pixel p;
	
	imageClear(dst);
	
	srcw = (float)src->w;
	srch = (float)src->h;
	dstw = (float)dst->w;
	dsth = (float)dst->h;
	
	if (srch > dsth || srcw > dstw)
	{
		xfactor = srcw / dstw / asprat;
		yfactor = srch / dsth;
		
		scalefactor = xfactor;
		if (yfactor > scalefactor) scalefactor = yfactor;
		
		sw = (int)(srcw / scalefactor / asprat);
		sh = (int)(srch / scalefactor);
		sx = (int)(((float)dst->w - sw) / 2);
		sy = (int)(((float)dst->h - sh) / 2);
		bx = sx;
		
		xfactor = scalefactor * asprat;
		
		printf("xfact:%g yfact:%g\n", xfactor, yfactor);
		printf("srcw:%g srch:%g sw:%d sh:%d\n", srcw, srch, sw, sh);
		printf("dstw:%g dsth:%g sx:%d sy:%d\n", dstw, dsth, sx, sy);
		
		for (y = 0.0; y < srch; y = ny)
		{
			ny = y + scalefactor;
			for (x = 0.0; x < srcw; x = nx)
			{
				nx = x + xfactor;
				r = g = b = 0;
				
				if (sy == 52 && sx == 161) {
					putchar('.');
				}
				
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
						
						p = imageGetPixel(src, (int)ix, (int)iy);
						
						r += (area * (float)p.r);
						g += (area * (float)p.g);
						b += (area * (float)p.b);
					}
				}
				
				r = r / xfactor / scalefactor;
				g = g / xfactor / scalefactor;
				b = b / xfactor / scalefactor;
				
				if (r < 0.0) r = 0.0; else if (r >= 256.0) r = 255.0;
				if (g < 0.0) g = 0.0; else if (g >= 256.0) g = 255.0;
				if (b < 0.0) b = 0.0; else if (b >= 256.0) b = 255.0;
				
				p.r = (int)(r+0.1);
				p.g = (int)(g+0.1);
				p.b = (int)(b+0.1);
				
				imagePutPixel(dst, sx, sy, p);
				sx += 1;
			}
			sx = bx;
			sy += 1;
		}
	}
}

struct cmap {
	char cx[8];
	Pixel px;
};

static int cmapcmp (const void *a, const void *b)
{
	char *x, *y;
	x = ((struct cmap *)a)->cx;
	y = ((struct cmap *)b)->cx;
	return strcmp(x,y);
}

static ImageRef imageFromJPG (FILE *f)
{
	ImageRef ip = 0;
	PixelRef pp = 0;
	JSAMPLE * scanline = 0;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	ip = imageNew(cinfo.output_width, cinfo.output_height);
	if (!ip) goto emalloc;
	// fprintf(stderr, "w:%d h:%d\n", ip->w, ip->h);

	scanline = malloc(ip->w * cinfo.out_color_components * sizeof(JSAMPLE));
	if (!scanline) goto emalloc;

	pp = ip->p;
	// fprintf(stderr, "first: %p   limit: %p\n", ip->p, ip->p + ip->h * ip->w);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		int icnt;
		JSAMPLE *sp = scanline;

		jpeg_read_scanlines(&cinfo, &scanline, 1);

		// fprintf(stderr, "[%d:pp=%p] ", cinfo.output_scanline, pp); fflush(stderr);

		if (cinfo.out_color_components == 1)
		{
			for (icnt = ip->w; icnt; --icnt, ++pp)
			{
				pp->r = pp->g = pp->b = *sp++;
			}
		}
		else
		{
			for (icnt = ip->w; icnt; --icnt, ++pp)
			{
				pp->r = *sp++;
				pp->g = *sp++;
				pp->b = *sp++;
			}
		}
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return ip;

emalloc:
	if (scanline) free(scanline);
	if (ip->p) free(ip->p);
	if (ip) free(ip);
	return 0;
}

static ImageRef imageFromPNG(FILE *fp)
{
	ImageRef ip = 0;
	PixelRef pp = 0;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep scanline;
	png_byte color_type;
	int y, i;
	char header[8];    // 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	fread(header, 1, 8, fp);
	if (png_sig_cmp((png_const_bytep)header, 0, 8)) {
		printf("[read_png_file] File is not recognized as a PNG file");
		return NULL;
	}


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		printf("[read_png_file] png_create_read_struct failed");
		return NULL;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		printf("[read_png_file] png_create_info_struct failed");
		return NULL;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		printf("[read_png_file] Error during init_io");
		return NULL;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	ip = imageNew(png_get_image_width(png_ptr, info_ptr), png_get_image_height(png_ptr, info_ptr));
	if (!ip) goto emalloc;	
	png_read_update_info(png_ptr, info_ptr);
	
	scanline = malloc(png_get_rowbytes(png_ptr,info_ptr));
	if (!scanline) goto emalloc;
	/* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		printf("[read_png_file] Error during read_image");
		goto emalloc;
	}

	pp = ip->p;
	for (y=0; y<ip->h; y++){
		png_bytep sp = scanline;
		png_read_row(png_ptr, scanline, NULL);
		for (i = 0; i < ip->w; i++, ++pp)
		{
			pp->r = *sp++;
			pp->g = *sp++;
			pp->b = *sp++;
			if(color_type == PNG_COLOR_TYPE_RGB_ALPHA) sp++;
		}
	}
	return ip;

emalloc:
	if (scanline) free(scanline);
	if (ip->p) free(ip->p);
	if (ip) free(ip);
	return 0;		
}

static char sbuf[65536];
static ImageRef imageFromXPM (FILE *f)
{
	int c, n, ncol, pwid;
	unsigned int r, g, b;
	char *t;
	char pb[8];
	ImageRef ip = 0;
	PixelRef pp;
	Pixel zp = {0,0,0};
	struct cmap *cp = 0, *ep;
	
	c = 0;
	while (c != '"' && !feof(f)) c = fgetc(f);
	if (feof(f)) return 0;
	
	ip = malloc(sizeof *ip);
	if (!ip) goto emalloc;
	
	if (!fgets(sbuf, 65536, f)) goto expm;
	sscanf(sbuf, "%d %d %d %d", &(ip->w), &(ip->h), &ncol, &pwid);
	//printf("w:%d h:%d ncol:%d pwid:%d\n", ip->w, ip->h, ncol, pwid);
	
	ip->p = malloc(ip->h * ip->w * sizeof ip->p[0]);
	if (!ip->p) goto emalloc;
	cp = malloc(ncol * sizeof *cp);
	if (!cp) goto emalloc;
	
	for (n = 0; n < ncol; ++n) {
		if (!fgets(sbuf, 65536, f)) goto expm;
		cp[n].px = zp;
		memset(cp[n].cx, 0, 8);
		strncpy(cp[n].cx, sbuf+1, pwid);
		t = strchr(sbuf+1+pwid, 'c');
		if (t && *(t+2) == '#') {
			sscanf(t+3, "%2x%2x%2x", &r, &g, &b);
			cp[n].px.r = r;
			cp[n].px.g = g;
			cp[n].px.b = b;
		}
	}
	
	qsort(cp, ncol, sizeof *cp, cmapcmp);
	pp = ip->p;
	memset(pb, 0, 8);
	for (n = 0; n < ip->h; ++n) {
		if (!fgets(sbuf, 65536, f)) goto expm;
		t = sbuf+1;
		for (c = 0; c < ip->w; ++c) {
			strncpy(pb, t+pwid*c, pwid);
			ep = bsearch(pb, cp, ncol, sizeof *cp, cmapcmp);
			if (ep) *pp++ = ep->px;
			else *pp++ = zp;
		}
	}
	
	return ip;

expm:
	printf("Error reading XPM file\n");
emalloc:
	if (ip->p) free(ip->p);
	if (ip) free(ip);
	if (cp) free(cp);
	return 0;
}

unsigned char *img_to_hgr(FILE *in, char *format, size_t *len) {
	ImageRef im = NULL, sm = NULL;

	initGreyPals();
	initBaseAddrs();
	
	if (format == NULL || in == NULL) {
		*len = 0;
		return NULL;
	}

	if (!strcasecmp(format, "xpm")) {
		im = imageFromXPM(in);
		if (!im) {
			*len = 0;
			return NULL;
		}
	} else if (!strcasecmp(format, "jpg") || !strcasecmp(format, "jpeg")) {
		im = imageFromJPG(in);
		if (!im) {
			*len = 0;
			return NULL;
		}
	} else if (!strcasecmp(format, "png")) {
		im = imageFromPNG(in);
		if (!im) {
			*len = 0;
			return NULL;
		}
	}
	if (im == NULL) {
		printf("Unhandled format %s\n", format);
		*len = 0;
		return NULL;
	}

	sm = imageNew(140, 192);
	imageScale(im, sm, 1.904762);
	*len = hgrDither(EDIFF, sm, grbuf, 0);

	imageFree(im);
	imageFree(sm);

	return grbuf;
}
