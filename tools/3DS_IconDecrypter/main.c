#include "lib.h"

#define SMDH_SIZE 0x36c0
#define MAX_CACHE_NUM 360
#define CACHE_EXTDATA_OFFSET 0x18000 
#define CACHE_DAT_TID_LIST_SIZE 0x1680

typedef enum
{
	MAJOR = 0,
	MINOR = 3
} app_version;

typedef enum
{
	gen_xorpad = 0,
	dec_icondata = 1,
	read_cache_dat = 2,
} modes;

typedef enum
{
	CTR = 1,
	SRL,
	BAD_ICON,
} icon_type;

const static u8 SRL_ICON_MAGIC[8] = {0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21};
static u8 FF_bytes[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const static char SMDH_MAGIC[4] = {"SMDH"};

int xor_file(u8 *out, u8 *buff0, u8 *buff1, u32 size);
void app_title(void);
void help(char *app_name);
int CheckDecIcon(u8 *icon);

int ReadCacheDat(int argc, char *argv[]);
void GenBlankCacheDat(u8 *buffer);
u64 GetDecTitleID(u8 *enc_used, u8 *enc_blank);
void GetDecCacheDat(u8 *dec_used, u8 *enc_used, u8 *enc_blank);


int main(int argc, char *argv[])
{	
	if(argc < 2){
		printf("[!] Invalid Arguments (Not Enough Arguments)\n");
		help(argv[0]);
		return 1;
	}
	int mode = -1;
	// Deciding Mode of Operation
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i],"-g") == 0 || strcmp(argv[i],"--genxor") == 0){ 
			mode = gen_xorpad; 
			break;
		}
		else if(strcmp(argv[i],"-d") == 0 || strcmp(argv[i],"--decrypt") == 0){ 
			mode = dec_icondata; 
			break;
		}
		else if(strcmp(argv[i],"-t") == 0 || strcmp(argv[i],"--disp_tid") == 0){
			mode = read_cache_dat; 
			break;
		}
		else if(strcmp(argv[i],"-o") == 0 || strncmp(argv[i],"--dec_list=",11) == 0){
			mode = read_cache_dat; 
			break;
		}
	}
	if(mode == -1){
		printf("[!] Invalid Arguments (No Mode Specified)\n");
		help(argv[0]);
		return 1;
	}
	
	if(mode == read_cache_dat){
		return ReadCacheDat(argc,argv);
	}
	
	// Getting Current Working Directory
	char CWD[1024];
	memset(CWD,0,1024);
	getcwdir(CWD,1024);
	
	// Getting Data from args
	s32 pre_existing_icons = -1;
	s32 icons_to_crypt = -1;
	char *xor_dir = NULL;
	char *dec_icn_dir = NULL;
	char *cacheD_path = NULL;
	char xor_abs_dir[1024];
	memset(xor_abs_dir,0,1024);
	char dec_icn_abs_dir[1024];
	memset(dec_icn_abs_dir,0,1024);
	
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i],"-i") == 0 && dec_icn_dir == NULL && i < argc-1){ 
			dec_icn_dir = argv[i+1];
		}
		else if(strncmp(argv[i],"--decdata=",10) == 0 && dec_icn_dir == NULL){
			dec_icn_dir = (char*)(argv[i]+10);
		}
		else if(strcmp(argv[i],"-x") == 0 && xor_dir == NULL && i < argc-1){ 
			xor_dir = argv[i+1];
		}
		else if(strncmp(argv[i],"--xorpaddir=",12) == 0 && xor_dir == NULL){
			xor_dir = (char*)(argv[i]+12);
		}
		else if(strcmp(argv[i],"-c") == 0 && cacheD_path == NULL && i < argc-1){ 
			cacheD_path = argv[i+1];
		}
		else if(strncmp(argv[i],"--iconcache=",12) == 0 && cacheD_path == NULL){
			cacheD_path = (char*)(argv[i]+12);
		}
		else if(strcmp(argv[i],"-0") == 0 && pre_existing_icons == -1 && i < argc-1){ 
			pre_existing_icons = strtoul(argv[i+1],NULL,10);
		}
		else if(strncmp(argv[i],"--unused_slots=",15) == 0 && pre_existing_icons == -1){
			pre_existing_icons = strtoul((argv[i]+15),NULL,10);
		}
		else if(strcmp(argv[i],"-1") == 0 && icons_to_crypt == -1 && i < argc-1){ 
			icons_to_crypt = strtoul(argv[i+1],NULL,10);
		}
		else if(strncmp(argv[i],"--num_decrypt=",14) == 0 && icons_to_crypt == -1){
			icons_to_crypt = strtoul((argv[i]+14),NULL,10);
		}
	}
	
	// Sorting out bad input
	if(pre_existing_icons < 0 || pre_existing_icons > 360){
		printf("[!] Invalid input or no input, for option '-0'/'--unused_slots='\n");
		help(argv[0]);
		return 1;
	}
	if(icons_to_crypt < 1 || icons_to_crypt > 360){
		printf("[!] Invalid input or no input, for option '-1'/'--num_decrypt='\n");
		help(argv[0]);
		return 1;
	}
	if(dec_icn_dir == NULL){
		printf("[!] No Plaintext Icon directory was specified\n");
		help(argv[0]);
		return 1;
	}
	if(xor_dir == NULL){
		printf("[!] No XOR pad Directory was specified\n");
		help(argv[0]);
		return 1;
	}
	if(cacheD_path == NULL){
		printf("[!] No Encryption Icon Cache was specified\n");
		help(argv[0]);
		return 1;
	}
	// Opening Icon Cache File
	FILE *cacheD = fopen(cacheD_path,"rb");
	if(cacheD == NULL){
		printf("[!] Failed to open '%s'\n",cacheD_path);
		return 1;
	}
	
	// Allocating Buffers
	u8 *DecIconData = malloc(SMDH_SIZE);
	u8 *EncIconData = malloc(SMDH_SIZE);
	u8 *XORpad = malloc(SMDH_SIZE);
	if(DecIconData == NULL || EncIconData == NULL || XORpad == NULL){
		printf("[!] Memory Error\n");
		return 1;
	}
	
	// Getting Absolute Dir Paths for Icon Dir and XOR pad DIR
	if(mode == gen_xorpad) makedir(xor_dir);
	else if(mode == dec_icondata) makedir(dec_icn_dir);
	
	chdir(xor_dir);
	getcwdir(xor_abs_dir,1024);
	chdir(CWD);
	chdir(dec_icn_dir);
	getcwdir(dec_icn_abs_dir,1024);
	chdir(CWD);
	
	// Informing the user what the program will do
	if(mode == gen_xorpad) printf("[+] Generating XOR pad(s)\n");
	else if(mode == dec_icondata) printf("[+] Decrypting Icon Data\n");
	
	// Init File Pointers / Arrays / Values
	FILE *dec_icon_fp = NULL;
	FILE *xor_pad_fp = NULL;
	char deciconfile[100];
	char xorfile[100];
	u32 enc_icon_pos = 0;
	
	for(int i = pre_existing_icons; i < (pre_existing_icons+icons_to_crypt); i++){
		enc_icon_pos = CACHE_EXTDATA_OFFSET + SMDH_SIZE*i;
		
		// Getting File Names
		memset(&deciconfile,0,100);
		memset(&xorfile,0,100);
		sprintf(deciconfile,"icon_%d.icn",i);
		sprintf(xorfile,"icon_%d.xor",i);
		
		
		// Getting Enc Icon
		ReadFile_64(EncIconData,SMDH_SIZE,enc_icon_pos,cacheD);
		
		// Getting File Pointers and Storing Data
		if(mode == gen_xorpad){
			chdir(dec_icn_abs_dir);
			dec_icon_fp = fopen(deciconfile,"rb");
			if(dec_icon_fp == NULL){
				printf("[!] Failed To Open '%s'\n",deciconfile);
				return 1;
			}
			chdir(xor_abs_dir);
			xor_pad_fp = fopen(xorfile,"wb");
			if(xor_pad_fp == NULL){
				printf("[!] Failed To Create '%s'\n",xorfile);
				return 1;
			}
			
			// Storing Dec Icon
			ReadFile_64(DecIconData,SMDH_SIZE,0,dec_icon_fp);
			int icon_check = CheckDecIcon(DecIconData);
			if(icon_check == BAD_ICON) printf("[!] Caution '%s' does not appear to be an icon file, '%s' may be invalid\n",deciconfile,xorfile);
		}
		else if(mode == dec_icondata){
			chdir(dec_icn_abs_dir);
			dec_icon_fp = fopen(deciconfile,"wb");
			if(dec_icon_fp == NULL){
				printf("[!] Failed To Create '%s'\n",deciconfile);
				return 1;
			}
			chdir(xor_abs_dir);
			xor_pad_fp = fopen(xorfile,"rb");
			if(xor_pad_fp == NULL){
				printf("[!] Failed To Open '%s'\n",xorfile);
				return 1;
			}
			
			// Storing XOR pad
			ReadFile_64(XORpad,SMDH_SIZE,0,xor_pad_fp);
		}
		
		// Perform XOR crypto
		if(mode == gen_xorpad) xor_file(XORpad, DecIconData, EncIconData, SMDH_SIZE);
		else if(mode == dec_icondata){
			xor_file(DecIconData, XORpad, EncIconData, SMDH_SIZE);
			int icon_check = CheckDecIcon(DecIconData);
			if(icon_check == BAD_ICON) printf("[!] '%s' does not appear to have decrypted properly\n",deciconfile);
		}
		
		// Writing Output to File
		if(mode == gen_xorpad) WriteBuffer(XORpad, SMDH_SIZE, 0, xor_pad_fp);
		else if(mode == dec_icondata) WriteBuffer(DecIconData, SMDH_SIZE, 0, dec_icon_fp);
		
		fclose(dec_icon_fp);
		fclose(xor_pad_fp);
	}
	
	// Freeing Buffers and Icon Cache File Pointer
	free(EncIconData);
	free(DecIconData);
	free(XORpad);
	fclose(cacheD);
	
	printf("[*] Done\n");
	return 0;
}

