#include "headers.h"
using namespace std;

SuperBlock* superblock = new SuperBlock();

FILE* fw;
FILE* fr;

bool INODE_BITMAP[INODE_NUM];
bool BLOCK_BITMAP[BLOCK_NUM];

const int Superblock_StartAddr = 0;											//SuperBlock占用一个block(block大小为1024kb)
const int INODE_BITMAP_StartAddr = Superblock_StartAddr + BLOCK_SIZE;		//INODE_BITMAP 1000个bit 占用1个block
const int BLOCK_BITMAP_StartAddr = INODE_BITMAP_StartAddr + BLOCK_SIZE * 2;	//BLOCK_BITMAP 10000个bit 占用10个block
const int Inode_StartAddr = BLOCK_BITMAP_StartAddr + BLOCK_SIZE * 10;		//Inode 1000*128 占用128个block
const int Block_StartAddr = Inode_StartAddr + BLOCK_SIZE * 128;				//Block 10000*1024 占用10000个block
const char authority[11] = "drwxrwxrwx";

int Root_Dir_Addr;							//根目录inode地址
int Cur_Dir_Addr;							//当前目录
int Cur_Dir_Num;							//目录层数
char Cur_Dir_Name[30][28];					//当前目录名
char Cur_Host_Name[110];					//当前主机名
char Cur_User_Name[110];					//当前登陆用户名
char Cur_Group_Name[110];					//当前登陆用户组名
char Cur_User_Dir_Name[310];				//当前登陆用户目录名

int nextUID;								//下一个要分配的用户标识号
int nextGID;								//下一个要分配的用户组标识号

bool isLogin;								//是否有用户登陆

char command[1000];
int main()
{
	if((fr = fopen(FILESYSNAME,"rb"))==NULL){	//只读打开虚拟磁盘文件（二进制文件）
		//虚拟磁盘文件不存在，创建一个
		printf("发现没有磁盘，正在创建磁盘\n");
		fw = fopen(FILESYSNAME,"wb");	//只写打开虚拟磁盘文件（二进制文件）
		if(fw==NULL){
			printf("虚拟磁盘文件打开失败\n");
			return 0;	//打开文件失败
		}
		else
			printf("虚拟磁盘打开成功\n");

		fr = fopen(FILESYSNAME,"rb");	//现在可以打开了
		printf("成功读取磁盘，正在格式化\n");

		if (Format())
			printf("格式化成功\n");
		else {
			printf("格式化失败\n");
			return 0;
		}
		if (!Install()){
			printf("安装文件系统失败\n");
			return 0;
		}
		if (create_root())
			printf("创建root成功\n");
        isLogin = true;

	}
	else{
		if (!Install()){
			printf("安装文件系统失败\n");
			return 0;
		}
		printf("成功读取磁盘\n");
		fw = fopen(FILESYSNAME,"rb+");
	}
	while (true){
        if (!isLogin) {
            Login();
            gets(command);
            continue;
        }
		printf("[%s@%s ", Cur_Host_Name, Cur_User_Name);
		if (Cur_Dir_Num == 0) printf("/");
		else
			for (int i = 0; i < Cur_Dir_Num; i++)
				printf("/%s", Cur_Dir_Name[i]);
		printf("]# ");
		gets(command);
		cmd(command);
	}

	return 0;
}
