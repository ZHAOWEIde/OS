#include "naiveExt2.h"
#include <string.h>

static char divid[2] = {'/','\0'};
struct super_block sp_block;
char whitespace[] = " \t\r\n\v";
char currentPath[128] = "/";
char currentFileName[128] ="";

// static char temp_buf[2*DEVICE_BLOCK_SIZE];
char temp_buf[2*DEVICE_BLOCK_SIZE];

int init_super_block(){

    // temp_buf = (char *)calloc(2*DEVICE_BLOCK_SIZE,sizeof(char));
    if(disk_read_block(0,temp_buf) == 0){
        int32_t magic_num = *((int32_t *)temp_buf);
        printf("note:read_magic_num is %#4x\n",magic_num);
        
        if(magic_num != MAGICNUM){//初始化
            
            sp_block.magic_num = MAGICNUM;
            
            sp_block.free_block_count = BLOCKNUM;
            sp_block.free_inode_count = INODENUM;
            sp_block.dir_inode_count = 0;
            for(int i = 0;i<(BLOCKNUM/32);i++){
                sp_block.block_map[i] = 0;
                if(i<(INODENUM/32)){
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
            // printf("ok:first init super block\n");
            init_rootDirectory();
        }
        else
        {  
            // printf("ok:get super block \n");
            disk_read_block(1,temp_buf+DEVICE_BLOCK_SIZE);
            memcpy(&sp_block,temp_buf,sizeof(sp_block));
            // printf("ok:magic_num:%#4x\n",sp_block.magic_num);
            char temp_block[DEVICE_BLOCK_SIZE];
            struct inode * a;
            a = (struct inode *)temp_block;
            if(disk_read_block(ROOTDIRBLOCK,temp_block)==0){
                // printf("ok:get root directoty inode\n");
                printf("note: root_node_block[0]:%d\n",*(a->block_point));
            }
        }
        
    }
}

//
void del_all_file(){
    struct dir_item init;
    init.valid = FALSE;
    struct dir_item * path;
    for(int i = 0;i<BLOCKNUM;i++){
        for(int j =0;j<8;j++){
            if(disk_read_block(i*2+DATABLOCKOFFSET,temp_buf)==0
            &&disk_read_block(i*2+DATABLOCKOFFSET+1,temp_buf+DEVICE_BLOCK_SIZE)==0
            ){
                path =(struct dir_item*)(temp_buf+128*j);
                memcpy(path,&init,sizeof(init));
                disk_write_block(i*2+DATABLOCKOFFSET,temp_buf);
                disk_write_block(i*2+DATABLOCKOFFSET+1,temp_buf+DEVICE_BLOCK_SIZE);
            }
        }
    }
}
//不为幻数，第一次初始化
void init_rootDirectory(){
    char temp[DEVICE_BLOCK_SIZE];
    struct inode root_dir;
    root_dir.size = 0;
    root_dir.file_type = DIR;
    root_dir.link = 1;
    root_dir.block_point[0] = 0;
    del_all_file();
    for(int i =1;i<6;i++){
        root_dir.block_point[i] = INVALIDBLOCKNUM;
    }
    memcpy(temp,&root_dir,sizeof(root_dir));
    if(disk_write_block(ROOTDIRBLOCK,temp) == 0){
        // printf("ok:初始化根目录成功\n");
        write_super_block(0,0,DIR,BOTH_INODE_DATA);
    }
    // else printf("ok:初始化根目录失败\n");
}


int init_naiveExt2(){
    if(open_disk() == 0 ){//打开磁盘
        // printf("ok:open disk \n");
    }else {
        printf(RED"erro:cannot open disk \n");
        exit(1);
    }
}

void read_super_block(){
    char temp[2*DEVICE_BLOCK_SIZE];
    if(disk_read_block(0,temp) == 0
    &&(disk_read_block(1,temp+DEVICE_BLOCK_SIZE)==0)){
        memcpy(&sp_block,temp,sizeof(sp_block));
        // printf("ok:读入超级块成功！\n");

    }else{
        printf(RED"erro:读入超级块失败\n");
        exit(1);
    }
}

int write_super_block(int inode_num,int data_block_num,int type,int option){
    read_super_block();
    switch (option)
    {
    case BOTH_INODE_DATA:
        if(inode_num>=0 && inode_num < INODENUM
        && data_block_num >=0 && data_block_num < BLOCKNUM
            ){
            int inode_index = inode_num / 32;
            int offsetNum = inode_num % 32;
            int data_block_index = data_block_num/32;
            int offestDataNum = data_block_num % 32;
            
            if(((sp_block.inode_map[inode_index]>>31-offsetNum)& 0x1) ==0  
            && ((sp_block.block_map[data_block_index]>>31-offestDataNum)  & 0x1) == 0){
                sp_block.inode_map[inode_index] = sp_block.inode_map[inode_index] | (0x1 << 31-offsetNum);
                sp_block.block_map[data_block_index] = sp_block.block_map[data_block_index]|(0x1<<31-offestDataNum);
                sp_block.free_block_count--;
                sp_block.free_inode_count--;
                if(type == DIR) sp_block.dir_inode_count++;


                memcpy(temp_buf,&sp_block,sizeof(sp_block));
                disk_write_block(0,temp_buf);
                disk_write_block(1,temp_buf+DEVICE_BLOCK_SIZE);
                // printf("note:%#x\n",sp_block.inode_map[inode_index]);
                // printf("note:%#x\n",sp_block.block_map[data_block_index]);
                // printf("ok:successly update super block !\n");
                
            }else{
                printf(RED"erro:【写超级块】 inode 或 block 已被使用\n");
                exit(1);
            }
        }else{
            printf(RED"erro:【写超级块】 inode_num 或 block_num 错误\n");
            exit(1);
        }
        break;
    case ONLY_DATA:
        if(data_block_num >=0 && data_block_num < BLOCKNUM){
            int data_block_index = data_block_num/32;
            int offestDataNum = data_block_num % 32;

            printf("note:write before:%#x\n",sp_block.block_map[data_block_index]);
            uint32_t ans= (sp_block.block_map[data_block_index]>>31-offestDataNum)& 0x1;
            printf("note:block ans is: %#x\n",ans);
            
            if(((sp_block.block_map[data_block_index]>>31-offestDataNum)  & 0x1) == 0){
                sp_block.block_map[data_block_index] = sp_block.block_map[data_block_index]|(0x1<<31-offestDataNum);
                sp_block.free_block_count--;

                memcpy(temp_buf,&sp_block,sizeof(sp_block));
                disk_write_block(0,temp_buf);
                disk_write_block(1,temp_buf+DEVICE_BLOCK_SIZE);

                printf("note:%#x\n",sp_block.block_map[data_block_index]);
                // printf("ok:successly update super block !\n");
                
            }else{
                printf(RED"erro:【写超级块】  block 已被使用\n");
            }
        }else{
            printf(RED"erro:【写超级块】 block_num 错误\n");
        }
        break;

    default:
        break;
    }
    
}

int apply_free_inode(){
    read_super_block();
    for(int i = 0;i<(INODENUM/32);i++){
        for(int j = 31;j>=0;j--){
            if(!((sp_block.inode_map[i]>>j)&0x00000001)){
                int num = i*32 + 31-j;
                // if j = 30 
                // sp_block.inode_map[i] = sp_block.inode_map[i] | (0x00000001<<j);
                // printf("ok:find a free inode:%d\n",num);
                return num;
            }
        }
    }
    printf("warn:no free inode\n");
    return -1;
}

int apply_free_block(){
    read_super_block();
    for(int i = 0;i<(BLOCKNUM/32);i++){
        for(int j = 31;j>=0;j--){
            if(!((sp_block.block_map[i]>>j)&0x00000001)){
                int num = i*32 + 31-j;
                // sp_block.inode_map[i] = sp_block.inode_map[i] | (0x00000001<<j);
                // printf("ok:find a free data block:%d\n",num);
                return num;
            }
        }
    }
    printf("warn:no free block\n");
    return -1;
}

void read_inode(int inodeNum,char *inodeIn){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        char* path_inode = temp_buf + 32*offsetNum;
        memcpy(inodeIn, path_inode,32);
        // printf("ok:successly read inode to inodeIn\n");
    }
}

void write_inode(int inodeNum,struct inode inodeIn){
    
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        char* path_inode = temp_buf + 32*offsetNum;
        memcpy(path_inode, &inodeIn,sizeof(inodeIn));
        if(disk_write_block(blockNum,temp_buf)==0){
            // printf("ok:successly write inode\n");
        }
    }

}


