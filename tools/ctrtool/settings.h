#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "types.h"
#include "keyset.h"
#include "filepath.h"

#define SECTION_FILE_ARG_BASE (20)
#define SECTION_NAME_ARG_BASE (28)

typedef struct
{
	keyset keys;
	filepath exefspath;
	filepath exefsdirpath;
	filepath exefssectionpath[8];
	char     exefssectionname[8][8];
	filepath firmdirpath;
	filepath romfspath;
	filepath romfsdirpath;
	filepath exheaderpath;
	filepath certspath;
	filepath contentpath;
	filepath tikpath;
	filepath tmdpath;
	filepath metapath;	
	filepath lzsspath;
	filepath wavpath;
	unsigned int mediaunitsize;
	int ignoreprogramid;
	int listromfs;
	u32 cwavloopcount;
} settings;

void settings_init(settings* usersettings);
filepath* settings_get_lzss_path(settings* usersettings);
filepath* settings_get_exefs_path(settings* usersettings);
filepath* settings_get_romfs_path(settings* usersettings);
filepath* settings_get_exheader_path(settings* usersettings);
filepath* settings_get_certs_path(settings* usersettings);
filepath* settings_get_tik_path(settings* usersettings);
filepath* settings_get_tmd_path(settings* usersettings);
filepath* settings_get_meta_path(settings* usersettings);
filepath* settings_get_content_path(settings* usersettings);
filepath* settings_get_exefs_dir_path(settings* usersettings);
filepath* settings_get_romfs_dir_path(settings* usersettings);
filepath* settings_get_firm_dir_path(settings* usersettings);
filepath* settings_get_wav_path(settings* usersettings);
filepath* settings_get_exefs_section_path(settings* usersettings, u32 n);
char* settings_get_exefs_section_name(settings* usersettings, u32 n);
unsigned int settings_get_mediaunit_size(settings* usersettings);
unsigned char* settings_get_ncch_key(settings* usersettings);
unsigned char* settings_get_ncch_fixedsystemkey(settings* usersettings);
unsigned char* settings_get_common_key(settings* usersettings);
int settings_get_ignore_programid(settings* usersettings);
int settings_get_list_romfs_files(settings* usersettings);
int settings_get_cwav_loopcount(settings* usersettings);

void settings_set_lzss_path(settings* usersettings, const char* path);
void settings_set_exefs_path(settings* usersettings, const char* path);
void settings_set_romfs_path(settings* usersettings, const char* path);
void settings_set_exheader_path(settings* usersettings, const char* path);
void settings_set_certs_path(settings* usersettings, const char* path);
void settings_set_tik_path(settings* usersettings, const char* path);
void settings_set_tmd_path(settings* usersettings, const char* path);
void settings_set_meta_path(settings* usersettings, const char* path);
void settings_set_content_path(settings* usersettings, const char* path);
void settings_set_exefs_dir_path(settings* usersettings, const char* path);
void settings_set_romfs_dir_path(settings* usersettings, const char* path);
void settings_set_firm_dir_path(settings* usersettings, const char* path);
void settings_set_wav_path(settings* usersettings, const char* path);
void settings_set_mediaunit_size(settings* usersettings, unsigned int size);
void settings_set_ignore_programid(settings* usersettings, int enable);
void settings_set_list_romfs_files(settings* usersettings, int enable);
void settings_set_cwav_loopcount(settings* usersettings, u32 loopcount);
void settings_set_exefs_section_path(settings* usersettings, u32 n, const char* path);
void settings_set_exefs_section_name(settings* usersettings, u32 n, const char* name);

#endif // _SETTINGS_H_
