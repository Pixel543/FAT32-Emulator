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

void read_sector(uint32_t sector_num, void* buffer) {
    lseek(disk_fd, sector_num * SECTOR_SIZE, SEEK_SET);
    read(disk_fd, buffer, SECTOR_SIZE);
}

void write_sector(uint32_t sector_num, const void* buffer) {
    lseek(disk_fd, sector_num * SECTOR_SIZE, SEEK_SET);
    write(disk_fd, buffer, SECTOR_SIZE);
}

uint32_t cluster_to_sector(uint32_t cluster) {
    uint32_t first_data_sector = boot_sector.BPB_RsvdSecCnt + (boot_sector.BPB_NumFATs * boot_sector.BPB_FATSz32);
    return ((cluster - 2) * boot_sector.BPB_SecPerClus) + first_data_sector;
}

int validate_fs() {
    read_sector(0, &boot_sector);
    if (boot_sector.signature == 0xAA55 && strncmp((char*)boot_sector.BS_FilSysType, "FAT32", 5) == 0) {
        return 1;
    }
    return 0;
}

// --- Реализация команд ---

void format_disk() 
{
    memset(&boot_sector, 0, sizeof(FAT32BootSector));

    boot_sector.BS_jmpBoot[0] = 0xEB;
    boot_sector.BS_jmpBoot[1] = 0x58;
    boot_sector.BS_jmpBoot[2] = 0x90;
    memcpy(boot_sector.BS_OEMName, "MSWIN4.1", 8);
    boot_sector.BPB_BytsPerSec = SECTOR_SIZE;
    boot_sector.BPB_SecPerClus = 8; 
    boot_sector.BPB_RsvdSecCnt = 32;
    boot_sector.BPB_NumFATs = 2;
    boot_sector.BPB_RootEntCnt = 0; 
    boot_sector.BPB_TotSec16 = 0;  
    boot_sector.BPB_Media = 0xF8;
    boot_sector.BPB_FATSz16 = 0;   
    boot_sector.BPB_TotSec32 = DISK_SIZE_BYTES / SECTOR_SIZE;
    
    
    uint32_t total_clusters = boot_sector.BPB_TotSec32 / boot_sector.BPB_SecPerClus;
    boot_sector.BPB_FATSz32 = (total_clusters * 4 + SECTOR_SIZE - 1) / SECTOR_SIZE;

    boot_sector.BPB_RootClus = 2;
    boot_sector.BPB_FSInfo = 1;
    boot_sector.BS_BootSig = 0x29;
    boot_sector.BS_VolID = (uint32_t)time(NULL);
    memcpy(boot_sector.BS_VolLab, "NO NAME    ", 11);
    memcpy(boot_sector.BS_FilSysType, "FAT32   ", 8);
    boot_sector.signature = 0xAA55;

 
    write_sector(0, &boot_sector);


    char sector_buffer[SECTOR_SIZE] = {0};
    for (uint32_t i = 0; i < boot_sector.BPB_FATSz32 * boot_sector.BPB_NumFATs; ++i) 
    {
        write_sector(boot_sector.BPB_RsvdSecCnt + i, sector_buffer);
    }


    uint32_t fat_buffer[SECTOR_SIZE / sizeof(uint32_t)];
    memset(fat_buffer, 0, SECTOR_SIZE);
    fat_buffer[0] = 0x0FFFFFF8; 
    fat_buffer[1] = 0x0FFFFFFF; 
    fat_buffer[2] = 0x0FFFFFFF; 
    write_sector(boot_sector.BPB_RsvdSecCnt, fat_buffer);
    write_sector(boot_sector.BPB_RsvdSecCnt + boot_sector.BPB_FATSz32, fat_buffer); 


    uint32_t root_dir_sector = cluster_to_sector(boot_sector.BPB_RootClus);
    for (int i = 0; i < boot_sector.BPB_SecPerClus; ++i) 
    {
        write_sector(root_dir_sector + i, sector_buffer);
    }
    
    fs_valid = validate_fs();
    current_cluster = boot_sector.BPB_RootClus;
    strcpy(current_path, "/");
    printf("Ok\n");
}


void to_dos_filename(const char* filename, char* dos_filename) 
{
    memset(dos_filename, ' ', 11);
    char* dot = strrchr(filename, '.');
    int name_len, ext_len;

    if (dot) 
    {
        name_len = dot - filename;
        ext_len = strlen(filename) - name_len - 1;
        strncpy(dos_filename, filename, name_len > 8 ? 8 : name_len);
        strncpy(dos_filename + 8, dot + 1, ext_len > 3 ? 3 : ext_len);
    } else {
        name_len = strlen(filename);
        strncpy(dos_filename, filename, name_len > 8 ? 8 : name_len);
    }

    for (int i = 0; i < 11; ++i) {dos_filename[i] = toupper(dos_filename[i]);}
}


