/**
Copyright 2013 3DSGuy

This file is part of extdata_tool.

extdata_tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

extdata_tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with extdata_tool. If not, see <http://www.gnu.org/licenses/>.
**/
#include "lib.h"
#include "aesmac.h"
#include "extdata.h"
#include "vsxe.h"
#include "titledb.h"
#include "ctr_crypto.h"


//Version
typedef enum
{
	MAJOR = 2,
	MINOR = 4
} app_version;

typedef enum
{
	image = 1,
	fs,
	generate
} program_action;

void app_title(void);
void help(char *app_name);

int main(int argc, char *argv[])
{
	if(argc < 2){
		help(argv[0]);
		return 1;
	}
	int action = 0;
	int i = 1;
	while(!action){
		if(i == argc) break; 
		if(strcmp(argv[i],"-i") == 0 || strncmp(argv[i],"--image=",8) == 0) action = image;
		if(strcmp(argv[i],"-d") == 0 || strncmp(argv[i],"--FSdir=",8) == 0) action = fs;
		if(strcmp(argv[i],"-g") == 0 || strncmp(argv[i],"--genextdata=",13) == 0) action = generate;
		i++;
	}	
	if(action == 0){
		printf("[!] Nothing to do\n");
		help(argv[0]);
		return ARG_ERROR;
	}
	
	if(action == image){
		ExtdataContext *extdata = malloc(sizeof(ExtdataContext));
		InitaliseExtdataContext(extdata);
		char *input = NULL;
		char *output = NULL;
		int DB_Mode = -1;
		u8 Extract = False;
		extdata->PrintInfo = False;
		extdata->Verbose = False;
		extdata->OverrideActiveDIFI = False;
		u8 result = 0;
		
		for(i = 1; i < argc; i++){
			if(strcmp(argv[i],"-i") == 0 && input == NULL && i < argc-1){
				input = argv[i+1];
			}
			else if(strncmp(argv[i],"--image=",8) == 0 && input == NULL){
				input = (char*)(argv[i]+8);
			}
			else if(strcmp(argv[i],"-x") == 0 && output == NULL && i < argc-1 && Extract == False){
				Extract = True;
				output = argv[i+1];
			}
			else if(strncmp(argv[i],"--extract=",10) == 0 && output == NULL && Extract == False){
				Extract = True;
				output = (char*)(argv[i]+10);
			}
			else if(strcmp(argv[i],"-a") == 0 && i < argc-1){
				extdata->OverrideActiveDIFI = True;
				extdata->DIFIPartition = strtol(argv[i+1],NULL,10);
			}
			else if(strncmp(argv[i],"--forcedifi=",12) == 0){
				extdata->OverrideActiveDIFI = True;
				extdata->DIFIPartition = strtol((argv[i]+12),NULL,10);
			}
			else if(strcmp(argv[i],"-p") == 0 || strcmp(argv[i],"--info") == 0){
				extdata->PrintInfo = True;
			}
			else if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0){
				extdata->Verbose = True;
			}
			else if(strcmp(argv[i],"-l") == 0 || strcmp(argv[i],"--listDB") == 0){
				DB_Mode = DB_Normal;
			}
			else if(strcmp(argv[i],"-0") == 0 || strcmp(argv[i],"--listTID") == 0){
				DB_Mode = DB_ByTID;
			}
		}
		
		if(input == NULL){
			printf("[!] No input extdata image was specified\n");
			FreeExtdataContext(extdata);
			help(argv[0]);
			return ARG_ERROR;
		}
		if(Extract == False && extdata->PrintInfo == False && extdata->Verbose == False && DB_Mode == -1){
			printf("[!] Nothing to do\n");
			FreeExtdataContext(extdata);
			help(argv[0]);
			return ARG_ERROR;
		}
		
		if(extdata->OverrideActiveDIFI && extdata->DIFIPartition != 0 && extdata->DIFIPartition != 1){
			printf("[!] Invalid DIFI partition: %d\n",extdata->DIFIPartition);
			FreeExtdataContext(extdata);
			help(argv[0]);
			return ARG_ERROR;
		}
		FILE *fp = fopen(input,"rb");
		if(fp == NULL){
			printf("[!] Failed to open '%s'\n",input);
			FreeExtdataContext(extdata);
			return IO_ERROR;
		}
		fclose(fp);
		extdata->extdata.size = GetFileSize_u64(input);
		extdata->extdata.buffer = malloc(extdata->extdata.size);
		if(extdata->extdata.buffer == NULL){
			printf("[!] Failed to allocate memory for extdata image\n");
			FreeExtdataContext(extdata);
			return MEM_ERROR;
		}
		fp = fopen(input,"rb");
		ReadFile_64(extdata->extdata.buffer,extdata->extdata.size,0,fp);
		fclose(fp);
		result = GetExtdataContext(extdata);
		if(result != 0){
			printf("[!] Failed to interprete extdata image (%d)\n",result);
			FreeExtdataContext(extdata);
			return 1;
		}
		
		if(extdata->Files.Count > 0){
			u8* embedded_data = extdata->extdata.buffer + extdata->Files.Data[0].offset;
		
			// Extracting Data
			if(extdata->Files.Count == 1 && Extract){
				fp = fopen(output,"wb");
				if(fp == NULL){
					printf("[!] Failed to create '%s'\n",output);
					FreeExtdataContext(extdata);
					return IO_ERROR;
				}
				if(extdata->Verbose) printf("[+] Writing data to '%s'\n",output);
				WriteBuffer(embedded_data,extdata->Files.Data[0].size,0,fp);
				fclose(fp);
			}
		
			// Displaying DB... if possible
			if(DB_Mode >= 0){
				if(extdata->ExtdataType == RawIVFC && extdata->Files.Count == 1 && IsTitleDB(embedded_data)){
					result = ProcessTitleDB(embedded_data,DB_Mode);
					if(result){
						printf("[!] Failed to read DB (%d)\n",result);
					}
				}
				else{
					printf("[!] No valid Title Database in extdata\n"); 
				}
			
			}
		}
		else{
			printf("[!] No data exists embedded in extdata\n"); 
		}
		FreeExtdataContext(extdata);
	}
	
	else if(action == fs){
		VSXEContext *vsxe_ctx = malloc(sizeof(VSXEContext));
		ExtdataContext *vsxe = malloc(sizeof(ExtdataContext));
		char *input = NULL;
		char *output = NULL;
		char cwd[IO_PATH_LEN];
		char VSXE_image_path[IO_PATH_LEN];
		memset(vsxe_ctx,0,sizeof(VSXEContext));
		memset(&cwd,0,IO_PATH_LEN);
		memset(&VSXE_image_path,0,IO_PATH_LEN);
		memset(vsxe,0,sizeof(ExtdataContext));
		
		if(getcwdir(cwd,IO_PATH_LEN) == NULL){
			printf("[!] Could not store Current Working Directory\n");
			return IO_ERROR;
		}
		
		
		for(i = 1; i < argc; i++){
			if(strcmp(argv[i],"-d") == 0 && input == NULL && i < argc-1){
				input = argv[i+1];
			}
			else if(strncmp(argv[i],"--FSdir=",8) == 0 && input == NULL){
				input = (char*)(argv[i]+8);
			}
			else if(strcmp(argv[i],"-x") == 0 && output == NULL && i < argc-1){
				vsxe_ctx->Flags[vsxe_extract] = True;
				output = argv[i+1];
			}
			else if(strncmp(argv[i],"--extractFS=",12) == 0 && output == NULL){
				vsxe_ctx->Flags[vsxe_extract] = True;
				output = (char*)(argv[i]+12);
			}
			else if(strcmp(argv[i],"-s") == 0 || strcmp(argv[i],"--showFS") == 0){
				vsxe_ctx->Flags[vsxe_show_fs] = True;
			}
			else if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0){
				vsxe_ctx->Flags[vsxe_verbose] = True;
			}
			else if(strcmp(argv[i],"-f") == 0 || strcmp(argv[i],"--FStable") == 0){
				vsxe_ctx->Flags[vsxe_fstable] = True;
			}
			else if(strcmp(argv[i],"-a") == 0 && i < argc-1){
				vsxe->OverrideActiveDIFI = True;
				vsxe->DIFIPartition = strtol(argv[i+1],NULL,10);
			}
			else if(strncmp(argv[i],"--forcedifi=",12) == 0){
				vsxe->OverrideActiveDIFI = True;
				vsxe->DIFIPartition = strtol((argv[i]+12),NULL,10);
			}
		}
		
		if(input == NULL){
			printf("[!] No Extdata FS directory was specified\n");
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return ARG_ERROR;
		}
		
		if(vsxe_ctx->Flags[vsxe_extract] && output == NULL){
			printf("[!] No Output directory was specified\n");
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return ARG_ERROR;
		}
		
		if(vsxe_ctx->Flags[vsxe_show_fs] == False && vsxe_ctx->Flags[vsxe_extract] == False && vsxe_ctx->Flags[vsxe_fstable] == False){
			printf("[!] Nothing to do\n");
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return ARG_ERROR;
		}
		