int xor_file(u8 *out, u8 *buff0, u8 *buff1, u32 size)
{
	//memdump(stdout,"In  :",buff0,16);
	//memdump(stdout,"Out :",buff1,16);
	memset(out,0,size);
	for(u32 i = 0; i < size; i++){
		out[i] = (buff0[i] ^ buff1[i]);
	}	
	return 0;
}

int CheckDecIcon(u8 *icon)
{
	u8 *SRL_Check = (icon+0xc0);
	char *SMDH_Check = (char*)(icon);
	
	if(strncmp(SMDH_Check,SMDH_MAGIC,4) == 0) return CTR;
	if(memcmp(SRL_Check,SRL_ICON_MAGIC,8) == 0) return SRL;
	return BAD_ICON;
}

int ReadCacheDat(int argc, char *argv[])
{
	int Show_TID = False;
	char *enc_empty_path = NULL;
	char *enc_used_path = NULL;
	char *extract_path = NULL;
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i],"-b") == 0 && enc_empty_path == NULL && i < argc-1){ 
			enc_empty_path = argv[i+1];
		}
		else if(strncmp(argv[i],"--empty_image=",14) == 0 && enc_empty_path == NULL){
			enc_empty_path = (char*)(argv[i]+14);
		}
		else if(strcmp(argv[i],"-u") == 0 && enc_used_path == NULL && i < argc-1){ 
			enc_used_path = argv[i+1];
		}
		else if(strncmp(argv[i],"--used_image=",13) == 0 && enc_used_path == NULL){
			enc_used_path = (char*)(argv[i]+13);
		}
		else if(strcmp(argv[i],"-o") == 0 && extract_path == NULL && i < argc-1){ 
			extract_path = argv[i+1];
		}
		else if(strncmp(argv[i],"--dec_list=",11) == 0 && extract_path == NULL){
			extract_path = (char*)(argv[i]+11);
		}
		else if(strcmp(argv[i],"-t") == 0 || strcmp(argv[i],"--disp_tid") == 0){
			Show_TID = True; 
		}
	}
	
	if(enc_empty_path == NULL){
		printf("[!] Empty Title List not specified\n");
		return 1;
	}
	
	if(enc_used_path == NULL){
		printf("[!] Used Title List not specified\n");
		return 1;
	}
	
	if(extract_path == NULL && Show_TID == False){
		printf("[!] Nothing to do\n");
		return 1;
	}
	
	FILE *cache_empty_fp = fopen(enc_empty_path,"rb");
	FILE *cache_used_fp = fopen(enc_used_path,"rb");
	if(cache_empty_fp == NULL || cache_used_fp == NULL){
		printf("[!] Error opening input files\n");
		return 1;
	}
	u8 *cache_empty = malloc(CACHE_DAT_TID_LIST_SIZE);
	u8 *cache_used = malloc(CACHE_DAT_TID_LIST_SIZE);
	ReadFile_64(cache_empty,CACHE_DAT_TID_LIST_SIZE,0x4000,cache_empty_fp);
	ReadFile_64(cache_used,CACHE_DAT_TID_LIST_SIZE,0x4000,cache_used_fp);
	fclose(cache_empty_fp);
	fclose(cache_used_fp);
	
	if(extract_path != NULL){
		printf("[+] Decrypting Icon Cache List\n");
		FILE *dec_list_fp = fopen(extract_path,"wb");
		if(dec_list_fp == NULL){
			printf("[!] Failed to create '%s'\n",extract_path);
			return 1;
		}
		u8 *dec_cache_list = malloc(CACHE_DAT_TID_LIST_SIZE);
		GetDecCacheDat(dec_cache_list,cache_used,cache_empty);
		WriteBuffer(dec_cache_list,CACHE_DAT_TID_LIST_SIZE,0,dec_list_fp);
		free(dec_cache_list);
		fclose(dec_list_fp);
	}
	
	if(Show_TID == True){
		printf("[+] Displaying Title IDs for the Icon Cache Slots\n");
		int Active_Slots = 0;
		for(int i = 0; i < 360; i++){
			u8 *empty_pos = (cache_empty + 0x10*i + 8);
			u8 *used_pos = (cache_used + 0x10*i + 8);
			if(memcmp(empty_pos,used_pos,8) == 0){ break; }
			Active_Slots++;
		}
		printf(" There are %d icons in cache\n",Active_Slots);
		for(int i = 0; i < Active_Slots; i++){
			u8 *empty_pos = (cache_empty + 0x10*i + 8);
			u8 *used_pos = (cache_used + 0x10*i + 8);
			u64 TID = GetDecTitleID(used_pos,empty_pos);
			printf(" Icon %3d: %016llx\n",i,TID);
		}
	}
	printf("[*] Done\n");
	return 0;
}

