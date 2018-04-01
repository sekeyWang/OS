#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define FILESYSNAME	"OS.wsk"

#define INODE_SIZE 128				//inode大小128b
#define BLOCK_SIZE 1024				//block大小1kb
#define MAX_NAME_SIZE 28			//目录或者文件名最大长度为28
#define INODE_NUM 1000				//文件个数
#define BLOCK_NUM 10000				//块个数
#define BLOCKS_PER_GROUP 100

//权限
#define MODE_DIR	01000					//目录标识
#define MODE_FILE	00000					//文件标识
#define OWNER_R	4<<6						//本用户读权限
#define OWNER_W	2<<6						//本用户写权限
#define OWNER_X	1<<6						//本用户执行权限
#define GROUP_R	4<<3						//组用户读权限
#define GROUP_W	2<<3						//组用户写权限
#define GROUP_X	1<<3						//组用户执行权限
#define OTHERS_R	4						//其它用户读权限
#define OTHERS_W	2						//其它用户写权限
#define OTHERS_X	1						//其它用户执行权限
#define FILE_DEF_PERMISSION 0664			//文件默认权限
#define DIR_DEF_PERMISSION	0755			//目录默认权限

extern const int Superblock_StartAddr;
extern const int INODE_BITMAP_StartAddr;
extern const int BLOCK_BITMAP_StartAddr;
extern const int Inode_StartAddr;
extern const int Block_StartAddr;
extern const char authority[11];

extern int Root_Dir_Addr;					//根目录inode地址
extern int Cur_Dir_Addr;					//当前目录
extern int Cur_Dir_Num;						//目录层数
extern char Cur_Dir_Name[30][28];			//当前目录名
extern char Cur_Host_Name[110];				//当前主机名
extern char Cur_User_Name[110];				//当前登陆用户名
extern char Cur_Group_Name[110];			//当前登陆用户组名
extern char Cur_User_Dir_Name[310];			//当前登陆用户目录名

extern int nextUID;							//下一个要分配的用户标识号
extern int nextGID;							//下一个要分配的用户组标识号

extern bool isLogin;						//是否有用户登陆


extern FILE* fw;
extern FILE* fr;

extern bool INODE_BITMAP[INODE_NUM];
extern bool BLOCK_BITMAP[BLOCK_NUM];


struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode节点数，最多 65535
	unsigned int s_BLOCK_NUM;				//磁盘块块数，最多 4294967294

	unsigned short s_free_INODE_NUM;		//空闲inode节点数
	unsigned int s_free_BLOCK_NUM;			//空闲磁盘块数
	int s_free_addr;						//空闲块堆栈指针
	int s_free[BLOCKS_PER_GROUP];			//空闲块堆栈

	unsigned short s_BLOCK_SIZE;			//磁盘块大小
	unsigned short s_INODE_SIZE;			//inode大小
	unsigned short s_SUPERBLOCK_SIZE;		//超级块大小
	unsigned short s_blocks_per_group;		//每 blockgroup 的block数量

	//磁盘分布
	int s_Superblock_StartAddr;
	int s_INODE_BITMAP_StartAddr;
	int s_BLOCK_BITMAP_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

extern SuperBlock* superblock;

struct Inode{
	unsigned short i_id;					//inode标识（编号）(0~65535)
	unsigned short i_mode;					//存取权限。r--读取，w--写，x--执行
	unsigned short i_cnt;					//链接数。有多少文件名指向这个inode
	char i_uname[20];						//文件所属用户
	char i_gname[20];						//文件所属用户组
	unsigned int i_size;					//文件大小，单位为字节（B）
	time_t  i_ctime;						//文件创建时间
	time_t  i_mtime;						//文件上一次变动的时间
	unsigned int i_dirBlock[10];			//10个直接块。10*1024B = 10KB
	unsigned int i_indirBlock_1;			//一级间接块。1024B/4 * 1024B = 256 * 1024B = 256KB
											//文件系统太小，暂且省略二级、三级间接块
};

//目录项
struct DirItem{								//4+28=32字节，一个磁盘块能存 1024/32=32 个目录项
	char itemName[MAX_NAME_SIZE];			//目录或者文件名
	int inodeAddr;							//目录项对应的inode节点地址
};


struct UserItem{    //16+16=32字节 一个磁盘块可以放1024/16个用户
    char userName[16];
    char password[16];
};

bool Format();								//格式化
bool Install();								//安装文件系统
int ialloc();								//分配一个空闲Inode节点
bool ifree(int address);					//释放一个Inode节点
void show_inode_bitmap();					//展示InodeBitmap
int balloc();								//分配一个空闲Block
bool bfree(int address);					//释放一个block
void show_block_bitmap();					//展示BlockBitmap
void prepare();								//安装前的初始化
bool create_root();							//创建根目录
int mkdir(int father_addr, char name[]);	//创建文件夹
void ls(int address);						//展示目录所有内容
void rmall(int address);					//删除该目录下所有内容
bool rmdir(int address, char name[]);		//删除该目录下制定文件或文件夹
int cd(int address, char name[]);			//进入制定目录
bool create(int address, char name[]);		//创建一个文件
void re_write(int address, char text[]);	//重写某一文件
void add_write(int address, char text[]);	//追加写某一文件
void cat(int address);						//用字符的方式展示文件中所有内容
void help();								//操作说明书
void Login();								//用户登录
void cmd(char str[]);						//读取用户指令并执行
