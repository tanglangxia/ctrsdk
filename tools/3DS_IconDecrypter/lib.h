#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
	#include <io.h>
	#include <direct.h>
	#include <windows.h>
	#include <wchar.h>
#else
	#include <sys/stat.h>
	#include <sys/types.h>
#endif

#include "types.h"
#include "utils.h"