int find_directory_inode_Num(int inodeNum,char *fileName,int type){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        struct inode* path_inode = (struct inode*)(temp_buf + 32*offsetNum);
        for(int i = 0;i<6;i++){
            uint32_t num = path_inode->block_point[i];
            if(num != INVALIDBLOCKNUM){
                char temp_buf2[2*DEVICE_BLOCK_SIZE];
                if(disk_read_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        struct dir_item * dirItem = (struct dir_item *)(temp_buf2+128*j);
                        if(type == DIR){
                            if(dirItem->valid && dirItem->type == DIR ){
                                if(strcmp(dirItem->name,fileName) == 0 )//>>>>
                                    return dirItem->inode_id;
                            }
                        }else if(type == FILE)
                        {
                            if(dirItem->valid && dirItem->type == FILE ){
                                if(strcmp(dirItem->name,fileName) == 0 )//>>>>
                                    return dirItem->inode_id;
                            }
                        }else if(type == BOTH_FILE_DIR){
                            if(dirItem->valid){
                                if(strcmp(dirItem->name,fileName) == 0 )//>>>>
                                    return dirItem->inode_id;
                                    
                            }
                        }
                    }

                }
            }

        }
        if(type== DIR){
            // printf(RED"erro:no diretory called: \"%s\" in this directory\n",fileName);
            return -1;
        }else if(type== FILE){
            // printf("erro:no FILE called: \"%s\" in this directory\n",fileName);
            return -1;
        }
    }

}

void write_inode_block(int inodeNum,struct dir_item dirItem){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        struct inode* path_inode = (struct inode*)(temp_buf + 32*offsetNum);
        for(int i = 0;i<6;i++){
            uint32_t num = path_inode->block_point[i];
            if(num!=INVALIDBLOCKNUM){
                char temp_buf2[2*DEVICE_BLOCK_SIZE];
                if(disk_read_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        struct dir_item * dir_item = (struct dir_item *)(temp_buf2+128*j);
                        if(dir_item->valid != TRUE){
                            memcpy(dir_item,&dirItem,sizeof(dirItem));
                            if(disk_write_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                                &&disk_write_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                            ){
                                // printf("ok:写入目录inode对应的数据block 成功\n");
                                return;
                            }
                        }
                    }
                }
            }
            if(i<5){
                int newBlockNum = apply_free_block();
                path_inode->block_point[i+1] = newBlockNum;
                write_super_block(INVALIDBLOCKNUM,newBlockNum,NULL,ONLY_DATA);
                write_inode(inodeNum,*path_inode);
            }
        }
        printf(RED"erro:no space for new file in this directory!\n");
        return;
    }
}