u64 GetDecTitleID(u8 *enc_used, u8 *enc_blank)
{
	u8 xorpad[8];
	u8 decTID[8];
	xor_file(xorpad,FF_bytes,enc_blank,8);
	xor_file(decTID,enc_used,xorpad,8);
	return u8_to_u64(decTID,LE);
}

void GenBlankCacheDat(u8 *buffer)
{
	memset(buffer,0,CACHE_DAT_TID_LIST_SIZE);
	for(int i = 0; i < 360; i++){
		u8 *pos = (buffer + 0x10*i + 8);
		memset(pos,0xff,8);
	}
}

void GetDecCacheDat(u8 *dec_used, u8 *enc_used, u8 *enc_blank)
{
	u8 *DecBlankCacheDat = malloc(CACHE_DAT_TID_LIST_SIZE);
	GenBlankCacheDat(DecBlankCacheDat);
	u8 *XORpad = malloc(CACHE_DAT_TID_LIST_SIZE);
	xor_file(XORpad,DecBlankCacheDat,enc_blank,CACHE_DAT_TID_LIST_SIZE);
	xor_file(dec_used,enc_used,XORpad,CACHE_DAT_TID_LIST_SIZE);
}

void app_title(void)
{
	printf("CTR_Toolkit - Home Menu Icon Cache Decrypter\n");
	printf("Version %d.%d (C) 3DSGuy 2013\n",MAJOR,MINOR);
	
}

