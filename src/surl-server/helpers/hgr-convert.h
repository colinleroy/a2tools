#ifndef __hgr_convert_h
#define __hgr_convert_h

char *hgr_to_png(char *hgr_data, size_t hgr_len, char monochrome, size_t *len);
unsigned char *sdl_to_hgr(const char *filename, char monochrome, char save_preview, size_t *len);

#endif