//创建文件夹
//修改对应路径下的索引
// 找到对应路径下的block 修改其中的dir_item项使得其链接上该inode
// /  mkdir /home/a/b/c
// / home /
//预分配一个数据块
int mkdir(char *filePath){
    //解析字符串得到名字、路径，
    //查看路径是否存在
    uint length = strlen(filePath);

    int isRootDivid = TRUE;
    // int write_temp =FALSE;
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char before_temp[121];
    char temp[121];
    char temp_index = 0;

    for(int i= 0;i<length;i++){
        
        if(filePath[i]=='/'){
            if(isRoot) isRoot = FALSE;
            else{
                //需要去处理 路径 
                //存在路径则继续
                
                dirInode = find_directory_inode_Num(dirInode,temp,DIR);
                //
                memcpy(before_temp,temp,temp_index);
                //初始化temp
                for(int j=0;j<temp_index;j++){
                    temp[j] = '\0';
                }
                temp_index = 0;
            }
        }
        else{

            temp[temp_index] = filePath[i];
            temp_index++;
            temp[temp_index] = '\0'; 
        }
    }
    if(dirInode >=0){
        if(find_directory_inode_Num(dirInode,temp,DIR) == -1  //当前目录找不到同名的文件、文件夹
        && find_directory_inode_Num(dirInode,temp,FILE)== -1  
        ){
            //最终处理
            int freeInodeNum = apply_free_inode();//申请一个空闲文件描述
            int freeBlockNum = apply_free_block();//申请一个数据块 

            struct dir_item dirItem;
            dirItem.inode_id = freeInodeNum;
            memcpy(dirItem.name,temp,temp_index+1);
            dirItem.type = DIR;
            dirItem.valid = TRUE;

            //要将dirItem写入目录inode对应某个文件的数据block
            write_inode_block(dirInode,dirItem);


            if(freeInodeNum!=-1 && freeBlockNum!=-1){
                struct inode free_inode;
                free_inode.size = 0;
                free_inode.file_type = DIR;
                free_inode.link = 1;
                for(int i = 0;i<6;i++){
                    free_inode.block_point[i] = INVALIDBLOCKNUM;
                }
                free_inode.block_point[0] = freeBlockNum;
                //写inode
                write_inode(freeInodeNum,free_inode);
                write_super_block(freeInodeNum,freeBlockNum,DIR,BOTH_INODE_DATA);
            }
        }else
        {
            fprintf(stderr,RED"erro:exist directory or file called\"%s\" in this directory\n"DARY_GRAY"OR\n"RED"directory erro\n",temp);
        
        }
    }else
    {
        fprintf(stderr,RED"erro:not exist directory:called\"%s\"\n",before_temp);
    }
    
    
}


char* touch(char *filePath){
    //解析字符串得到名字、路径，
    //查看路径是否存在
    uint length = strlen(filePath);
    int isRootDivid = TRUE;
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char before_temp[121];
    char temp[121];
    char temp_index = 0;
    for(int i= 0;i<length;i++){
        
        if(filePath[i]=='/'){
            if(isRoot) isRoot = FALSE;
            else{
                //需要去处理 路径 
                //存在路径则继续
                dirInode = find_directory_inode_Num(dirInode,temp,DIR);
                memcpy(before_temp,temp,temp_index);
                //初始化temp
                for(int j=0;j<temp_index;j++){
                    temp[j] = '\0';
                }
                temp_index = 0;
            }

        }
        else{

            temp[temp_index] = filePath[i];
            temp_index++;
            temp[temp_index] = '\0'; 
        }
    }

    if(dirInode >=0){
        if(find_directory_inode_Num(dirInode,temp,DIR) == -1 
        && find_directory_inode_Num(dirInode,temp,FILE)== -1
        ){
            //最终处理
            int freeInodeNum = apply_free_inode();//申请一个空闲文件描述
            int freeBlockNum = apply_free_block();//申请一个数据块 

            struct dir_item* dirItem = malloc(SIZE_DIR_ITEM);
            dirItem->inode_id = freeInodeNum;
            memcpy(dirItem->name,temp,temp_index+1);
            dirItem->type = FILE;
            dirItem->valid = TRUE;

            //要将dirItem写入目录inode对应某个文件的数据block
            write_inode_block(dirInode,*dirItem);


            if(freeInodeNum!=-1 && freeBlockNum!=-1){
                struct inode free_inode;
                free_inode.size = 0;
                free_inode.file_type = FILE;
                free_inode.link = 1;
                for(int i = 0;i<6;i++){
                    free_inode.block_point[i] = INVALIDBLOCKNUM;
                }
                free_inode.block_point[0] = freeBlockNum;
                //写inode
                write_inode(freeInodeNum,free_inode);
                write_super_block(freeInodeNum,freeBlockNum,FILE,BOTH_INODE_DATA);
            }
            return (char *)dirItem;
        }else
        {
            fprintf(stderr,RED"erro:exist directory or file called\"%s\" in this directory\n"DARY_GRAY"OR\n"RED"directory erro\n",temp);
        }
    }else{
        fprintf(stderr,RED"erro:not exist directory:called\"%s\"\n",before_temp);
    }
    
}


