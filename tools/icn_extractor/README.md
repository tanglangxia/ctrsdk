#### CTR_Toolkit - ICN Icon Extractor - Takes the CTPK encoded icons from ICN files re-encoded them into PNG files ####
#### Version: 0.4 2013 (C) 3DSGuy ####

### Usage ###

Usage: icn_extractor <options>

OPTIONS                 Possible Values       Explanation
 -i                     File-in               Input ICN file.
 -s                     File-out              Output path for small icon.
 -l                     File-out              Output path for large icon.
 
This program supports "drag & drop" (i.e. icn_extractor <path to ICN>). However only the large icon is outputted, and is written to the same path as the input icon, except the file extension is changed to ".png".

### Credits ###
Big thanks to Lode Vandevenne who wrote the lodepng library, which I've used to handle bmp->png re-encoding.
You can check out lodepng here: http://lodev.org/lodepng/