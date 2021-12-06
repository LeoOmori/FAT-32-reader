#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <ctype.h>

// Estrutura do Boot Sector do fat32
struct fat32BS {
	char oem_name[8];
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
	char volume_label[11]; 
	char fat_type_label[8];
};

// Estrutura de diretorios da fat32
struct fat32Dir {
    char DIR_Name[12];
    uint8_t DIR_Attr;
    uint8_t unused1[4];
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
	uint16_t DIR_FirstClusterHigh;
	uint8_t unused2[4];
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
};

// Iniciar a estruta do boot loader
struct fat32BS bs;
// Variavel para guardar a imagem do disco
FILE *fd; 
// Variaveis para guardar as informacoes do boot sector
uint32_t sectors_per_cluster;
uint16_t byte_per_sector, byte_per_cluster;
FILE* infoFile, *listFile;

// Variavel para guardar a endereco do diretorio raiz
int32_t RootDirCluster = 0; 
// Variavel para guardar o endereco do diretorio atual
int32_t CurrentDirCluster = 0; 

// Array de diretorios
struct fat32Dir dir[16];

// Variavel para guardar o nome do diretorio
char CurrentDirName[100];

// funcao para printar informacoes do boot sector
void bsInfo() {
	// Primeiro setor de dados
    uint32_t FirstDataSector = bs.reserved_sector_count + (bs.table_count * bs.table_size_32);
	// Setor de dado
    uint32_t DataSec = bs.total_sectors_32 - FirstDataSector;
	// Contador de todos os clustes
    uint32_t CountofClusters = DataSec / bs.sectors_per_cluster;

	// Printar informacoes do boot sector
    printf("Disk Label: %.*s\n", 11, bs.volume_label);
	printf("File system type: %s\n", bs.fat_type_label);
	printf("OEM name: %s\n", bs.oem_name);
	printf("Total sectors: %d\n",bs.total_sectors_32);
	printf("Bytes per sector: %d\n",bs.bytes_per_sector);
	printf("Sectors per cluster: %d\n",bs.sectors_per_cluster);
	printf("Bytes pr Cluster: %d\n",bs.sectors_per_cluster*byte_per_sector);
	printf("Sectors per Fat: %d\n",bs.table_size_32);
    printf("free Space: %dkB\n", (CountofClusters * bs.sectors_per_cluster * bs.bytes_per_sector)/1024);
}

// FUncao para criar a estrura de um diretorio dado um endereco
void CreateDir(int DirAddr, struct fat32Dir* direct) {
	// ir ate o endereco do diretorio
    fseek(fd, DirAddr, SEEK_SET);
	// Loop para guardar os diretorios na struct de diretorio
    for( int i = 0 ; i < 16; i ++){
        fread(direct[i].DIR_Name, 1, 11, fd);
        direct[i].DIR_Name[11] = 0;
        fread(&direct[i].DIR_Attr, 1, 1, fd);
        fread(&direct[i].unused1, 1, 4, fd);
		fread(&direct[i].DIR_CrtTime, 1, 2, fd);
		fread(&direct[i].DIR_CrtDate, 1, 2, fd);
        fread(&direct[i].DIR_FirstClusterHigh, 2, 1, fd);
        fread(&direct[i].unused2, 1, 4, fd);
        fread(&direct[i].DIR_FstClusLO, 2, 1, fd);
        fread(&direct[i].DIR_FileSize, 4, 1, fd);
    }
}

// Funcao para implementar a operacao PWD
void listDir(struct fat32Dir* direct) {
	// Loop para listar todos os diretorios
	printf("Name:             Size:     CreatedAt:\n");
	for( int i = 0 ; i < 16; i ++){
		/***
		 * caso for um arquivo ou um diretorio:
		 * printar o nome do diretorio
		***/
		if(
			direct[i].DIR_Attr == 1 ||
			direct[i].DIR_Attr == 16 ||
			direct[i].DIR_Attr == 32
		){
			printf("%s       %d        %lu\n", direct[i].DIR_Name, direct[i].DIR_Attr, dir[i].DIR_CrtDate);
		}
	}
}

// retorna um endereco por um bloco logico
int LogicAddr(int32_t sector) {
	// Caso o setor for vazio, retornar diretorio raiz
    if(!sector)
        return RootDirCluster;
	// Retornar o endereco
    return (bs.bytes_per_sector * bs.reserved_sector_count) + ((sector - 2) * bs.bytes_per_sector) + (bs.bytes_per_sector * bs.table_count * bs.table_size_32);
}