void read_inode_block(int inodeNum,char *filePath,int option){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        struct inode* path_inode = (struct inode*)(temp_buf + 32*offsetNum);
        for(int i = 0;i<6;i++){
            uint32_t num = path_inode->block_point[i];
            if(num!=INVALIDBLOCKNUM){
                char temp_buf2[2*DEVICE_BLOCK_SIZE];
                if(disk_read_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        struct dir_item * dir_item = (struct dir_item *)(temp_buf2+128*j);
                        if(dir_item->valid == TRUE){
                            
                            if(dir_item->type == DIR){
                                int str_len_filePath = strlen(filePath);
                                int str_len_dir_item = strlen(dir_item->name);
                                fprintf(stderr,">%s/%s  ",filePath,dir_item->name);
                                for(int i =0;i<LS_MAX_SIZE-str_len_dir_item-str_len_filePath;i++){
                                    fprintf(stderr,"-");
                                }
                                fprintf(stderr," directoy\n");
                                // printf(">%s/%s -----------directoy\n",filePath,dir_item->name);
                                char history[512];

                                strcpy(history,filePath);
                                strcat(history,divid);
                                strcat(history,dir_item->name);
                                // printf("note:history_path>>%s\n",history);
                                if(option == LS_RECURISON){
                                    read_inode_block(dir_item->inode_id,history,LS_RECURISON);
                                }
                            }else if(dir_item->type == FILE){
                                int str_len_filePath = strlen(filePath);
                                int str_len_dir_item = strlen(dir_item->name);
                                fprintf(stderr,">%s/%s  ",filePath,dir_item->name);
                                for(int i =0;i<LS_MAX_SIZE-str_len_dir_item-str_len_filePath;i++){
                                    fprintf(stderr,"-");
                                }
                                fprintf(stderr," file\n");
                                // printf(">%s/%s -----------file\n",filePath,dir_item->name);S
                            }
                        }
                    }
                }
            }
        }
    }
}

//给出一个文件路径，解析该路径的数据block的dir_item
//如果为空则列出所有
//eg: /hello/a  hello所在的inode   再找到a所在的inode
int ls(char *filePath,int option){
    int dirInode = 0;//上级目录所在的inode
    if(strcmp(filePath,"/")== 0 || strlen(filePath)==0 || strcmp(filePath,".")== 0 ){
        char temp[1] ={'\0'};
        read_inode_block(dirInode,temp,option);
    }else{
        uint length = strlen(filePath);
        int isRootDivid = TRUE;
        int isRoot = TRUE;
        char temp[121];
        char temp_index = 0;
        for(int i= 0;i<length;i++){
            if(filePath[i]=='/'){
                if(isRoot) isRoot = FALSE;
                else{
                    //需要去处理 路径 
                    //存在路径则继续
                    dirInode = find_directory_inode_Num(dirInode,temp,DIR);
                    //初始化temp
                    for(int j=0;j<temp_index;j++){
                        temp[j] = '\0';
                    }
                    temp_index = 0;
                }
            }
            else{

                temp[temp_index] = filePath[i];
                temp_index++;
                temp[temp_index] = '\0'; 
            }
        }
        dirInode = find_directory_inode_Num(dirInode,temp,DIR);
        read_inode_block(dirInode,filePath,option);
    }
}

///
///
///
void str_hand(char *filePath,int *dirInode,char *temp,int *temp_index,int *isRoot){
    uint length = strlen(filePath);
    for(int i= 0;i<length;i++){
        
        if(filePath[i]=='/'){
            if(*isRoot) *isRoot = FALSE;
            else{
                //需要去处理 路径 
                //存在路径则继续
                *dirInode = find_directory_inode_Num(*dirInode,temp,DIR);
                //初始化temp
                for(int j=0;j<*temp_index;j++){
                    temp[j] = '\0';
                }
                *temp_index = 0;
            }

        }
        else{

            temp[*temp_index] = filePath[i];
            (*temp_index)++;
            temp[*temp_index] = '\0'; 
        }
    }
}
void copy_data_block(int beforeBlockNum,int nextBlockNum){
    char temp[2*DEVICE_BLOCK_SIZE];
    if(disk_read_block(beforeBlockNum*2+DATABLOCKOFFSET,temp) == 0
    &&disk_read_block(beforeBlockNum*2+DATABLOCKOFFSET,temp+DEVICE_BLOCK_SIZE) ==0){
        if(disk_write_block(nextBlockNum*2+DATABLOCKOFFSET,temp) == 0
        &&disk_write_block(nextBlockNum*2+DATABLOCKOFFSET,temp+DEVICE_BLOCK_SIZE) ==0){
            // printf("ok:copy dataBlock!\n");
        }
    }
}

