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

#include "lib.h"
#include "bmp.h"
#include "LodePNG/lodepng.h"
#include "LodePNG/decode_bmp.h"

_RGBTRIPLE RGB565toRGB24bit(u16 pixel);
void CTPKtoBMP(u16 *in, u8 *out, s32 width);
int ConvertIcon(FILE *in, char *out, u32 offset, s32 width);
void DecodeBMPData(u8 *in, u8 *out, u16 width);

int main(int argc, char* argv[])
{
    // ensure proper usage
    if (argc < 2)
    {
		printf("ICN Icon Extractor v0.4 by 3DSGuy\n");
        printf("Usage: %s -i <ICN File> -l <Large Icon> -s <Small Icon>\n",argv[0]);
        return 1;
    }
	
	char *InputICN = NULL;
	char *SmallIcon = NULL;
	char *LargeIcon = NULL;
	
	if(argc == 2){ // Drag&Drop
		InputICN = argv[1]; 
		
		char outfile_large[1024];
		append_filextention(outfile_large,1024,argv[1],".png");
		LargeIcon = (char*)&outfile_large;
	}
	
	else{ // user has entered command line arguments
		for(int i = 1; i < argc-1; i++){
			if(strcmp(argv[i],"-i")==0 && argv[i+1][0] != '-') InputICN = argv[i+1];
			else if(strcmp(argv[i],"-l")==0 && argv[i+1][0] != '-') LargeIcon = argv[i+1];
			else if(strcmp(argv[i],"-s")==0 && argv[i+1][0] != '-') SmallIcon = argv[i+1];
		}
	}
	
	if(InputICN == NULL){
		fprintf(stderr, "No input ICN data specified.");
		return 1;
	}
	
	if(LargeIcon == NULL && SmallIcon == NULL){
		fprintf(stderr, "Nothing to do.");
		return 1;
	}

    FILE* icn = fopen(InputICN, "rb");
    if (icn == NULL)
    {
        printf("Could not open %s.\n", InputICN);
        return 2;
    }

	// Checking if ICN file is actually an ICN file
	char magic[4];
	fseek(icn,0,SEEK_SET);
	fread(&magic,4,1,icn);
	if(strncmp("SMDH",magic,4)!=0){
		fclose(icn);
		fprintf(stderr, "%s is not an ICN file", InputICN);
		return 1;
	}
	
	if(SmallIcon != NULL) ConvertIcon(icn,SmallIcon,0x2040,24);
    if(LargeIcon != NULL) ConvertIcon(icn,LargeIcon,0x24C0,48);
	
    // close infile
    fclose(icn);
	
    return 0;
}

int ConvertIcon(FILE *in, char *out, u32 offset, s32 width)
{
	// In and Out Buffers
	u32 ctpk_size = width*width*sizeof(u16);
	u32 bmp_encoded_buff_size = width*width*3;
	u32 bmp_decoded_buff_size = width*width*4;
	
	u16 *ctpk = malloc(ctpk_size);
	u8 *bmp_encoded = malloc(bmp_encoded_buff_size);
	u8 *bmp_decoded = malloc(bmp_decoded_buff_size);
	
	// Reading CTPK (CTR Texture Package)
	fseek(in,offset,SEEK_SET);
	fread(ctpk,ctpk_size,1,in);
	
	// Getting Decoding CTPK and encoding to 24-bit BMP data
	CTPKtoBMP(ctpk,bmp_encoded,width);
	free(ctpk);
	
	// Decoding BMP data
	DecodeBMPData(bmp_encoded,bmp_decoded,(u16)width);
	free(bmp_encoded);
	
	// Writing PNG
	unsigned result = lodepng_encode32_file(out, bmp_decoded, width, width);
	free(bmp_decoded);
	
	return (int)result;
}

void CTPKtoBMP(u16 *in, u8 *out, s32 width)
{
	if(width < 4) return; // Not supported
	
	s32 gap = (width*4);
	s32 num = (width*width*4) - gap;
	s32 num2 = 0;
	
	// CTPK pixel index
	u16 pixel = 0;
	
	// These modifications change the location of the pixels from the CTPK tile format, to a reverse linear 24-bit bitmap, when used with the below loop
	s32 mod[16] = {0,4,-gap,-gap+4,8,0xc,-gap+8,-gap+0xc,-gap-gap,-gap-gap+4,-gap-gap-gap,-gap-gap-gap+4,-gap-gap+8,-gap-gap+0xc,-gap-gap-gap+8,-gap-gap-gap+0xc};
	
	// Complicated translation of pixel location
	for(int i = 0; i < (width*width*4)/(8*gap); i++){
		num2 = num;
		for(int j = 0; j < (width/4); j++){
			for(int k = 0; k < 2; k++){
					for(int l = 0; l < 16; l++){
						// Calculating output offset for pixel
						s32 offset = num2+mod[l]; // 32-bit offset
						offset = (offset/4)*3; // Converting 32-bit offset to 24-bit offset
						// Converting pixel encoding
						_RGBTRIPLE tmp = RGB565toRGB24bit(in[pixel]);
						// Writing pixel to 'out'
						memcpy((out+offset),&tmp,3);
						// Incrementing CTPK pixel index
						pixel++;
					}
				num2 += 0x10;
			}
			if(j % 2) num2 += (4*gap);
			else num2 -= (4*gap)+0x20;
		}
		num -=8*gap;
	}
}

/* Adapted from http://msdn.microsoft.com/en-us/library/windows/desktop/dd390989%28v=vs.85%29.aspx */
_RGBTRIPLE RGB565toRGB24bit(u16 pixel)
{
	_RGBTRIPLE p;
	
	u8 red_value = (pixel & red_mask) >> 11;
	u8 green_value = (pixel & green_mask) >> 5;
	u8 blue_value = (pixel & blue_mask);
	
	// Expand to 8-bit values.
	p.rgbtRed   = red_value << 3;
	p.rgbtGreen = green_value << 2;
	p.rgbtBlue  = blue_value << 3;
	
	return p;
}