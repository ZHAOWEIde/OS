#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#define MAGICNUM 0x2077

#define BLOCKNUM 126*32
#define INODENUM 32*32
#define FILE     0
#define DIR      1
#define DATABLOCKOFFSET  66
#define ROOTDIRBLOCK     2
#define TRUE      1
#define FALSE     0
#define INVALIDBLOCKNUM   0xffffffff
#define TYPE_CP     0
#define TYPE_LS     1
#define BOTH_INODE_DATA     0
#define ONLY_INODE          1
#define ONLY_DATA           2
#define SIZE_DIR_ITEM       128
//  gcc -g disk.h disk.c naiveExt2.c naiveExt2.h -o main


//结构体 128*4 +32*4 +4*4 = 164*4B = 656B
typedef struct super_block {
    int32_t magic_num; // 幻数， 用于识别文件系统
    int32_t free_block_count; // 空闲数据块数
    int32_t free_inode_count; // 空闲索引节点数
    int32_t dir_inode_count; // 被占用的目录inode数
    uint32_t block_map[128]; // 数据块占用位图，记录数据块的使用情况
    uint32_t inode_map[32]; // inode占用位图，记录索引节点表的使用情况
}super_block1;

// 24+4+4 = 32B
struct inode {
    uint32_t size; // 文件大小 
    uint16_t file_type; // 文件类型（文件/文件夹）
    uint16_t link; // 文件链接数
    uint32_t block_point[6]; // 数据块指针 存的是数据块的逻辑地址
};

//121+1+2+4 = 128B  一个block支持 4个，所以根目录最多24个dir_item
struct dir_item {
    uint32_t inode_id; // 当前目录项表示的文件/目录的对应inode
    uint16_t valid; // 当前目录项是否有效
    uint8_t type; // 当前目录项类型（文件/目录）
    char name[121]; // 目录项表示的文件/目录的文件名/目录名
}; 



//初始化超级块
int init_super_block();

//初始化根目录
void init_rootDirectory();

//删除所有文件
void del_all_file();


//初始化文件系统 
int init_naiveExt2();

//申请free inode \ data_block后写超级块
//type:用于inode是不是目录
//>option:用于控制是一起写inode、data_block;还是分别写
int write_super_block(int inode_num,int data_block_num,int type,int option);

//用于释放inode \data_block
void free_super_block(int inode_num,int data_block_num,int type,int option);

// 读入超级块
void read_super_block();

// 申请空闲inode
int apply_free_inode();

// 申请空闲数据block
int apply_free_block();

void read_inode(int inodeNum,char *inodeIn);
// 写inode所在的block
void write_inode(int inodeNum,struct inode inodeIn);

//要写inode对应某个文件的数据block
void write_inode_block(int inodeNum,struct dir_item dirItem);

//读取目录inode对应的数据block，数据将读到全局变量temp_buf;
void read_inode_block(int inodeNum,char *filePath);

// //读取目录inode对应的数据block，数据将返回一个存储地址;
// char* naive_read_inode_block(int inodeNum,char *filePath);

//查找对应目录Inode下的某个文件所在的inode
int find_directory_inode_Num(int inodeNum,char *fileName,int type);


//创建文件夹 
int mkdir(char *filePath);

//创建文件,同时通过参数返回dir_item
char* touch(char *filePath);

//展示文件夹;
int ls(char *filePath);

//处理路径
void str_hand(char *filePath,int *dirInode,char *temp,int *temp_index,int *isRoot);

//复制数据块
void copy_data_block(int beforeBlockNum,int nextBlockNum);

//复制inode与数据块
void copy_inode_data(int beforeInode,int nextInode);

//复制文件;
int copy(char *filePath,char *nextFilePath);

//通过文件名查找文件，返回文件路径;
void find(char *charName,char *filePath);

//工作环境
int work();


///
/// 第一个block存放了super_block  512B - 164B = 348B
/// 
/// 一共有8K个block  8K*512B = 4MB
/// 数据块大小？1kB
/// 128*32 = 4K个数据块    超级块+inode 已经占用了66个block所以 数据块数有 4k-33个 所以对齐32，数据块个数设为4k- 64个
/// inode 最多有 1k 个：32*32 = 1k个 一个最多6kB 当inode 满时 占用6kB 
/// 1k 个inode 大小为 32B 32K 占用 64个block 1个block有16个inode
/// 
                        
/// block       0~1    2 ~ 65               66 ~ 8k-128                    62
///         |_______|____|______________|________________________________|______|
///           超级块     inode             数据块                         未使用的
///                 |____|
///                 根目录的inode 默认放在inode 0  即 block 2
///                         根目录存储在 data_block 0,即 block 66 67
/// 数据block                               0 ~ 4k
/// 超级块偏移 0
/// inode 根目录 inode 0