void copy_inode_data(int beforeInode,int nextInode){
    struct inode before_inodeIn;
    read_inode(beforeInode,(char *)&before_inodeIn);
    struct inode next_inodeIn;
    read_inode(nextInode,(char *)&next_inodeIn);
    int isNextHaveBlock = TRUE;
    uint32_t before_num;
    uint32_t next_num;
    int i=0;
    int before_block_num = 0;
    int next_block_num = 0;
    for(int k=0;k<6;k++){
        if(next_inodeIn.block_point[k]!=INVALIDBLOCKNUM) next_block_num++;
        if(before_inodeIn.block_point[k]!=INVALIDBLOCKNUM) before_block_num++;
    }

    for(;i<before_block_num;i++){
        before_num = before_inodeIn.block_point[i];
        if(before_num!=INVALIDBLOCKNUM ){
            if(isNextHaveBlock == TRUE){
                next_num = next_inodeIn.block_point[i];
                if(next_num != INVALIDBLOCKNUM){
                    copy_data_block(before_num,next_num);
                }
                else{//后者当前没有数据块，则查找后面有没有数据块
                    isNextHaveBlock = FALSE;
                    for(int j =i;j<6;j++){
                        next_num = next_inodeIn.block_point[j];
                        if(next_num!=INVALIDBLOCKNUM){
                            //复制数据块
                            next_inodeIn.block_point[i] = next_num; //调整位置
                            next_inodeIn.block_point[j] = INVALIDBLOCKNUM;
                            copy_data_block(before_num,next_num);
                            isNextHaveBlock = TRUE;
                            break;
                        }
                    }
                }
            }
            if(isNextHaveBlock==FALSE){//如果后者没有空间则新申请数据块
                int newBlock = apply_free_block();
                write_super_block(INVALIDBLOCKNUM,newBlock,FALSE,ONLY_DATA);  
                uint32_t free_num = before_inodeIn.block_point[i];
                if(free_num==INVALIDBLOCKNUM){
                    before_inodeIn.block_point[i] =newBlock;
                    copy_data_block(before_num,newBlock);
                }
            }
        }
    }
    if(next_block_num > before_block_num){//后者还有剩余的数据块、需要删除
        for(int j=before_block_num;j<next_block_num;j++){
            // free_super_block();
        }
    }
    //修改inode的信息
    next_inodeIn.size = before_inodeIn.size;
    write_inode(nextInode,next_inodeIn);

}
//第一步读取到该文件的datablock 
//创建或读取一个新的文件
//复制文件内容
int copy(char *filePath,char *nextFilePath){
    //解析字符串得到名字、路径，
    //查看路径是否存在
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char temp[121];
    int temp_index = 0;
    str_hand(filePath,&dirInode,temp,&temp_index,&isRoot);

    int isRootNext = TRUE;
    int dirInodeNext = 0;//上级目录所在的inode
    char tempNext[121];
    int temp_index_Next = 0;
    str_hand(nextFilePath,&dirInodeNext,tempNext,&temp_index_Next,&isRootNext);

    if(dirInode == -1  ){
        fprintf(stderr,RED"erro:not exist file:\"%s\"\n",filePath);
    }else if(dirInodeNext == -1){
        fprintf(stderr,RED"erro:not exist file:\"%s\"\n",nextFilePath);
    }else{
        char beforeInode = find_directory_inode_Num(dirInode,temp,FILE);
        char nextInode = find_directory_inode_Num(dirInodeNext,tempNext,FILE);
        
        if(beforeInode != -1){
            if(nextInode > 0){ //存在后者文件
                copy_inode_data(beforeInode,nextInode);
            }else{ //不存在，先创建
                struct dir_item *item = (struct dir_item *)touch(nextFilePath);
                nextInode = item->inode_id;
                copy_inode_data(beforeInode,nextInode);
                free(item);
            }
        }else
        {
            fprintf(stderr,RED"erro:not exist file:\"%s\"\n",filePath);
        }
    }

    
    
}

