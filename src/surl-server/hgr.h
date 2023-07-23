#ifndef __hgr_h
#define __hgr_h

unsigned char *img_to_hgr(FILE *in, char *format, char monochrome, size_t *len);
char *hgr_to_png(char *hgr_data, size_t hgr_len, char monochrome, size_t *len);

#endif