// Funcao para implementar a operacao CD
void change_dir(char *path){
	// Variavel auxiliar para o nome
    char aux[15];
	// Token para quebrar "/" da string path
    char * token; 
	// flag para ver se achou o diretorio
    int match; 
	// caso a string path for 0
    if(strlen(path) == 0){
		printf("No argument!\n");
        return;
    }
    // Comeca separando o path em tokens
    token = strtok(path, "/");
	// loop enquanto exister tokens apos quebrar em "/"
    while(token != NULL){
        // Caso o token nao seja valido
        if(strlen(token) > 12){
            printf("Not found!\n");
            return;
        }
        // Transforma o nome para a variavel aux						       
        strcpy(aux, token);
		// Numero de espacos em branco na string
		int whitespace = 11 - strlen(aux);
		// Transformar tudo em uppercase 
		for(int i = 0; i < 12; i ++){
			aux[i] = toupper(aux[i]);
		}
		// Adicionar espaco em branco nas strings
		for(int j = 0; j < whitespace; j ++){
			strcat(aux, " ");
		} 
		// deixar setado em falso a flag de diretorio achado
        match = 0;
        // loop dos diretorios
		for(int i = 0; i < 16; i ++) {
        	// Checar caso exista o diretorio
            if(!strcmp(dir[i].DIR_Name, aux)){
				// Pegar o endereco do novo diretorio
                CurrentDirCluster = LogicAddr(dir[i].DIR_FstClusLO);
				// Criar a estruta de diretorios
                CreateDir(CurrentDirCluster, dir);
				// aumentar o contador de match
                match ++;
				strcpy(CurrentDirName, token);
				// sair do loop
                break;
            }
        }
        // Se nao achar o diretorio   
        if(!match){
            printf("Could not find directory!\n");
            break;
        }
        // Ir para o proximo token
        token = strtok(NULL, "/");
        if(token == NULL){
            break;
        }
    }
}

// funcao para printar em qual diretorio voce esta
void printShell(){
	printf("\033[0;32m");
	printf("FatShell:");
	printf("\033[0;34m");
	// caso seja o root printa "img/"
	if(RootDirCluster == CurrentDirCluster){
		printf("[img/]");
	// caso nao seja printar o diretorio atual	
	}else{
		printf("[%s/]", CurrentDirName);
	}
	printf("$ ");
	printf("\033[0m");
}

int main(int agrc, char *argc[]){
	// Abrir arquivo
	fd = fopen("disk.img", "r");
	if(!argc[1]){
		printf("A disk image is necessary!\n");
		exit(0);
	}
	// Caso o arquivo exista
	if(fd != NULL){
		// Ler o boot section
        fseek(fd, 3, SEEK_SET);
        fread(bs.oem_name, 1, 8, fd);
        fread(&bs.bytes_per_sector, 1, 2, fd);
        fread(&bs.sectors_per_cluster, 1, 1, fd);
        fread(&bs.reserved_sector_count, 1, 2, fd);
        fread(&bs.table_count, 1, 1, fd);
        fread(&bs.root_entry_count, 1, 2, fd);
        fseek(fd, 32, SEEK_SET);
		fread(&bs.total_sectors_32, 1, 4, fd);
        fread(&bs.table_size_32, 1, 4, fd);
        fseek(fd, 44, SEEK_SET);
        fread(&bs.root_cluster, 1, 4, fd);
        fseek(fd, 71, SEEK_SET);
        fread(bs.volume_label, 1, 11, fd); 
		fread(bs.fat_type_label, 1, 8, fd); 

		// Setores por cluster
		sectors_per_cluster = bs.sectors_per_cluster;
		// Bytes por setores
		byte_per_sector = bs.bytes_per_sector;
		// Bytes por clusters
		byte_per_cluster = bs.sectors_per_cluster*byte_per_sector;
		// Endereco do diretorio raiz
		RootDirCluster = (bs.table_count * bs.table_size_32 * bs.bytes_per_sector) + (bs.reserved_sector_count * bs.bytes_per_sector);
		// Endereco do diretorio atual comecando no diretorio root
		CurrentDirCluster = RootDirCluster;
		// Criar a estrutura de diretorio
		CreateDir(CurrentDirCluster,dir);
		// iniciar em root
		strcpy(CurrentDirName, "img/");
		// Main Loop
		while(1) {
			// Input do usuario
			char input[200];
			// caminho para um arquivo ou diretorio
			char path[200];
			// operacao feita no fat32
			char op[200];
			// Token para quebra de texto em " "
			char * token; 
			// prints do shell
			printShell();
			scanf(" %[^\n]", input);

			token = strtok(input, " ");	
			strcpy(op, token);
			token = strtok(NULL, " ");	
			if(token != NULL) {
				strcpy(path, token);
			}
			// Opcoes de operacoes
			if(!strcmp(op, "info")){
				bsInfo();
			}
			else if(!strcmp(op, "pwd")){
				listDir(dir);
			}
			else if(!strcmp(op, "cd")){
				change_dir(path);
            }
		}
	} else {
		printf("File dosen't exist or not passed as an argument!\n");
	}
	exit(0);
}