void write_data_block(struct inode *dataInode,char*buf){
    int blockIndex = dataInode->size/BLOCK_SIZE;
    int blockOffset = dataInode->size%BLOCK_SIZE;
    char temp_buf2[2*DEVICE_BLOCK_SIZE];
    int buf_size = strlen(buf);
    int isOK = ((BLOCK_SIZE-blockOffset)>buf_size)? TRUE:FALSE;
    int blockNum =dataInode->block_point[blockIndex];
    if(blockNum!=INVALIDBLOCKNUM){//如果有效
        if(disk_read_block(blockNum*2+DATABLOCKOFFSET,temp_buf2)==0
        &&disk_read_block(blockNum*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
        ){
            if(isOK){
                memcpy(temp_buf2+blockOffset,buf,buf_size);
            }else
            {
                memcpy(temp_buf2+blockOffset,buf,BLOCK_SIZE-blockOffset);
                if(blockNum<INODE_BLOCK_NUM-1){
                    int newBlock = apply_free_block();
                    char temp_buf3[2*DEVICE_BLOCK_SIZE];
                    if(disk_read_block(newBlock*2+DATABLOCKOFFSET,temp_buf3)==0
                 &&disk_read_block(newBlock*2+DATABLOCKOFFSET+1,temp_buf3+DEVICE_BLOCK_SIZE)==0
                    ){
                        memcpy(temp_buf3,buf+BLOCK_SIZE-blockOffset,buf_size-BLOCK_SIZE-blockOffset);
                        if(disk_write_block(newBlock*2+DATABLOCKOFFSET,temp_buf3)==0
                        &&disk_write_block(newBlock*2+DATABLOCKOFFSET+1,temp_buf3+DEVICE_BLOCK_SIZE)==0
                        ){
                            // printf("ok:写入数据block 成功\n");
                        }
                    }
                    dataInode->block_point[blockNum+1] = newBlock;
                    write_super_block(INVALIDBLOCKNUM,newBlock,FALSE,ONLY_DATA); 
                }
            }
            dataInode->size +=buf_size;
            if(disk_write_block(blockNum*2+DATABLOCKOFFSET,temp_buf2)==0
         &&disk_write_block(blockNum*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
            ){
                // printf("ok:写入数据block 成功\n");
                return;
            }
        }
    }
}

void write_file(char *charName,char *buf){
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char temp[121];
    int temp_index = 0;
    str_hand(charName,&dirInode,temp,&temp_index,&isRoot);
    char beforeInode = find_directory_inode_Num(dirInode,temp,FILE);//文件的inode
    if(beforeInode>0){
        struct inode dataInode;
        read_inode(beforeInode,(char *)&dataInode);
        write_data_block(&dataInode,buf);
        write_inode(beforeInode,dataInode);
    }else{
        fprintf(stderr,RED"erro:no file:\"%s\" in this directory\n",temp);
    }
}

void read_data_block(struct inode *dataInode){
    int blockIndex = dataInode->size/BLOCK_SIZE;
    int blockOffset = dataInode->size%BLOCK_SIZE;
    int blockNum =dataInode->block_point[blockIndex];
    char temp_buf2[2*DEVICE_BLOCK_SIZE];
    for(int i = 0;i<=blockIndex;i++){
        if(blockNum!=INVALIDBLOCKNUM){//如果有效
            if(disk_read_block(blockNum*2+DATABLOCKOFFSET,temp_buf2)==0
            &&disk_read_block(blockNum*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
            ){
                if(i<blockIndex){
                    for(int j = 0;j<BLOCK_SIZE;j++)
                        fprintf(stderr,"%c",temp_buf2[j]);
                    fprintf(stderr,"\n");
                }
                else
                {
                    for(int j = 0;j<blockOffset;j++)
                        fprintf(stderr,"%c",temp_buf2[j]);
                    fprintf(stderr,"\n");
                }
            }
        }
    }
    
}

void read_file(char *charName){
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char temp[121];
    int temp_index = 0;
    str_hand(charName,&dirInode,temp,&temp_index,&isRoot);
    char beforeInode = find_directory_inode_Num(dirInode,temp,FILE);//文件的inode
    if(beforeInode>0){
        struct inode dataInode;
        read_inode(beforeInode,(char *)&dataInode);
        read_data_block(&dataInode);
    }else{
        fprintf(stderr,RED"erro:no file:\"%s\" in this directory\n",temp);
    }

}

void find(char *charName){

}

void free_super_block(int inode_num,int data_block_num,int type,int option){
    read_super_block();
    switch (option)
    {
    case BOTH_INODE_DATA:
        if(inode_num>=0 && inode_num < INODENUM
        && data_block_num >=0 && data_block_num < BLOCKNUM
            ){
            int inode_index = inode_num / 32;
            int offsetNum = inode_num % 32;
            int data_block_index = data_block_num/32;
            int offestDataNum = data_block_num % 32;
            // printf("note:write before:%#x\n",sp_block.inode_map[inode_index]);
            // printf("note:write before:%#x\n",sp_block.block_map[data_block_index]);
            // uint32_t ans = (sp_block.inode_map[inode_index]>>31-offsetNum)& 0x1;
            // printf("note:inode ans is: %#x\n",ans);
            // ans = (sp_block.block_map[data_block_index]>>31-offestDataNum)& 0x1;
            // printf("note:block ans is: %#x\n",ans);
            
            if(((sp_block.inode_map[inode_index]>>31-offsetNum)& 0x1) ==0  
            && ((sp_block.block_map[data_block_index]>>31-offestDataNum)  & 0x1) == 0){
                sp_block.inode_map[inode_index] = sp_block.inode_map[inode_index] | (0x1 << 31-offsetNum);
                sp_block.block_map[data_block_index] = sp_block.block_map[data_block_index]|(0x1<<31-offestDataNum);
                sp_block.free_block_count--;
                sp_block.free_inode_count--;
                if(type == DIR) sp_block.dir_inode_count++;


                memcpy(temp_buf,&sp_block,sizeof(sp_block));
                disk_write_block(0,temp_buf);
                disk_write_block(1,temp_buf+DEVICE_BLOCK_SIZE);
                // printf("note:%#x\n",sp_block.inode_map[inode_index]);
                // printf("note:%#x\n",sp_block.block_map[data_block_index]);
                // printf("ok:successly update super block !\n");
                
            }else{
                printf(RED"erro:【写超级块】 inode 或 block 已被使用\n");
                exit(1);
            }
        }else{
            printf(RED"erro:【写超级块】 inode_num 或 block_num 错误\n");
            exit(1);
        }
        break;
    case ONLY_DATA:
        if(data_block_num >=0 && data_block_num < BLOCKNUM){
            int data_block_index = data_block_num/32;
            int offestDataNum = data_block_num % 32;

            printf("note:write before:%#x\n",sp_block.block_map[data_block_index]);
            uint32_t ans= ((sp_block.block_map[data_block_index] & (0x1 << 31-offestDataNum))>>31-offestDataNum)| 0x0;
            printf("note:block ans is: %#x\n",ans);
            
            if( ans == 0x1){
                sp_block.block_map[data_block_index] = sp_block.block_map[data_block_index]&(~(0x1<<31-offestDataNum));
                sp_block.free_block_count++;

                memcpy(temp_buf,&sp_block,sizeof(sp_block));
                disk_write_block(0,temp_buf);
                disk_write_block(1,temp_buf+DEVICE_BLOCK_SIZE);

                printf("note:%#x\n",sp_block.block_map[data_block_index]);
                // printf("ok:successly update super block !\n");
                
            }else{
                printf(RED"erro:【释放超级块】  block 未被使用，不能释放\n");
            }
        }else{
            printf(RED"erro:【释放超级块】 block_num 错误\n");
        }
        break;
    case ONLY_INODE:
        if(inode_num>=0 && inode_num < INODENUM){
            int inode_index = inode_num / 32;
            int offsetNum = inode_num % 32;
            printf("note:write before:%#x\n",sp_block.block_map[inode_index]);
            uint32_t ans= ((sp_block.inode_map[inode_index] & ((0x1 << 31-offsetNum)))>>31-offsetNum);
            printf("note:block ans is: %#x\n",ans);

            if(ans ==1 ){
                sp_block.inode_map[inode_index] = sp_block.inode_map[inode_index] & (~(0x1 << 31-offsetNum));
                sp_block.free_inode_count++;
                if(type == DIR) sp_block.dir_inode_count--;

                memcpy(temp_buf,&sp_block,sizeof(sp_block));
                disk_write_block(0,temp_buf);
                disk_write_block(1,temp_buf+DEVICE_BLOCK_SIZE);
            }else{
                printf(RED"erro:【释放超级块】 inode 未被使用，不能释放\n");
            }
        }else{
            printf(RED"erro:【释放超级块】 inode_num  错误\n");
        }
        break;
    default:
        break;
    }
   
}
void del_inode_block(struct inode *delInode){
    if(delInode->file_type == FILE){
        for(int i =0 ;i<INODE_BLOCK_NUM;i++){
            if(delInode->block_point[i]!= INVALIDBLOCKNUM){
                free_super_block(INVALIDBLOCKNUM,delInode->block_point[i],FILE,ONLY_DATA);
            }
        }
    }else if(delInode->file_type == DIR){
        struct dir_item init;
        init.valid = FALSE;
        struct dir_item * path;
        char temp_buf2[2*DEVICE_BLOCK_SIZE];
        for(int i =0;i<INODE_BLOCK_NUM;i++){
            if(delInode->block_point[i]!= INVALIDBLOCKNUM){
                if(disk_read_block(delInode->block_point[i]*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(delInode->block_point[i]*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        path =(struct dir_item*)(temp_buf2+128*j);
                        if(path->valid == TRUE){
                            path->valid = FALSE;
                            struct inode pathInode;
                            read_inode(path->inode_id,(char *)&pathInode);
                            del_inode_block(&pathInode);
                            //free_super_block(path->inode_id,INVALIDBLOCKNUM,FILE,ONLY_INODE);
                        }
                        // disk_write_block(delInode->block_point[i]*2+DATABLOCKOFFSET,temp_buf2);
                        // disk_write_block(delInode->block_point[i]*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE);
                    }
                    disk_write_block(delInode->block_point[i]*2+DATABLOCKOFFSET,temp_buf2);
                    disk_write_block(delInode->block_point[i]*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE);
                }
                free_super_block(INVALIDBLOCKNUM,delInode->block_point[i],FILE,ONLY_DATA);
                free_super_block(delInode->block_point[i],INVALIDBLOCKNUM,DIR,ONLY_INODE);
            }
        }
    }

    

    
}
void remove_diritem(int inodeNum,char *name){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16;
    if(disk_read_block(blockNum,temp_buf) == 0){
        struct inode* path_inode = (struct inode*)(temp_buf + 32*offsetNum);
        for(int i = 0;i<6;i++){
            uint32_t num = path_inode->block_point[i];
            if(num!=INVALIDBLOCKNUM){
                char temp_buf2[2*DEVICE_BLOCK_SIZE];
                if(disk_read_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        struct dir_item * dir_item = (struct dir_item *)(temp_buf2+128*j);
                        if(dir_item->valid == TRUE 
                        && strcmp(dir_item->name,name) == 0 
                        ){
                            dir_item->valid = FALSE;
                            memset(dir_item->name,'\0',121);
                            if(disk_write_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                                &&disk_write_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                            ){
                                // printf("ok:写入目录inode对应的数据block 成功\n");
                                return;
                            }
                        }
                    }
                }
            }
        }
        printf(RED"erro:canot delete because no\"%s\" in this directory!\n",name);
        return;
    }
}
void remove_file(char *charName){
    int isRoot = TRUE;
    int dirInode = 0;//上级目录所在的inode
    char temp[121];
    int temp_index = 0;
    str_hand(charName,&dirInode,temp,&temp_index,&isRoot);

    char beforeInode = find_directory_inode_Num(dirInode,temp,BOTH_FILE_DIR);//文件的inode
    if(beforeInode>0){
        
        struct inode dataInode;
        read_inode(beforeInode,(char *)&dataInode);
        del_inode_block(&dataInode);
        remove_diritem(dirInode,temp);
    }else{
        fprintf(stderr,RED"erro:cannot remove because:no file or directory:\"%s\" in this directory\n",temp);
    }
}
int is_exsit_dir_item(int inodeNum,char *dir_name){
    int blockNum = inodeNum / 16 + 2;
    int offsetNum = inodeNum % 16; 
    if(disk_read_block(blockNum,temp_buf) == 0){
        struct inode* path_inode = (struct inode*)(temp_buf + 32*offsetNum);
        for(int i = 0;i<6;i++){
            uint32_t num = path_inode->block_point[i];
            if(num!=INVALIDBLOCKNUM){
                char temp_buf2[2*DEVICE_BLOCK_SIZE];
                if(disk_read_block(num*2+DATABLOCKOFFSET,temp_buf2)==0
                &&disk_read_block(num*2+DATABLOCKOFFSET+1,temp_buf2+DEVICE_BLOCK_SIZE)==0
                ){
                    for(int j =0;j<8;j++){
                        struct dir_item * dir_item = (struct dir_item *)(temp_buf2+128*j);
                        if(dir_item->valid == TRUE && dir_item->type ==DIR){
                            return TRUE;
                        }
                    }
                }
            }
        }
        return FALSE;
    }

}

void change_current_path(char *dir_name){
    if(strcmp(dir_name,"..")==0){
        for(int i=strlen(currentPath)-1;i>=1;i--){
            if(currentPath[i]!='/'){
                currentPath[i]='\0';
            }
            else{
                currentPath[i]='\0';
                break;
            }
        }        
    }else{
        
        int isRoot = TRUE;
        int dirInode = 0;//上级目录所在的inode
        char temp[121];
        int temp_index = 0;
        // char temp_path[128];
        // strcat(temp_path,currentPath);
        // strcat(temp_path,dir_name);
        str_hand(currentPath,&dirInode,temp,&temp_index,&isRoot);
        if(is_exsit_dir_item(dirInode,dir_name) == TRUE && dirInode !=-1){
            if(strlen(currentPath)!=1)
                strcat(currentPath,"/");
            strcat(currentPath,dir_name);
            strcpy(currentFileName,dir_name);
        }else{
            fprintf(stderr,"erro: no direcoty called\"%s\" in \"%s\"",dir_name,currentPath);
        }

    }

}

int work(){
    printf(YELLOW"------------------------------------------\n");
    printf(LIGHT_BLUE"|   Welcome to NaiveExt2! 年轻人耗子尾汁  |\n");
    printf(CYAN"|   支持的命令：                          |\n");
    printf(CYAN"|   展示文件夹   ： ls fileName            |\n");
    printf(CYAN"|   创建文件夹   ： mkdir filename         |\n");
    printf(CYAN"|   创建文件     ： touch filename         |\n");
    printf(CYAN"|   删除文件/目录： rm file/path           |\n");
    printf(CYAN"|   复制文件     ： cp fileA fileB         |\n");
    printf(CYAN"|   向文件写数据 ： echo data > fileName   |\n");
    printf(CYAN"|   打印文件     ： cat fileName           |\n");
    printf(CYAN"|   退出         ： quit                   |\n");
    printf(YELLOW"------------------------------------------\n");
}

int getcmd(char *buf, int nbuf) //读取指令
{
//   fprintf(2, "@ ");
  fprintf(stderr,GREEN"Iris@: %s:"NONE,currentPath);
  memset(buf, 0, nbuf); //初始话buf全为0
  gets(buf, nbuf);      //从缓冲区读
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

void getargs(char *buf, char* argv[],int *argc){
    int argv_num=0;  //参数个数
    int buf_index=0; //缓冲区索引
    for(;buf[buf_index]!='\n'&&buf[buf_index]!='\0';buf_index++){
        while(strchr(whitespace,buf[buf_index])){ //跳过空格
            buf_index++;
        }
        // echo  "hello world"
        argv[argv_num] = buf + buf_index; //参数列表存储第一个参数
        argv_num ++;
        while(strchr(whitespace,buf[buf_index]) == 0){ //寻找下一个空格
            buf_index++;
        }
        buf[buf_index]='\0';//修改成字符串终结符'\0'
    }
    argv[argv_num] = 0;
    *argc = argv_num;
}


void runcmd(char* argv[],int argc){
    read_super_block();
    int soutIndex =-1;
    for(int i = 0;i<argc;i++){
        if(!strcmp(argv[i],"|")){ //管道通信
            if(argc != i){ //后面至少还有一个参数
                // usePipe(argv,argc,i);
            }
            else{
                fprintf(stderr,"%s\n","no argv after |");
            }
        }
        if(!strcmp(argv[i],">")){ //输出重定向到下一个文件！
            if(i<argc-1){ //后面至少还有一个参数
                // close(1);
                // open(argv[i+1],O_CREATE|O_WRONLY);
                soutIndex = i;
            }
            else{
                fprintf(stderr,"%s\n","no argv after >");
            }
        }
        if(!strcmp(argv[i],"<")){ //输入重定向
            if(argc != i){ //后面至少还有一个参数
                // close(0);
                // open(argv[i+1],O_RDONLY);
                argv[i] = '\0'; //停止
            }
            else{
                fprintf(stderr,"%s\n","no argv after <");
            }
        }
    }
    // exec(argv[0],argv);
    if(!strcmp(argv[0],"ls")){//ls
        if(strcmp(argv[argc-1],"-a") == 0){
            for(int i = 1;i<argc-1;i++){
                ls(argv[i],LS_RECURISON);
            }
            if(argc == 2) ls("",LS_RECURISON);
        }else
        {
            for(int i = 1;i<argc;i++){
                ls(argv[i],LS_ONLY_CURRENT_DIR);
            }
        }
        if(argc == 1) ls("",LS_RECURISON);

    }else if(!strcmp(argv[0],"mkdir")){
        for(int i = 1;i<argc;i++){
            mkdir(argv[i]);
        }
    }
    else if(!strcmp(argv[0],"touch")){
        for(int i = 1;i<argc;i++){
            touch(argv[i]);
        }
    }
    else if(!strcmp(argv[0],"cp")){
        if(argc == 3){
            copy(argv[1],argv[2]);
        }
    }else if(!strcmp(argv[0],"echo")){//打印字符串及重定位

        if(soutIndex>0){
            for(int i = 1;i<soutIndex;i++){
                write_file(argv[soutIndex+1],argv[i]);
            }
        }else
        {
            for(int i = 1;i<argc;i++){
                fprintf(stdout,"%s\n",argv[i]);
            }
        }
        
    }else if(!strcmp(argv[0],"cat")){ //暂不支持重定位
        read_file(argv[1]);
    }else if(!strcmp(argv[0],"rm")){
        remove_file(argv[1]);
    }else if(!strcmp(argv[0],"cd")){
        if(argc == 2)
            change_current_path(argv[1]);
    }else if(!strcmp(argv[0],"quit")){
        fprintf(stderr,"------------------------------------------\n");
        fprintf(stderr,"|   NaiveExt2:      莎扬娜拉~             |\n");
        fprintf(stderr,"------------------------------------------\n");
        exit(0);
    }else
    {
        fprintf(stderr,RED"erro:no order called:\"%s\" in this directory\n",argv[0]);
    }
    
}



int naive_shell_main(void){
    static char buf[128];
    while(getcmd(buf, sizeof(buf)) >= 0){
      char *argv[MAXARGS]; //参数列表
      int argc = -1;       //参数个数
      getargs(buf,argv,&argc); //处理缓冲区 得到命令 
      runcmd(argv,argc);
    }
  exit(0);
}

int main(){
    init_naiveExt2();
    init_super_block();
    
    char filename[121] = {'/','\0'};
    work();
    naive_shell_main();
    close_disk();
}