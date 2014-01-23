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
typedef enum
{
	vsxe_magic = 0x56535845,
	vsxe_magic_id = 0x30000
} VSXE_data;

typedef enum
{
	VSXE_BAD_EXTDATA_TYPE = 20,
	VSXE_UNEXPECTED_DATA_IN_EXTDATA,
	VSXE_MEM_FAIL,
} VSXE_errors;

typedef enum
{
	vsxe_extract = 0,
	vsxe_show_fs,
	vsxe_verbose,
	vsxe_fstable,
} VSXE_OptionIndex;

typedef enum
{
	vsxe_fs_deleted = 0,
	vsxe_fs_modified = 2,
} vsxe_fs_file_action;

//Structs
typedef struct
{
	u8 parent_folder_index[4];
    char filename[0x10];
    u8 prev_folder_id[4];
    u8 last_folder_index[4]; 
    u8 last_file_index[4];
    u8 unk0[4]; 
    u8 unk1[4];
} folder_entry;

typedef struct
{
	u8 parent_folder_index[4];
    char filename[0x10];
    u8 prev_file_id[4];
    u8 unk0[4]; 
    u8 unk1[4];
    u8 unique_extdata_id[8];
    u8 unk2[4]; 
    u8 unk3[4];
} file_entry;

typedef struct
{
	//u8 unk0[40][4];
	u8 unk0[0x38];
	u8 folder_table_offset[8];
} vsxe_data_table;

typedef struct
{
	u8 magic[4];
	u8 magic_id[4];
	u8 data_table_offset[8];
	u8 filesizeX[8];
	u8 filesizeY[8];
	u8 unk1[8];
	u8 last_used_file_action[4];
	u8 unk2[4];
	u8 last_used_file_extdata_id[4];
	u8 unk3[4];
	char last_used_file[0x100];
} vsxe_header;

typedef struct
{
	u8 used_slots[4];
	u8 max_slots[4];
	u8 reserved[0x20];
} folder_table_header;

typedef struct
{
	u8 used_slots[4];
	u8 max_slots[4];
	u8 reserved[0x28];
} file_table_header;


typedef struct
{
	u8 *vsxe;

	u8 showfs;
	u8 showfs_tables;
	u8 verbose;
	
	// File I/O Paths
	char *input;
	char *output;
	char platform;
	
	// Header
	vsxe_header *header;
	vsxe_data_table *data_table;
	
	// Folders
	u64 folder_table_offset;
	u8 foldercount;
	folder_entry *folders;
	
	// Files
	u64 file_table_offset;
	u8 filecount;
	file_entry *files;
} VSXE_INTERNAL_CONTEXT;

typedef struct
{
	u8 *vsxe;
	
	u8 Flags[3];
	
	char *input;
	char *output;
	
	char platform;
} VSXEContext;

// Prototypes
int ProcessExtData_FS(VSXEContext *ctx);
int IsVSXEFileSystem(u8 *vsxe);