#ifdef _WIN32
		vsxe_ctx->platform = WIN_32;
#else
		vsxe_ctx->platform = UNIX;
#endif
		
		// Storing Input Directory
		vsxe_ctx->input = malloc(IO_PATH_LEN);
		if(vsxe_ctx->input == NULL){
			printf("[!] Failed to allocate memory for input path\n");
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return MEM_ERROR;
		}
		chdir(input);
		if(getcwdir(vsxe_ctx->input,IO_PATH_LEN) == NULL){
			printf("[!] Could not store input Directory\n");
			free(vsxe_ctx->input);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return IO_ERROR;
		}
		chdir(cwd);
		
		// Storing Output Directory
		vsxe_ctx->output = malloc(IO_PATH_LEN);
		if(vsxe_ctx->output == NULL){
			printf("[!] Failed to allocate memory for output path\n");
			free(vsxe_ctx->input);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return MEM_ERROR;
		}
		chdir(output);
		if(getcwdir(vsxe_ctx->output,IO_PATH_LEN) == NULL){
			printf("[!] Could not store output Directory\n");
			free(vsxe_ctx->input);
			free(vsxe_ctx->output);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return IO_ERROR;
		}
		chdir(cwd);
		
		// Getting path to VSXE FS Image
		sprintf(VSXE_image_path,"%s%c00000001.dec",vsxe_ctx->input,vsxe_ctx->platform);
		
		FILE *fp = fopen(VSXE_image_path,"rb");
		if(fp == NULL){
			printf("[!] Could not open '%s'\n",VSXE_image_path);
			free(vsxe_ctx->input);
			free(vsxe_ctx->output);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return IO_ERROR;
		}
		
		fclose(fp);
		vsxe->extdata.size = GetFileSize_u64(VSXE_image_path);
		vsxe->extdata.buffer = malloc(vsxe->extdata.size);
		if(vsxe->extdata.buffer == NULL){
			printf("[!] Failed to allocate memory for VSXE FST Image\n");
			free(vsxe_ctx->input);
			free(vsxe_ctx->output);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return MEM_ERROR;
		}
		
		// Getting VSXE FST Image
		fp = fopen(VSXE_image_path,"rb");
		ReadFile_64(vsxe->extdata.buffer,vsxe->extdata.size,0,fp);
		fclose(fp);
		
		u8 result = GetExtdataContext(vsxe);
		if(result){
			printf("[!] Failed to obtain VSXE FST from image '%s' (%d)\n",VSXE_image_path,result);
			free(vsxe_ctx->input);
			free(vsxe_ctx->output);
			free(vsxe_ctx);
			FreeExtdataContext(vsxe);
			return result;
		}
		
		if(vsxe->Files.Count > 0){
			u8 *vsxe_offset = vsxe->extdata.buffer + vsxe->Files.Data[0].offset;
			if(vsxe->ExtdataType == RawIVFC && vsxe->Files.Count == 1 && IsVSXEFileSystem(vsxe_offset)){
				vsxe_ctx->vsxe = vsxe_offset;
				result = ProcessExtData_FS(vsxe_ctx);
				if(result){
					printf("[!] Failed to read VSXE FST (%d)\n",result);
				}
			}
			else{
				printf("[!] No valid VSXE FST exists in '%s'\n",output); 
			}
		}
		else{
			printf("[!] No data exists embedded in extdata\n"); 
		}
		free(vsxe_ctx->input);
		free(vsxe_ctx->output);
		free(vsxe_ctx);
		FreeExtdataContext(vsxe);
	}
	
	else if(action == generate){
		// Extdata Gen
		COMPONENT_STRUCT sourcedata;
		memset(&sourcedata,0,sizeof(COMPONENT_STRUCT));
		COMPONENT_STRUCT outdata;
		memset(&outdata,0,sizeof(COMPONENT_STRUCT));
		
		char *input = NULL;
		char *output = NULL;
		char *extdatatype = NULL;
		u32 activeDIFI = 0;
		u8 type = 0;
		u8 Verbose = False;
		u8 UniqueExtdataID[8];
		memset(&UniqueExtdataID,0,8);
		
		u8 GenMAC = False;
		
		for(i = 1; i < argc; i++){
			if(strcmp(argv[i],"-1") == 0 || strcmp(argv[i],"-2") == 0 || strncmp(argv[i],"--keyX=",6) == 0 || strncmp(argv[i],"--keyY=",6) == 0){
				GenMAC = True;
			}
			else if(strcmp(argv[i],"-g") == 0 && input == NULL && i < argc-1){
				input = argv[i+1];
			}
			else if(strncmp(argv[i],"--genextdata=",13) == 0 && input == NULL){
				input = (char*)(argv[i]+13);
			}
			else if(strcmp(argv[i],"-o") == 0 && output == NULL && i < argc-1){
				output = argv[i+1];
			}
			else if(strncmp(argv[i],"--outimage=",11) == 0 && output == NULL){
				output = (char*)(argv[i]+11);
			}
			else if(strcmp(argv[i],"-t") == 0 && extdatatype == NULL && i < argc-1){
				extdatatype = argv[i+1];
			}
			else if(strncmp(argv[i],"--type=",7) == 0 && extdatatype == NULL){
				extdatatype = (char*)(argv[i]+7);
			}
			else if(strcmp(argv[i],"-a") == 0 && i < argc-1){
				activeDIFI = strtol(argv[i+1],NULL,10);
			}
			else if(strncmp(argv[i],"--activedifi=",13) == 0){
				activeDIFI = strtol((argv[i]+13),NULL,10);
			}
			else if(strcmp(argv[i],"-u") == 0 && i < argc-1){
				if(strlen(argv[i+1]) == 16)
					char_to_u8_array(UniqueExtdataID,argv[i+1],8,BE,16);
				else
					printf("[!] Invalid length for Extdata Unique ID (expected 16 hex characters)\n"); 
			}
			else if(strncmp(argv[i],"--uniqueID=",11) == 0){
				if(strlen((argv[i]+11)) == 16)
					char_to_u8_array(UniqueExtdataID,(argv[i]+11),8,BE,16);
				else
					printf("[!] Invalid length for Extdata Unique ID (expected 16 hex characters)\n"); 
			}
			else if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0){
				Verbose = True;
			}
		}
		
		if(input == NULL){
			printf("[!] No input image was specified\n");
			help(argv[0]);
			return ARG_ERROR;
		}
		
		if(output == NULL){
			printf("[!] No output extdata image was specified\n");
			help(argv[0]);
			return ARG_ERROR;
		}
		
		if(extdatatype == NULL){
			printf("[!] No extdata type was specified\n");
			help(argv[0]);
			return ARG_ERROR;
		}
		
		// MAC Gen
		AESMAC_CTX *aes_ctx = malloc(sizeof(AESMAC_CTX));
		memset(aes_ctx,0,sizeof(AESMAC_CTX));
		u8 KeyX[0x10];
		u8 KeyY[0x10];
		memset(&KeyX,0,0x10);
		memset(&KeyY,0,0x10);
		
		aes_ctx->Verbose = Verbose;

		if(strcmp(extdatatype,"DATA") == 0){
			type = BUILD_DATA;
			aes_ctx->type = mac_extdata;	
		}
		else if(strcmp(extdatatype,"FS") == 0){
			type = BUILD_FS;
			aes_ctx->type = mac_extdata;	
		}
		else if(strcmp(extdatatype,"TDB") == 0){
			type = BUILD_DB;
			aes_ctx->type = mac_title_db;	
		}
		else if(strcmp(extdatatype,"IDB") == 0){
			type = BUILD_DB;
			aes_ctx->type = mac_import_db;	
		}
			
		if(type == 0){
			printf("[!] Invalid Extdata type '%s'\n",extdatatype);
			help(argv[0]);
			free(aes_ctx);
			return ARG_ERROR;
		}
			
		if(GenMAC){
			for(i = 1; i < argc; i++){
				if(strcmp(argv[i],"-1") == 0 && i < argc-1){
					if(strlen(argv[i+1]) == 32)
						char_to_u8_array(KeyX,argv[i+1],16,BE,16);
					else{
						printf("[!] Invalid length for KeyX (expected 32 hex characters)\n"); 
						help(argv[0]);
						free(aes_ctx);
						return ARG_ERROR;
					}
				}
				else if(strncmp(argv[i],"--keyX=",7) == 0){
					if(strlen((argv[i]+7)) == 32)
						char_to_u8_array(KeyX,(argv[i]+7),16,BE,16);
					else{
						printf("[!] Invalid length for KeyX (expected 32 hex characters)\n"); 
						help(argv[0]);
						free(aes_ctx);
						return ARG_ERROR;
					}
				}
				else if(strcmp(argv[i],"-2") == 0 && i < argc-1){
					if(strlen(argv[i+1]) == 32)
						char_to_u8_array(KeyY,argv[i+1],16,BE,16);
					else{
						printf("[!] Invalid length for KeyY (expected 32 hex characters)\n"); 
						help(argv[0]);
						free(aes_ctx);
						return ARG_ERROR;
					}
				}
				else if(strncmp(argv[i],"--keyY=",7) == 0){
					if(strlen((argv[i]+7)) == 32)
						char_to_u8_array(KeyY,(argv[i]+7),16,BE,16);
					else{
						printf("[!] Invalid length for KeyY (expected 32 hex characters)\n"); 
						help(argv[0]);
						free(aes_ctx);
						return ARG_ERROR;
					}
				}
				else if(strcmp(argv[i],"-3") == 0 && i < argc-1){
					u32_to_u8(aes_ctx->subdir_id,strtol(argv[i+1],NULL,16),BE);
				}
				else if(strncmp(argv[i],"--SubDirID=",11) == 0){
					u32_to_u8(aes_ctx->subdir_id,strtol((argv[i]+11),NULL,16),BE);
				}
				else if(strcmp(argv[i],"-4") == 0 && i < argc-1){
					u32_to_u8(aes_ctx->image_id,strtol(argv[i+1],NULL,16),BE);				
				}
				else if(strncmp(argv[i],"--ImageID=",10) == 0){
					u32_to_u8(aes_ctx->image_id,strtol((argv[i]+11),NULL,16),BE);
				}
				else if(strcmp(argv[i],"-5") == 0 || strcmp(argv[i],"--IsQuotaDat") == 0){
					aes_ctx->is_quote_dat = 1;
				}
			}
			if(aes_ctx->is_quote_dat == 1){
				memset(aes_ctx->image_id,0,4);
				memset(aes_ctx->subdir_id,0,4);
			}
		}
	
		//Extdata creation
		FILE *outfile = fopen(output,"wb");
		if(outfile == NULL){
			printf("[!] Failed to create '%s'\n",output);
			free(aes_ctx);
			return IO_ERROR;
		}
		FILE *infile = fopen(input,"rb");
		if(infile == NULL){
			printf("[!] Failed to open '%s'\n",input);
			free(aes_ctx);
			free(sourcedata.buffer);
			return IO_ERROR;
		}
		fclose(infile);
		sourcedata.size = GetFileSize_u64(input);
		sourcedata.buffer = malloc(sourcedata.size);
		if(sourcedata.buffer == NULL){
			printf("[!] Failed to allocate memory to generate Extdata\n");
			free(aes_ctx);
			return MEM_ERROR;
		}
		infile = fopen(input,"rb");
		ReadFile_64(sourcedata.buffer,sourcedata.size,0,infile);
		fclose(infile);
		
		u8 result = CreateExtdata(&outdata,&sourcedata,type,activeDIFI,UniqueExtdataID);
		if(result){
			free(aes_ctx);
			free(sourcedata.buffer);
			if(outdata.size > 0){ free(outdata.buffer); }
			return result;
		}
		if(GenMAC){
			if(Verbose) printf("[+] Generating AES MAC\n");
			memcpy(aes_ctx->header,(outdata.buffer+0x100),0x100);
			GenAESMAC(KeyX,KeyY,aes_ctx);
			memcpy(outdata.buffer,aes_ctx->aesmac,16);
		}
		if(Verbose) printf("[+] Writing '%s'\n",output);
		WriteBuffer(outdata.buffer,outdata.size,0,outfile);
		if(outdata.size > 0) free(outdata.buffer);
		if(sourcedata.size > 0)free(sourcedata.buffer);
		free(aes_ctx);
		fclose(outfile);
	}
	return 0;
}

