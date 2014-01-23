/**
Copyright 2013 3DSGuy

This file is part of icn_extractor.

icn_extractor is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

icn_extractor is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with icn_extractor.  If not, see <http://www.gnu.org/licenses/>.
**/
#include <stdlib.h>
#include <stdint.h>
//Bools
typedef enum
{
	False,
	True
} _boolean;

typedef enum
{
	Good,
	Fail
} return_basic;

typedef enum
{
	ARGC_FAIL = 1,
	ARGV_FAIL,
	aes_key_fail,
	rsa_key_fail,
	cia_type_fail,
	cert_gen_fail,
	content_mismatch,
	ticket_gen_fail,
	tmd_gen_fail,
	cia_header_gen_fail
} errors;

typedef enum
{
	BE = 0,
	LE = 1
} endianness_flag;

typedef enum
{
	KB = 1024,
	MB = 1048576,
	GB = 1073741824
} file_unit_size;

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long long      u64;

typedef signed char     s8;
typedef signed short    s16;
typedef signed int      s32;
typedef signed long long        s64;
