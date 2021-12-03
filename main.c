#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>


#define O_RDWR           02
#define BUFFER_SIZE 32
#define ULONG unsigned long 

#define FAT_ADDR_SIZE 4
#define FILE_TOTAL_NAME_LENGTH 11
#define FILE_NAME_LENGTH 8
#define FILE_EXT_LENGTH 3
#define ENTRY_SIZE 32
#define DIR_CLUSTER_SIZE 4
#define END_OF_DIR 0x00
#define FILE_ATTR_BIT_OFFSET 11
#define KB_IN_BYTES 1021

unsigned char *buffer;
unsigned long *FAT;

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8 

//function prototype
void bsInfo(void);


/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8 

struct fat32BS_struct {
	char bootjmp[3];
	char oem_name[BS_OEMName_LENGTH];
	uint16_t bytes_per_sector; 
	uint8_t sectors_per_cluster; 
	uint16_t reserved_sector_count;
	uint8_t table_count; 
	uint16_t root_entry_count; 
	uint16_t total_sectors_16; 
	uint8_t media_type; 
	uint16_t table_size_16; 
	uint16_t sectors_per_track; 
	uint16_t head_side_count; 
	uint32_t hidden_sector_count; 
	uint32_t total_sectors_32; 
	uint32_t table_size_32; 
	uint16_t extended_flags; 
	uint8_t FSVerLow; 
	uint8_t FSVerHigh;
	uint32_t root_cluster;
	uint16_t fat_info;
	uint16_t backup_BS_sector; 
	char reserved_0[12]; 
	uint8_t drive_number; 
	uint8_t reserved_1; 
	uint8_t boot_signature; 
	uint32_t volume_id; 
	char volume_label[BS_VolLab_LENGTH]; 
	char fat_type_label[BS_FilSysType_LENGTH];
	// char BS_CodeReserved[420];
	// uint8_t BS_SigA; //0x55, sector[510]
	// uint8_t BS_SigB; //0xAA, sector[511]
};
typedef struct fat32BS_struct fat32BS;

/* dir entry constants */
#define DIR_Name_LENGTH 11
#define DIR_EMPTY 0xE5
#define DIR_LAST 0x00
#define DIR_ATTR_HIDDEN 0x02
#define DIR_ATTR_VOLUME_ID 0x08
#define DIR_ATTR_DIRECTORY 0x10

struct fat32Dir_struct {
    char DIR_Name[DIR_Name_LENGTH];
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
};

typedef struct fat32Dir_struct fat32Dir;

struct FSInfo_struct {
    uint32_t FSI_LeadSig;
    char FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    char FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
};

typedef struct FSInfo_struct FSInfo;



fat32BS bs;
int fd;
uint32_t sectors_per_cluster;
off_t fat_begin_lba, cluster_begin_lba;
uint16_t byte_per_sector, byte_per_cluster;
FILE* infoFile, *listFile;

int main(){
	fd = open("disk.img", O_RDONLY);
    if (fd == -1) {
        printf("Couldn't open image: .\n");
        return EXIT_FAILURE;
    }
	regBS();
	bsInfo();
	return 0;
}


void regBS(){
    read(fd, bs.bootjmp, sizeof(bs.bootjmp));
    read(fd, bs.oem_name, sizeof(bs.oem_name));
    read(fd, &bs.bytes_per_sector, sizeof(bs.bytes_per_sector));
    read(fd, &bs.sectors_per_cluster, sizeof(bs.sectors_per_cluster));
    read(fd, &bs.reserved_sector_count, sizeof(bs.reserved_sector_count));
    read(fd, &bs.table_count, sizeof(bs.table_count));
    read(fd, &bs.root_entry_count, sizeof(bs.root_entry_count));
    read(fd, &bs.total_sectors_16, sizeof(bs.total_sectors_16));
    read(fd, &bs.media_type, sizeof(bs.media_type));
    read(fd, &bs.table_size_16, sizeof(bs.table_size_16));
    read(fd, &bs.sectors_per_track, sizeof(bs.sectors_per_track));
    read(fd, &bs.head_side_count, sizeof(bs.head_side_count));
    read(fd, &bs.hidden_sector_count, sizeof(bs.hidden_sector_count));
    read(fd, &bs.total_sectors_32, sizeof(bs.total_sectors_32));
    read(fd, &bs.table_size_32, sizeof(bs.table_size_32));
    read(fd, &bs.extended_flags, sizeof(bs.extended_flags));
    read(fd, &bs.FSVerHigh, sizeof(bs.FSVerHigh));
    read(fd, &bs.FSVerLow, sizeof(bs.FSVerLow));
    read(fd, &bs.root_cluster, sizeof(bs.root_cluster));
    read(fd, &bs.fat_info, sizeof(bs.fat_info));
    read(fd, &bs.backup_BS_sector, sizeof(bs.backup_BS_sector));
    read(fd, bs.reserved_0, sizeof(bs.reserved_0));
    read(fd, &bs.drive_number, sizeof(bs.drive_number));
    read(fd, &bs.reserved_1, sizeof(bs.reserved_1));
    read(fd, &bs.boot_signature, sizeof(bs.boot_signature));
    read(fd, &bs.volume_id, sizeof(bs.volume_id));
    read(fd, bs.volume_label, sizeof(bs.volume_label));
    read(fd, bs.fat_type_label, sizeof(bs.fat_type_label));

    sectors_per_cluster = bs.sectors_per_cluster;
    cluster_begin_lba = bs.reserved_sector_count+(bs.table_size_32*bs.table_count);
    fat_begin_lba = bs.reserved_sector_count;
    byte_per_sector = bs.bytes_per_sector;
    byte_per_cluster = bs.sectors_per_cluster*byte_per_sector;
};


// funcao para printar informacoes do boot sector
void bsInfo() {
    uint32_t FirstDataSector = bs.reserved_sector_count + (bs.table_count * bs.table_size_32);
    uint32_t DataSec = bs.total_sectors_32 - FirstDataSector;
    uint32_t CountofClusters = DataSec / bs.sectors_per_cluster;
    //Drive name
    printf("Disk Label: %.*s\n", BS_VolLab_LENGTH, bs.volume_label);
	printf("File system type: %s\n", bs.fat_type_label);
	printf("OEM name: %s\n", bs.oem_name);
	printf("Total sectors: %d\n",bs.total_sectors_32);
	printf("Bytes per sector: %d\n",bs.bytes_per_sector);
	printf("Sectors per cluster: %d\n",bs.sectors_per_cluster);
	printf("Bytes pr Cluster: %d\n",bs.sectors_per_cluster*byte_per_sector);
	// todo
	printf("Sectors per Fat: %d\n",bs.table_size_32);
    printf("free Space: %dkB\n", (CountofClusters * bs.sectors_per_cluster * bs.bytes_per_sector)/1024);
}