void app_title(void)
{
	printf("CTR_Toolkit - Extra Data Tool\n");
	printf("Version %d.%d (C) 3DSGuy 2013\n",MAJOR,MINOR);
}

void help(char *app_name)
{
	app_title();
	printf("Usage: %s [options]\n", app_name);
	putchar('\n');
	printf("OPTIONS                 Possible Values       Explanation\n");
	printf(" -i, --image=           File-in               Specify input Extdata\n");
	printf(" -x, --extract=         File-out              Extract data from image\n");
	printf(" -a, --forcedifi=       '0'/'1'               Force reading Primary (0) or Secondary (1) DIFI\n");
	printf(" -p, --info                                   Print info\n");
	printf(" -v, --verbose                                Enable verbose output.\n");
	printf("Extdata (VSXE) File System Options:\n");
	printf(" -d, --FSdir=           Dir-in                Specify Extdata FS Directory\n");
	printf(" -x, --extractFS=       Dir-out               Extract VSXE File System\n");
	printf(" -s, --showFS                                 Display VSXE Extdata FS w/ Mount Points\n");
	printf(" -f, --FStable                                Display VSXE Folder and File Tables\n");
	printf("Extdata Title Database (BDRI) Options:\n");
	printf(" -l, --listDB                                 List the Titles in DB\n");
	printf(" -0, --listTID                                List the Title IDs of the titles in DB\n");
	printf("Extdata Creation Options:\n");
	printf(" -g, --genextdata=      File-in               Create an extdata image for a given image\n");
	printf(" -o, --outimage=        File-out              Output Extdata image\n");
	printf(" -t, --type=            DATA/FS/TDB/IDB       Specify Extdata Type\n");  
	printf(" -u, --uniqueID=        Value                 Specify Extdata Unique ID (16 hex characters)\n");
	printf(" -a, --activedifi=      '0'/'1'               Specify Active DIFI Partition\n");
	printf("AES MAC Options:\n");
	printf(" -1, --keyX=            Value                 Specify KeyX for MAC (32 hex characters)\n");
	printf(" -2, --keyY=            Value                 Specify KeyY for MAC (32 hex characters)\n");
	printf(" -3, --SubDirID=        Value                 Specify Extdata Sub directory ID (8 hex characters)\n");
	printf(" -4, --ImageID=         Value                 Specify Extdata Image ID (8 hex characters)\n");
	printf(" -5, --IsQuotaDat                             Specify if extdata will be Quota.dat?\n");
}