void ls_command(const char* path) {
    uint32_t dir_cluster_to_list = current_cluster;
    if (path) 
    {
        dir_cluster_to_list = find_path_cluster(path);
        if (dir_cluster_to_list == 0) 
        {
            fprintf(stderr, "Error: Path not found: %s\n", path);
            return;
        }
    }
    
    FAT32DirectoryEntry dir_entries[SECTOR_SIZE / sizeof(FAT32DirectoryEntry)];
    uint32_t current_sector = cluster_to_sector(dir_cluster_to_list);

    for (int s = 0; s < boot_sector.BPB_SecPerClus; ++s) 
    {
        read_sector(current_sector + s, dir_entries);
        for (int i = 0; i < SECTOR_SIZE / sizeof(FAT32DirectoryEntry); ++i) 
        {
            if (dir_entries[i].DIR_Name[0] == 0x00) return; 
            if (dir_entries[i].DIR_Name[0] == 0xE5) continue; 
            if (dir_entries[i].DIR_Attr == 0x0F) continue; 

            char name[13] = {0};
            int k = 0;
            for(int j=0; j<8; j++) 
            {if (dir_entries[i].DIR_Name[j] != ' ') name[k++] = dir_entries[i].DIR_Name[j];}

            if (dir_entries[i].DIR_Name[8] != ' ') 
            {
                name[k++] = '.';
                for(int j=8; j<11; j++) {if (dir_entries[i].DIR_Name[j] != ' ') name[k++] = dir_entries[i].DIR_Name[j];}
            }

            if(dir_entries[i].DIR_Attr & 0x10) printf("<DIR>  %s\n", name);
            else printf("       %s\n", name);
        }
    }
}


uint32_t find_path_cluster(const char* path) 
{
    if (strcmp(path, "/") == 0) {return boot_sector.BPB_RootClus;}
    
    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';
    
    uint32_t search_cluster = boot_sector.BPB_RootClus; 
    if (path[0] != '/') {search_cluster = current_cluster;}

    char* token = strtok(path_copy, "/");
    while (token) 
    {
        FAT32DirectoryEntry dir_entries[SECTOR_SIZE / sizeof(FAT32DirectoryEntry)];
        int found = 0;
        char dos_name[12];
        to_dos_filename(token, dos_name);

        for(int s=0; s < boot_sector.BPB_SecPerClus; s++) 
        {
            read_sector(cluster_to_sector(search_cluster) + s, dir_entries);
            for(int i=0; i < SECTOR_SIZE / sizeof(FAT32DirectoryEntry); i++) 
            {
                if (dir_entries[i].DIR_Name[0] == 0x00) break;
                if (memcmp(dir_entries[i].DIR_Name, dos_name, 11) == 0 && (dir_entries[i].DIR_Attr & 0x10)) 
                {
                    search_cluster = (dir_entries[i].DIR_FstClusHI << 16) | dir_entries[i].DIR_FstClusLO;
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) return 0; 
        token = strtok(NULL, "/");
    }
    return search_cluster;
}


void cd_command(const char* path) 
{
    if (strcmp(path, "..") == 0) 
    {

       if (strcmp(current_path, "/") != 0) 
       {
            strcpy(current_path, "/");
            current_cluster = boot_sector.BPB_RootClus;
       }
       return;
    }

    uint32_t target_cluster = find_path_cluster(path);
    if (target_cluster == 0) 
    {
        fprintf(stderr, "Error: Directory not found.\n");
    } else {
        current_cluster = target_cluster;
        strncpy(current_path, path, sizeof(current_path) - 1);
       
        if (strcmp(current_path, "/") != 0 && current_path[strlen(current_path)-1] != '/') 
        {strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);}
    }
}


uint32_t find_free_cluster() 
{
    uint32_t fat_buffer[SECTOR_SIZE / sizeof(uint32_t)];
    for (uint32_t s = 0; s < boot_sector.BPB_FATSz32; ++s) 
    {
        read_sector(boot_sector.BPB_RsvdSecCnt + s, fat_buffer);
        for (uint32_t i = 0; i < SECTOR_SIZE / 4; ++i) 
        {
            uint32_t cluster_num = s * (SECTOR_SIZE / 4) + i;
            if (cluster_num < 2) continue;
            if (fat_buffer[i] == 0x00000000) {return cluster_num;}
        }
    }
    return 0; 
}

int find_free_directory_entry(uint32_t dir_cluster, FAT32DirectoryEntry* new_entry) 
{
    FAT32DirectoryEntry dir_entries[SECTOR_SIZE / sizeof(FAT32DirectoryEntry)];
    uint32_t dir_sector = cluster_to_sector(dir_cluster);

    for (int s = 0; s < boot_sector.BPB_SecPerClus; ++s) 
    {
        read_sector(dir_sector + s, dir_entries);
        for (int i = 0; i < SECTOR_SIZE / sizeof(FAT32DirectoryEntry); ++i) 
        {
            if (dir_entries[i].DIR_Name[0] == 0x00 || dir_entries[i].DIR_Name[0] == 0xE5) 
            {
                memcpy(&dir_entries[i], new_entry, sizeof(FAT32DirectoryEntry));
                write_sector(dir_sector + s, dir_entries);
                return 1;
            }
        }
    }
    return 0; 
}

void update_fat(uint32_t cluster, uint32_t value) 
{
    uint32_t fat_sector = boot_sector.BPB_RsvdSecCnt + (cluster * 4 / SECTOR_SIZE);
    uint32_t fat_offset = (cluster * 4) % SECTOR_SIZE;
    uint32_t fat_buffer[SECTOR_SIZE/4];
    
    read_sector(fat_sector, fat_buffer);
    fat_buffer[fat_offset/4] = value;
    write_sector(fat_sector, fat_buffer);

  
    fat_sector += boot_sector.BPB_FATSz32;
    write_sector(fat_sector, fat_buffer);
}

void mkdir_command(const char* name) 
{
    FAT32DirectoryEntry new_dir;
    to_dos_filename(name, (char*)new_dir.DIR_Name);
    
    uint32_t new_cluster = find_free_cluster();
    if (new_cluster == 0) {
        fprintf(stderr, "Error: No free space on disk.\n");
        return;
    }
    
    update_fat(new_cluster, 0x0FFFFFFF); 

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    uint16_t fat_time = (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2);
    uint16_t fat_date = ((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday;
    
    new_dir.DIR_Attr = 0x10; 
    new_dir.DIR_NTRes = 0;
    new_dir.DIR_CrtTimeTenth = 0;
    new_dir.DIR_CrtTime = fat_time;
    new_dir.DIR_CrtDate = fat_date;
    new_dir.DIR_LstAccDate = fat_date;
    new_dir.DIR_WrtTime = fat_time;
    new_dir.DIR_WrtDate = fat_date;
    new_dir.DIR_FstClusHI = (new_cluster >> 16) & 0xFFFF;
    new_dir.DIR_FstClusLO = new_cluster & 0xFFFF;
    new_dir.DIR_FileSize = 0;

    if (!find_free_directory_entry(current_cluster, &new_dir)) 
    {
        fprintf(stderr, "Error: Could not create directory, parent directory is full.\n");
        update_fat(new_cluster, 0); 
        return;
    }
    
    
    FAT32DirectoryEntry dot_entries[2];
    
    memcpy(dot_entries[0].DIR_Name, ".          ", 11);
    dot_entries[0].DIR_Attr = 0x10;
    dot_entries[0].DIR_FstClusHI = (new_cluster >> 16) & 0xFFFF;
    dot_entries[0].DIR_FstClusLO = new_cluster & 0xFFFF;

    memcpy(dot_entries[1].DIR_Name, "..         ", 11);
    dot_entries[1].DIR_Attr = 0x10;
    dot_entries[1].DIR_FstClusHI = (current_cluster >> 16) & 0xFFFF;
    dot_entries[1].DIR_FstClusLO = current_cluster & 0xFFFF;
    
    char sector_buffer[SECTOR_SIZE] = {0};
    memcpy(sector_buffer, &dot_entries, 2 * sizeof(FAT32DirectoryEntry));
    write_sector(cluster_to_sector(new_cluster), sector_buffer);
    
    printf("Ok\n");
}

void touch_command(const char* name) 
{
    FAT32DirectoryEntry new_file;
    to_dos_filename(name, (char*)new_file.DIR_Name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    uint16_t fat_time = (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2);
    uint16_t fat_date = ((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday;
    
    new_file.DIR_Attr = 0x20; 
    new_file.DIR_NTRes = 0;
    new_file.DIR_CrtTimeTenth = 0;
    new_file.DIR_CrtTime = fat_time;
    new_file.DIR_CrtDate = fat_date;
    new_file.DIR_LstAccDate = fat_date;
    new_file.DIR_WrtTime = fat_time;
    new_file.DIR_WrtDate = fat_date;
    new_file.DIR_FstClusHI = 0;
    new_file.DIR_FstClusLO = 0;
    new_file.DIR_FileSize = 0;
    
    if (!find_free_directory_entry(current_cluster, &new_file)) 
    {
        fprintf(stderr, "Error: Could not create file, directory is full.\n");
        return;
    }
    
    printf("Ok\n");
}