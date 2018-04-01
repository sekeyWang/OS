#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define FILESYSNAME	"OS.wsk"

#define INODE_SIZE 128				//inode��С128b
#define BLOCK_SIZE 1024				//block��С1kb
#define MAX_NAME_SIZE 28			//Ŀ¼�����ļ�����󳤶�Ϊ28
#define INODE_NUM 1000				//�ļ�����
#define BLOCK_NUM 10000				//�����
#define BLOCKS_PER_GROUP 100

//Ȩ��
#define MODE_DIR	01000					//Ŀ¼��ʶ
#define MODE_FILE	00000					//�ļ���ʶ
#define OWNER_R	4<<6						//���û���Ȩ��
#define OWNER_W	2<<6						//���û�дȨ��
#define OWNER_X	1<<6						//���û�ִ��Ȩ��
#define GROUP_R	4<<3						//���û���Ȩ��
#define GROUP_W	2<<3						//���û�дȨ��
#define GROUP_X	1<<3						//���û�ִ��Ȩ��
#define OTHERS_R	4						//�����û���Ȩ��
#define OTHERS_W	2						//�����û�дȨ��
#define OTHERS_X	1						//�����û�ִ��Ȩ��
#define FILE_DEF_PERMISSION 0664			//�ļ�Ĭ��Ȩ��
#define DIR_DEF_PERMISSION	0755			//Ŀ¼Ĭ��Ȩ��

extern const int Superblock_StartAddr;
extern const int INODE_BITMAP_StartAddr;
extern const int BLOCK_BITMAP_StartAddr;
extern const int Inode_StartAddr;
extern const int Block_StartAddr;
extern const char authority[11];

extern int Root_Dir_Addr;					//��Ŀ¼inode��ַ
extern int Cur_Dir_Addr;					//��ǰĿ¼
extern int Cur_Dir_Num;						//Ŀ¼����
extern char Cur_Dir_Name[30][28];			//��ǰĿ¼��
extern char Cur_Host_Name[110];				//��ǰ������
extern char Cur_User_Name[110];				//��ǰ��½�û���
extern char Cur_Group_Name[110];			//��ǰ��½�û�����
extern char Cur_User_Dir_Name[310];			//��ǰ��½�û�Ŀ¼��

extern int nextUID;							//��һ��Ҫ������û���ʶ��
extern int nextGID;							//��һ��Ҫ������û����ʶ��

extern bool isLogin;						//�Ƿ����û���½


extern FILE* fw;
extern FILE* fr;

extern bool INODE_BITMAP[INODE_NUM];
extern bool BLOCK_BITMAP[BLOCK_NUM];


struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode�ڵ�������� 65535
	unsigned int s_BLOCK_NUM;				//���̿��������� 4294967294

	unsigned short s_free_INODE_NUM;		//����inode�ڵ���
	unsigned int s_free_BLOCK_NUM;			//���д��̿���
	int s_free_addr;						//���п��ջָ��
	int s_free[BLOCKS_PER_GROUP];			//���п��ջ

	unsigned short s_BLOCK_SIZE;			//���̿��С
	unsigned short s_INODE_SIZE;			//inode��С
	unsigned short s_SUPERBLOCK_SIZE;		//�������С
	unsigned short s_blocks_per_group;		//ÿ blockgroup ��block����

	//���̷ֲ�
	int s_Superblock_StartAddr;
	int s_INODE_BITMAP_StartAddr;
	int s_BLOCK_BITMAP_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

extern SuperBlock* superblock;

struct Inode{
	unsigned short i_id;					//inode��ʶ����ţ�(0~65535)
	unsigned short i_mode;					//��ȡȨ�ޡ�r--��ȡ��w--д��x--ִ��
	unsigned short i_cnt;					//���������ж����ļ���ָ�����inode
	char i_uname[20];						//�ļ������û�
	char i_gname[20];						//�ļ������û���
	unsigned int i_size;					//�ļ���С����λΪ�ֽڣ�B��
	time_t  i_ctime;						//�ļ�����ʱ��
	time_t  i_mtime;						//�ļ���һ�α䶯��ʱ��
	unsigned int i_dirBlock[10];			//10��ֱ�ӿ顣10*1024B = 10KB
	unsigned int i_indirBlock_1;			//һ����ӿ顣1024B/4 * 1024B = 256 * 1024B = 256KB
											//�ļ�ϵͳ̫С������ʡ�Զ�����������ӿ�
};

//Ŀ¼��
struct DirItem{								//4+28=32�ֽڣ�һ�����̿��ܴ� 1024/32=32 ��Ŀ¼��
	char itemName[MAX_NAME_SIZE];			//Ŀ¼�����ļ���
	int inodeAddr;							//Ŀ¼���Ӧ��inode�ڵ��ַ
};


struct UserItem{    //16+16=32�ֽ� һ�����̿���Է�1024/16���û�
    char userName[16];
    char password[16];
};

bool Format();								//��ʽ��
bool Install();								//��װ�ļ�ϵͳ
int ialloc();								//����һ������Inode�ڵ�
bool ifree(int address);					//�ͷ�һ��Inode�ڵ�
void show_inode_bitmap();					//չʾInodeBitmap
int balloc();								//����һ������Block
bool bfree(int address);					//�ͷ�һ��block
void show_block_bitmap();					//չʾBlockBitmap
void prepare();								//��װǰ�ĳ�ʼ��
bool create_root();							//������Ŀ¼
int mkdir(int father_addr, char name[]);	//�����ļ���
void ls(int address);						//չʾĿ¼��������
void rmall(int address);					//ɾ����Ŀ¼����������
bool rmdir(int address, char name[]);		//ɾ����Ŀ¼���ƶ��ļ����ļ���
int cd(int address, char name[]);			//�����ƶ�Ŀ¼
bool create(int address, char name[]);		//����һ���ļ�
void re_write(int address, char text[]);	//��дĳһ�ļ�
void add_write(int address, char text[]);	//׷��дĳһ�ļ�
void cat(int address);						//���ַ��ķ�ʽչʾ�ļ�����������
void help();								//����˵����
void Login();								//�û���¼
void cmd(char str[]);						//��ȡ�û�ָ�ִ��
