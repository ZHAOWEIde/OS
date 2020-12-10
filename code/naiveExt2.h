#include "disk.h"
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#define MAGICNUM 0x2077


//结构体
typedef struct super_block {
    int32_t magic_num; // 幻数， 用于识别文件系统
    int32_t free_block_count; // 空闲数据块数
    int32_t free_inode_count; // 空闲索引节点数
    int32_t dir_inode_count; // 被占用的目录inode数
    uint32_t block_map[128]; // 数据块占用位图，记录数据块的使用情况
    uint32_t inode_map[32]; // inode占用位图，记录索引节点表的使用情况
} sp_block1;

struct inode {
    uint32_t size; // 文件大小
    uint16_t file_type; // 文件类型（文件/文件夹）
    uint16_t link; // 文件链接数
    uint32_t block_point[6]; // 数据块指针
};

struct dir_item {
    uint32_t inode_id; // 当前目录项表示的文件/目录的对应inode
    uint16_t valid; // 当前目录项是否有效
    uint8_t type; // 当前目录项类型（文件/目录）
    char name[121]; // 目录项表示的文件/目录的文件名/目录名
}; 

//初始化超级快
int init_super_block();

//初始化文件系统 
int init_naiveExt2();