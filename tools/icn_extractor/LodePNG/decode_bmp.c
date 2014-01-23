/*
Copyright (c) 2005-2010 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/* 
	Adapted from example_bmp2png.cpp
	by Lode Vandevenne
	
	Retrieved from: http://lodev.org/lodepng/example_bmp2png.cpp
*/
#include "../types.h"

void DecodeBMPData(u8 *in, u8 *out, u16 width)
{
	u16 scanlineBytes = width * 3;
	
	for(u16 y = 0; y < width; y++){
		for(u16 x = 0; x < width; x++){
			u16 bmpos = (width - y - 1) * scanlineBytes + 3 * x;
			u16 newpos = 4 * y * width + 4 * x;
			out[newpos + 0] = in[bmpos + 2]; //R
			out[newpos + 1] = in[bmpos + 1]; //G
			out[newpos + 2] = in[bmpos + 0]; //B
			out[newpos + 3] = 255;           //A
		}
	}
	return;
}