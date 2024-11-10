#ifndef __PRODOS_DIR_FILE_COUNT_H
#define __PRODOS_DIR_FILE_COUNT_H

#ifdef __APPLE2__
unsigned int __fastcall__ prodos_dir_file_count(DIR *dir);
#else
#define prodos_dir_file_count(...) 8192
#endif

#endif
