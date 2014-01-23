#ifndef _BLZFS_H_
#define _BLZFS_H_

#include "types.h"

#define BLZ_NORMAL    0          // normal mode
#define BLZ_BEST      1          // best mode

unsigned char* BLZ_Encode(char *filename, unsigned int* pak_len, int mode);

#endif // _EXEFS_H_
