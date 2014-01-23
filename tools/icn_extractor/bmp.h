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

typedef enum
{
	red_mask = 0xF800,
	green_mask = 0x7E0,
	blue_mask = 0x1F
} RGB565_BITMASK;

typedef struct 
{ 
    u16   bfType; 
    u32  bfSize; 
    u16   bfReserved1; 
    u16   bfReserved2; 
    u32  bfOffBits; 
} _BITMAPFILEHEADER; 

typedef struct
{
    u32  biSize; 
    s32   biWidth; 
    s32   biHeight; 
    u16   biPlanes; 
    u16   biBitCount; 
    u32  biCompression; 
    u32  biSizeImage; 
    s32   biXPelsPerMeter; 
    s32   biYPelsPerMeter; 
    u32  biClrUsed; 
    u32  biClrImportant; 
} _BITMAPINFOHEADER; 

typedef struct
{
    u8  rgbtBlue;
    u8  rgbtGreen;
    u8  rgbtRed;
} _RGBTRIPLE;
