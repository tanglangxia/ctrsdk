ctrdev
=============

Legal, open source, 3DS SDK. Compiles under Linux and Windows(MinGW).

## SDK ##

### ctrulib ###

CTR User Library by smealum (sema)

library for writing user mode arm11 code for the 3DS (CTR)

## Tools ##

### CTR_SDK Substitute Tools ###

1/ make_cia - Substitutes ctr_makecia32 - Generates CIA files

2/ make_banner - Substitutes ctr_makebanner32 - Generates ICN/BNR files

### CTR 'Extra Data' Tools ###

1/ extdata_tool - Read/Verify/Extract decrypted extdata. Generating/signing extdata is in an experimental phase.

2/ iconcache_tool - Extracts cached icon data from homemenu extdata

3/ 3DS_IconDecrypter - A tool to automate the process of decrypting icons from 3DS extdata.

### Other CTR Tools ###

1/ rom_tool - Tool for interpreting both debug/retail CCI files.

2/ make_cdn_cia - Repackages CDN content into a CIA file

3/ icn_extractor - Converts the embedded icon data in ICN files to PNG files

4/ 3DSExplorer - A fork of Eli Sherer's 3DSExplorer, with rom_tool's functionality added.

5/ decrypt - Decrypts savedata

6/ 3dsfuse - Encrypts savedata

6/ ctrtool

7/ ctr_ldr.py - a FIRM/NCCH Loader for IDA

8/ parentool - Parental Control Masterkey Generator
