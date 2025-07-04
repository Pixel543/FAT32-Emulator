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

int main(int args, char* argv[])
{
    if (args != 2)
    {
        fprintf(stderr, "Usage %s <disk file>\n", argv[0]);
        return 1;
    }
    
    const char* disk_path = argv[1];
    struct stat st;
    if (stat(disk_path, &st) != 0)
    {
        printf("Disk file not found. Creating a new one of %d MB.\n", DISK_SIZE_MB);
        disk_fd = open(disk_path, O_RDWR | O_CREAT, 0644);

        if (disk_fd == -1)
        {
            perror("Error creating disk file");
            return 1;
        }
    } else{
        
        disk_fd = open(disk_path, O_RDWR);
        
        if (disk_fd == -1)
        {
            perror("Error opening disk file");
            return 1;
        }
    }

    fs_valid = validate_fs();

    if (fs_valid)
    {
        current_cluster = boot_sector.BPB_RootClus;
    }

    char command_line[256];
    char* command;
    char* arg;

    while (1)
    {
        printf("%s> ", current_path);
        fflush(stdout);

        if (fgets(command_line, sizeof(command_line), stdin) == NULL) break;
        
        command_line[strcspn(command_line, "\n")] = 0; 
        command = strtok(command_line, " \t");

        if (!command) continue;

        arg = strtok(NULL, " \t");

        if (strcmp(command, "format") == 0) 
        {
            format_disk();
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            if (!fs_valid && strcmp(command, "format") != 0) 
            {
                fprintf(stderr, "Error: Unknown disk format. Please use 'format'.\n");
                continue;
            }

            if (strcmp(command, "ls") == 0) 
            {
                ls_command(arg);
            } else if (strcmp(command, "cd") == 0) {
                if (!arg) fprintf(stderr, "Usage: cd <path>\n");
                else cd_command(arg);
            } else if (strcmp(command, "mkdir") == 0) {
                if (!arg) fprintf(stderr, "Usage: mkdir <name>\n");
                else mkdir_command(arg);
            } else if (strcmp(command, "touch") == 0) {
                if (!arg) fprintf(stderr, "Usage: touch <name>\n");
                else touch_command(arg);
            } else {
                fprintf(stderr, "Unknown command: %s\n", command);
            }
        }
    }

    close(disk_fd);
    return 0;
}
