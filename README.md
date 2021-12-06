# Como criar uma imagem fat32
## libs necessarias
### .mtools
### .mkfs
```
dd if=/dev/zero of=./disk.img bs=1M count=33
mkfs.vfat -F 32 -v ./disk.img
mcopy -i disk.img folder1 ::
mcopy -i disk.img folder2 ::

```

# Como Rodar

```
make
./main disk.img
```

# .As operações possíveis são
## .info (ver informações do disco)
## .pwd (ver arquivos no diretório corrente)
## .cd (mover para outro diretorio)

# Exemplo de uso
```
$ ./main disk.img
FatShell:[img/]$ info
Disk Label: NO NAME    
File system type: FAT32   
OEM name: mkfs.fat
Total sectors: 67584
Bytes per sector: 512
Sectors per cluster: 1
Bytes pr Cluster: 512
Sectors per Fat: 520
free Space: 33256kB

```

