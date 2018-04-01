#include "headers.h"
using namespace std;

SuperBlock* superblock = new SuperBlock();

FILE* fw;
FILE* fr;

bool INODE_BITMAP[INODE_NUM];
bool BLOCK_BITMAP[BLOCK_NUM];

const int Superblock_StartAddr = 0;											//SuperBlockռ��һ��block(block��СΪ1024kb)
const int INODE_BITMAP_StartAddr = Superblock_StartAddr + BLOCK_SIZE;		//INODE_BITMAP 1000��bit ռ��1��block
const int BLOCK_BITMAP_StartAddr = INODE_BITMAP_StartAddr + BLOCK_SIZE * 2;	//BLOCK_BITMAP 10000��bit ռ��10��block
const int Inode_StartAddr = BLOCK_BITMAP_StartAddr + BLOCK_SIZE * 10;		//Inode 1000*128 ռ��128��block
const int Block_StartAddr = Inode_StartAddr + BLOCK_SIZE * 128;				//Block 10000*1024 ռ��10000��block
const char authority[11] = "drwxrwxrwx";

int Root_Dir_Addr;							//��Ŀ¼inode��ַ
int Cur_Dir_Addr;							//��ǰĿ¼
int Cur_Dir_Num;							//Ŀ¼����
char Cur_Dir_Name[30][28];					//��ǰĿ¼��
char Cur_Host_Name[110];					//��ǰ������
char Cur_User_Name[110];					//��ǰ��½�û���
char Cur_Group_Name[110];					//��ǰ��½�û�����
char Cur_User_Dir_Name[310];				//��ǰ��½�û�Ŀ¼��

int nextUID;								//��һ��Ҫ������û���ʶ��
int nextGID;								//��һ��Ҫ������û����ʶ��

bool isLogin;								//�Ƿ����û���½

char command[1000];
int main()
{
	if((fr = fopen(FILESYSNAME,"rb"))==NULL){	//ֻ������������ļ����������ļ���
		//��������ļ������ڣ�����һ��
		printf("����û�д��̣����ڴ�������\n");
		fw = fopen(FILESYSNAME,"wb");	//ֻд����������ļ����������ļ���
		if(fw==NULL){
			printf("��������ļ���ʧ��\n");
			return 0;	//���ļ�ʧ��
		}
		else
			printf("������̴򿪳ɹ�\n");

		fr = fopen(FILESYSNAME,"rb");	//���ڿ��Դ���
		printf("�ɹ���ȡ���̣����ڸ�ʽ��\n");

		if (Format())
			printf("��ʽ���ɹ�\n");
		else {
			printf("��ʽ��ʧ��\n");
			return 0;
		}
		if (!Install()){
			printf("��װ�ļ�ϵͳʧ��\n");
			return 0;
		}
		if (create_root())
			printf("����root�ɹ�\n");
        isLogin = true;

	}
	else{
		if (!Install()){
			printf("��װ�ļ�ϵͳʧ��\n");
			return 0;
		}
		printf("�ɹ���ȡ����\n");
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