void help(char *app_name)
{
	app_title();
	printf("Usage: %s [options]\n", app_name);
	putchar('\n');
	printf("OPTIONS                 Possible Values       Explanation\n");
	printf(" -i, --decdata=         Directory             Specify Plaintext Icon Data directory\n");
	printf(" -x, --xorpaddir=       Directory             Specify XOR pad directory\n");
	printf(" -c, --iconcache=       File-in               Specify Encrypted Icon Cache\n");
	printf(" -0, --unused_slots=    Decimal Value         Specify Number of icons that exist before the one you want to decrypt\n");
	printf(" -1, --num_decrypt=     Decimal Value         Specify Number of icons to decrypt\n");
	printf(" -g, --genxor                                 Generate XOR pad(s)\n");
	printf(" -d, --decrypt                                Decrypt Icon Data using XOR pad(s)\n");
	printf("'Cache.dat' (image 00000004) Options:\n");
	printf(" -t, --disp_tid                               Display the Title IDs of the icons in the list\n");
	printf(" -b, --empty_image=     File-in               Specify Empty Encrypted 00000004 image\n");
	printf(" -u, --used_image=      File-in               Specify Used Encrypted 00000004 image\n");
	printf(" -o, --dec_list=        File-out              Decrypt Icon cache list from 00000004 image to file\n");
	
}