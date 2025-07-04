#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#define DISK_SIZE_MB 20
#define DISK_SIZE_BYTES (DISK_SIZE_MB * 1024 * 1024)
#define SECTOR_SIZE 512

#pragma pack(push, 1)

typedef struct 
{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
    uint8_t boot_code[420];
    uint16_t signature;
} FAT32BootSector;

typedef struct 
{
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} FAT32DirectoryEntry;

#pragma pack(pop)

int disk_fd = -1;
FAT32BootSector boot_sector;
char current_path[256] = "/";
uint32_t current_cluster;
int fs_valid = 0;

uint32_t cluster_to_sector(uint32_t cluster);
uint32_t find_free_cluster();
uint32_t find_path_cluster(const char* path);
void read_sector(uint32_t sector_num, void* buffer);
void write_sector(uint32_t sector_num, const void* buffer);
void format_disk();
void ls_command(const char* path);
void cd_command(const char* path);
void mkdir_command(const char* name);
void touch_command(const char* name);
void to_dos_filename(const char* filename, char* dos_filename);
int validate_fs();
int find_free_directory_entry(uint32_t dir_cluster, FAT32DirectoryEntry* new_entry);