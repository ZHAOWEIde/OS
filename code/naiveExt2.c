#include "naiveExt2.h"
#include <string.h>
#define BLOCKNUM 128
#define INODENUM 32


struct super_block sp_block;

int write_block(unsigned int block_num,char* buf){
    
}

int init_super_block(){
    char temp_buf[2*DEVICE_BLOCK_SIZE];
    if(disk_read_block(0,temp_buf) == 0){
        int32_t magic_num = *((int32_t *)temp_buf);
        printf("note:read_magic_num is %#4x\n",magic_num);
        if(magic_num != MAGICNUM){//初始化
            sp_block.magic_num = MAGICNUM;
            sp_block.free_block_count = BLOCKNUM;
            sp_block.free_inode_count = INODENUM;
            sp_block.dir_inode_count = 0;
            for(int i = 0;i<BLOCKNUM;i++){
                sp_block.block_map[i] = 0;
                if(i<INODENUM){
                    sp_block.inode_map[i] = 0;
                }
            }
            int sp_block_size = sizeof(sp_block);
            printf("note:sp_block size is %d\n",sizeof(sp_block));
            printf("note:firsr magic num is %#4x\n",*((uint32_t *)&sp_block));
            memcpy(temp_buf,&sp_block,sp_block_size);
            for(int i = 0;i<32*4;i+=32){
                printf(":%#4x\n",*((int32_t *)(temp_buf+i)));
            }
            disk_write_block(0,temp_buf);
            disk_write_block(1,temp_buf + DEVICE_BLOCK_SIZE);
            printf("ok:first init super block\n");
        }
        else
        {  
            printf("ok:get super block ");
            memcpy(&sp_block,temp_buf,sizeof(sp_block));
            printf("magic_num:%#4x\n",sp_block.magic_num);
        }
        
    }
}

int init_naiveExt2(){
    if(open_disk() == 0 ){//打开磁盘
        printf("OK:open disk \n");
    }else printf("erro:cannot open disk \n");
}

int main(){
    init_naiveExt2();
    init_super_block();
    close_disk();
}