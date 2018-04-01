#include "headers.h"

bool Format()
{
	superblock->s_INODE_NUM = INODE_NUM;
	superblock->s_BLOCK_NUM = BLOCK_NUM;
	superblock->s_free_INODE_NUM = INODE_NUM;
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;
	superblock->s_INODE_SIZE = INODE_SIZE;
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;
	superblock->s_INODE_BITMAP_StartAddr = INODE_BITMAP_StartAddr;
	superblock->s_BLOCK_BITMAP_StartAddr = BLOCK_BITMAP_StartAddr;
	superblock->s_Inode_StartAddr = Inode_StartAddr;
	superblock->s_Block_StartAddr = Block_StartAddr;
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock->s_free_addr = Block_StartAddr;

	//初始化inode位图
	memset(INODE_BITMAP,0,sizeof(INODE_BITMAP));
	fseek(fw,INODE_BITMAP_StartAddr,SEEK_SET);
	fwrite(INODE_BITMAP,sizeof(INODE_BITMAP),1,fw);

	//初始化block位图
	memset(BLOCK_BITMAP,0,sizeof(BLOCK_BITMAP));
	fseek(fw,BLOCK_BITMAP_StartAddr,SEEK_SET);
	fwrite(BLOCK_BITMAP,sizeof(BLOCK_BITMAP),1,fw);


	for (int i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--){
		if (i != BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			superblock->s_free[0] = Block_StartAddr + BLOCKS_PER_GROUP * (i + 1) * BLOCK_SIZE;
		else
			superblock->s_free[0] = -1;
		for (int j = 1; j < BLOCKS_PER_GROUP; j++)
			superblock->s_free[j] = Block_StartAddr + (BLOCKS_PER_GROUP * i + j) * BLOCK_SIZE;
		fseek(fw, Block_StartAddr + BLOCKS_PER_GROUP * i * BLOCK_SIZE, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);
	}

	fseek(fw, superblock->s_Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fr);
	fflush(fw);

	return true;
}

bool Install()	//安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
{
	prepare();
	//读取超级块
	fseek(fr,Superblock_StartAddr,SEEK_SET);
	fread(superblock,sizeof(SuperBlock),1,fr);

	//读取inode位图
	fseek(fr,INODE_BITMAP_StartAddr,SEEK_SET);
	fread(INODE_BITMAP,sizeof(INODE_BITMAP),1,fr);

	//读取block位图
	fseek(fr,BLOCK_BITMAP_StartAddr,SEEK_SET);
	fread(BLOCK_BITMAP,sizeof(BLOCK_BITMAP),1,fr);

	return true;
}

int ialloc()
{
	if (superblock->s_free_INODE_NUM == 0){
		printf("没有空闲的inode\n");
		return -1;
	}
	int pos = -1;
	for (int i = 0; i < superblock->s_INODE_NUM; i++){
		if (INODE_BITMAP[i] == 0) {pos = i; break;}
	}
	if (pos == -1) return pos;
	//rewrite INODE_BITMAP
	INODE_BITMAP[pos] = 1;
	fseek(fw, superblock->s_INODE_BITMAP_StartAddr + pos, SEEK_SET);
	fwrite(&INODE_BITMAP[pos], sizeof(bool), 1, fw);
	//rewrite superblock
	superblock->s_free_INODE_NUM--;
	fseek(fw, superblock->s_Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);
	//return address
	return superblock->s_Inode_StartAddr + pos * superblock->s_INODE_SIZE;
}

bool ifree(int address)
{
	if ((address - superblock->s_Inode_StartAddr) % superblock->s_INODE_SIZE != 0){
		printf("地址错误\n");
		return false;
	}
	int pos = (address - superblock->s_Inode_StartAddr) / superblock->s_INODE_SIZE;
	if (INODE_BITMAP[pos] == 0){
		printf("该位置已经是空的，无法释放此inode。inode地址：%d\n", address);
		return false;
	}
	//rewrite INODE_BITMAP
	INODE_BITMAP[pos] = 0;
	fseek(fw, superblock->s_INODE_BITMAP_StartAddr + pos, SEEK_SET);
	fwrite(&INODE_BITMAP[pos], sizeof(bool), 1, fw);
	//rewrite superblock
	superblock->s_free_INODE_NUM++;
	fseek(fw, superblock->s_Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);
	return true;
}

void show_inode_bitmap()
{
	for (int i = 0; i < superblock->s_INODE_NUM; i++){
		if (i % 50 == 0) printf("\n");
		bool flag = INODE_BITMAP[i];
		if (flag) printf("*");
		else printf(".");
	}
	printf("\n");
}

//apply for a new block
int balloc()
{
	if (superblock->s_free_BLOCK_NUM == 0){
		printf("没有空闲的block\n");
		return -1;
	}
	int top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	int retAddr;

	if (top == 0){
		retAddr = superblock->s_free_addr;
		superblock->s_free_addr = superblock->s_free[0];
		fseek(fr, superblock->s_free_addr, SEEK_SET);
		fread(superblock->s_free, sizeof(superblock->s_free), 1, fr);
	}
	else{
		retAddr = superblock->s_free[top];
		superblock->s_free[top] = -1;
	}
	superblock->s_free_BLOCK_NUM--;
	//rewrite superblock
	fseek(fw, superblock->s_Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	//rewrite BLOCK_BITMAP
	int m = (retAddr - superblock->s_Block_StartAddr) / superblock->s_BLOCK_SIZE;
	BLOCK_BITMAP[m] = 1;
	fseek(fw, superblock->s_BLOCK_BITMAP_StartAddr + m, SEEK_SET);
	fwrite(&BLOCK_BITMAP[m], sizeof(bool), 1, fw);
	//return address
	fflush(fw);
	fflush(fr);
	return retAddr;
}

bool bfree(int address)
{
	if ((address - superblock->s_Block_StartAddr) % superblock->s_BLOCK_SIZE != 0){
		printf("地址出错\n");
		return false;
	}
	int m = (address - superblock->s_Block_StartAddr) / superblock->s_BLOCK_SIZE;
	if (BLOCK_BITMAP[m] == 0){
		printf("该磁盘未被占用，无法释放此block。block地址%d\n", address);
		return false;
	}

	int top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	superblock->s_free_BLOCK_NUM++;

	if (top == superblock->s_blocks_per_group - 1){
		superblock->s_free[0] = superblock->s_free_addr;
		superblock->s_free_addr = address;
		for (int i = 1; i < superblock->s_blocks_per_group; i++)
			superblock->s_free[i] = -1;
		fseek(fw, address, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);
	}
	else {
		top++;
		superblock->s_free[top] = address;
	}
	//rewrite superblock
	fseek(fw, superblock->s_Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	//rewrite BLOCK_BITMAP
	BLOCK_BITMAP[m] = 0;
	fseek(fw, superblock->s_BLOCK_BITMAP_StartAddr + m, SEEK_SET);
	fwrite(&BLOCK_BITMAP[m], sizeof(bool), 1, fw);
	fflush(fw);
	fflush(fr);
	return true;
}

void show_block_bitmap()
{
	for (int i = 0; i < superblock->s_BLOCK_NUM; i++){
		if (i % superblock->s_blocks_per_group == 0) printf("\n");
		if (BLOCK_BITMAP[i] == 0) printf(".");
		else printf("*");
	}
	printf("\n");
}

void prepare()
{
	nextUID = 0;
	nextGID = 0;
	isLogin = false;
	strcpy(Cur_User_Name,"root");
	strcpy(Cur_Group_Name,"root");
	//获取主机名
	memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
	DWORD k = 100;
	GetComputerName(Cur_Host_Name,&k);
//	strcpy(Cur_Dir_Name, "/");
	Cur_Dir_Num = 0;
	Root_Dir_Addr = Inode_StartAddr;
	Cur_Dir_Addr = Root_Dir_Addr;
//	Cur_Dir_Addr;
//	Cur_User_Dir_Name;
}

bool create_root()
{
	Inode cur;
	int inoAddr = ialloc();
	int blockAddr = balloc();

	DirItem dirlist[32];
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inodeAddr = inoAddr;
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	cur.i_id = 0;
	cur.i_ctime = time(NULL);
	cur.i_mtime = time(NULL);
	cur.i_dirBlock[0] = blockAddr;
	for (int i = 1; i < 10; i++)
		cur.i_dirBlock[i] = -1;
	cur.i_indirBlock_1 = -1;
	cur.i_cnt = 1;
//	cur.i_size = sizeof(dirlist);
	cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;
	strcpy(cur.i_uname, Cur_User_Name);
	strcpy(cur.i_gname, Cur_Group_Name);

	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);

//create usr
	mkdir(inoAddr, "usr");
	mkdir(inoAddr, "root");
	cd(inoAddr, "root");
//	add_account("root", "root");
	return true;
}

int mkdir(int father_addr, char name[])
{
	if (strlen(name) > MAX_NAME_SIZE){
		printf("名字长度超过限制\n");
		return false;
	}
	//read inode
	Inode father;
	fseek(fr, father_addr, SEEK_SET);
	fread(&father, sizeof(Inode), 1, fr);
	//check the name
	DirItem dirlist[32];
	int indirlist[256];
	if (father.i_cnt > 320){
		fseek(fr, father.i_indirBlock_1, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	for (int i = 0; i < father.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			int blockAddr;
			if (pos_i < 10) blockAddr = father.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0) {
			printf("该命名已经存在\n");
			return -1;
		}
	}
	int father_block_addr;
	int pos_x = father.i_cnt / 32;
	int pos_y = father.i_cnt % 32;
	//创建一级间接块
	if (father.i_cnt == 320){
		int indirAddr = balloc();
		if (indirAddr == -1){
			printf("空间不足，无法生成一级间接快\n");
			return -1;
		}
		father.i_indirBlock_1 = indirAddr;
	}
	//find block's address
	if (pos_y == 0){
		father_block_addr = balloc();
		if (father_block_addr == -1) {
			printf("空间不足，无法生成新目录\n");
			return -1;
		}
		if (pos_x < 10) father.i_dirBlock[pos_x] = father_block_addr;
		else indirlist[pos_x - 10] = father_block_addr;
	}
	if (pos_x < 10) father_block_addr = father.i_dirBlock[pos_x];
	else father_block_addr = indirlist[pos_x - 10];

	//modify Inode's information and write back
	father.i_cnt++;
	father.i_mtime = time(NULL);
	fseek(fw, father_addr, SEEK_SET);
	fwrite(&father, sizeof(Inode), 1, fw);

	//read dirlist and add a diritem
	fseek(fr, father_block_addr, SEEK_SET);
	fread(dirlist, sizeof(dirlist), 1, fr);
	Inode cur;
	int inoAddr = ialloc();
	int blockAddr = balloc();
	dirlist[pos_y].inodeAddr = inoAddr;
	strcpy(dirlist[pos_y].itemName, name);

	//write back the dirlist
	fseek(fw, father_block_addr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//create a new dir
	DirItem dirlist2[32];
	strcpy(dirlist2[0].itemName, ".");
	dirlist2[0].inodeAddr = inoAddr;
	strcpy(dirlist2[1].itemName, "..");
	dirlist2[1].inodeAddr = father_addr;
	//write dirlist into disk
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist2, sizeof(dirlist2), 1, fw);

	cur.i_id = (inoAddr - superblock->s_Inode_StartAddr) / superblock->s_INODE_SIZE;
	cur.i_ctime = time(NULL);
//	cur.i_atime = time(NULL);
	cur.i_mtime = time(NULL);
	cur.i_dirBlock[0] = blockAddr;
	for (int i = 1; i < 10; i++)
		cur.i_dirBlock[i] = -1;
//	cur.i_indirBlock_1 = -1;
	cur.i_cnt = 2;
//	cur.i_size = sizeof(dirlist2);
	cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;
	strcpy(cur.i_uname, Cur_User_Name);
	strcpy(cur.i_gname, Cur_Group_Name);
	//rewrite inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);
	return inoAddr;
}

void ls(int address)
{
	Inode cur;
	fseek(fr, address, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	//权限判断后期加上
	DirItem dirlist[32];
	int indirlist[256];

	if (cur.i_cnt > 320){
		int blockAddr = cur.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	printf("total %d\n", cur.i_cnt);
	for (int i = 0; i < cur.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			int blockAddr;
			if (pos_i < 10) blockAddr = cur.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		Inode son;
		fseek(fr, dirlist[pos_j].inodeAddr, SEEK_SET);
		fread(&son, sizeof(Inode), 1, fr);
		for (int i = 9; i >= 0; i--)
			if ((1 << i) & son.i_mode) printf("%c", authority[9 - i]);
			else printf("_");
		char mtime[30];
		strcpy(mtime, ctime(&son.i_mtime));
		mtime[strlen(mtime) - 1] = '\0';
		printf("%4d%10s%10s%27s%15s\n", son.i_cnt, son.i_uname, son.i_gname, mtime, dirlist[pos_j].itemName);
//		printf(ctime(&son.i_mtime));
//		printf("%10s\n", dirlist[pos_j].itemName);
	}
}

void rmall(int address)
{
	Inode cur;
	fseek(fr, address, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	int indirlist[256];
	if (cur.i_cnt > 320){
		int blockAddr = cur.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	if ((cur.i_mode >> 9) == 1){
		//如果是目录，继续dfs；如果是文件，直接free
		DirItem dirlist[32];
		for (int i = 0; i < cur.i_cnt; i++){
			int pos_i = i / 32;
			int pos_j = i % 32;
			if (pos_j == 0) {
				int blockAddr;
				if (pos_i < 10) blockAddr = cur.i_dirBlock[pos_i];
				else blockAddr = indirlist[pos_i - 10];
				fseek(fr, blockAddr, SEEK_SET);
				fread(dirlist, sizeof(dirlist), 1, fr);
			}
			if ((strcmp(dirlist[pos_j].itemName, ".") != 0) && (strcmp(dirlist[pos_j].itemName, "..") != 0))
				rmall(dirlist[pos_j].inodeAddr);
		}
	}
	for (int i = 0; i < 10; i++)
		if (cur.i_dirBlock[i] != -1) {
			bfree(cur.i_dirBlock[i]);
			cur.i_dirBlock[i] = -1;
		}
		else break;
	ifree(address);

}

bool rmdir(int address, char name[])
{
	if (strcmp(name, ".") == 0) return false;
	if (strcmp(name, "..") == 0) return false;
	Inode cur;
	fseek(fr, address, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	DirItem dirlist[32];
	int indirlist[256];
	int blockAddr;
	if (cur.i_cnt > 320){
		blockAddr = cur.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	//get last inodeAddr
	int pos_x = (cur.i_cnt - 1) / 32;
	int pos_y = (cur.i_cnt - 1) % 32;

	if (pos_x < 10) blockAddr = cur.i_dirBlock[pos_x];
	else blockAddr = indirlist[pos_x - 10];

	fseek(fr, blockAddr, SEEK_SET);
	fread(dirlist, sizeof(dirlist), 1, fr);
	int last_inodeAddr = dirlist[pos_y].inodeAddr;
	char last_inodeName[28];
	strcpy(last_inodeName, dirlist[pos_y].itemName);

	for (int i = 0; i < cur.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			if (pos_i < 10) blockAddr = cur.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0){
		 	rmall(dirlist[pos_j].inodeAddr);
			dirlist[pos_j].inodeAddr = last_inodeAddr;
			strcpy(dirlist[pos_j].itemName, last_inodeName);
			//write the dirlist back to disk
			fseek(fw, blockAddr, SEEK_SET);
			fwrite(dirlist, sizeof(dirlist), 1, fw);

			cur.i_cnt--;
			//if the block would be empty after the inode was deleted, we can drop this block
			if (pos_y == 0) {
				if (pos_x < 10){
					bfree(cur.i_dirBlock[pos_x]);
					cur.i_dirBlock[pos_x] = -1;
				}
				else if (pos_x == 10){
					bfree(cur.i_indirBlock_1);
					cur.i_indirBlock_1 = -1;
				}
			}
			cur.i_mtime = time(NULL);
			//rewrite cur_Inode
			fseek(fw, address, SEEK_SET);
			fwrite(&cur, sizeof(Inode), 1, fw);
			fflush(fw);
			return true;
		}
	}
	printf("没有找到名为%s的文件/文件夹\n", name);
	return false;
}

int cd(int address, char name[])
{
	Inode cur;
	fseek(fr, address, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	DirItem dirlist[32];
	int indirlist[256];
	if (cur.i_cnt > 320){
		int blockAddr = cur.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}

	for (int i = 0; i < cur.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			int blockAddr;
			if (pos_i < 10) blockAddr = cur.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0){
			Inode son;
			fseek(fr, dirlist[pos_j].inodeAddr, SEEK_SET);
			fread(&son, sizeof(Inode), 1, fr);
			if ((son.i_mode >> 9) == 1){
				Cur_Dir_Addr = dirlist[pos_j].inodeAddr;
				if (strcmp(name, "..") == 0) Cur_Dir_Num--;
				else if (strcmp(name, ".") != 0){
					strcpy(Cur_Dir_Name[Cur_Dir_Num], name);
					Cur_Dir_Num++;
				}
				return dirlist[pos_j].inodeAddr;
			}
			else {
				printf("%s不是文件夹\n", name);
				return -1;
			}
		}
	}
	printf("没有找到名为%s的文件夹\n", name);
}

bool create(int address, char name[])
{
	if (strlen(name) > MAX_NAME_SIZE){
		printf("名字长度超过限制\n");
		return false;
	}
	//read inode
	Inode father;
	fseek(fr, address, SEEK_SET);
	fread(&father, sizeof(Inode), 1, fr);

	//check the name
	DirItem dirlist[32];
	int indirlist[256];
	if (father.i_cnt > 320){
		int blockAddr = father.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	for (int i = 0; i < father.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			int blockAddr;
			if (pos_i < 10) blockAddr = father.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0) {
			printf("该命名已经存在\n");
			return false;
		}
	}
	//find dirblock's address
	int father_block_addr;
	int pos_x = father.i_cnt / 32;
	int pos_y = father.i_cnt % 32;

	if (father.i_cnt == 320){
		int indirAddr = balloc();
		if (indirAddr == -1){
			printf("空间不足，无法生成一级间接快\n");
			return false;
		}
		father.i_indirBlock_1 = indirAddr;
	}

	if (pos_y == 0){
		father_block_addr = balloc();
		if (father_block_addr == -1) {
			printf("空间不足，无法生成新文件\n");
			return false;
		}
		if (pos_x < 10) father.i_dirBlock[pos_x] = father_block_addr;
		else indirlist[pos_x - 10] = father_block_addr;
	}

	if (pos_x < 10) father_block_addr = father.i_dirBlock[pos_x];
	else father_block_addr = indirlist[pos_x - 10];

	father.i_cnt++;
	father.i_mtime = time(NULL);
	fseek(fw, address, SEEK_SET);
	fwrite(&father, sizeof(Inode), 1, fw);

	//read diritem
	fseek(fr, father_block_addr, SEEK_SET);
	fread(dirlist, sizeof(dirlist), 1, fr);
	Inode cur;
	int inoAddr = ialloc();
	dirlist[pos_y].inodeAddr = inoAddr;
	strcpy(dirlist[pos_y].itemName, name);
	fseek(fw, father_block_addr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	cur.i_id = (inoAddr - superblock->s_Inode_StartAddr) / superblock->s_INODE_SIZE;
	cur.i_ctime = time(NULL);
//	cur.i_atime = time(NULL);
	cur.i_mtime = time(NULL);
//	cur.i_dirBlock[0] = blockAddr;
	for (int i = 0; i < 10; i++)
		cur.i_dirBlock[i] = -1;
	cur.i_mode = MODE_FILE | FILE_DEF_PERMISSION;
	cur.i_size = 0;
	strcpy(cur.i_uname, Cur_User_Name);
	strcpy(cur.i_gname, Cur_Group_Name);
	//rewrite inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);
	return true;
}
bool re_write(int address, char name[], char text[])
{
	Inode father;
	int fileAddr = -1;
	fseek(fr, address, SEEK_SET);
	fread(&father, sizeof(Inode), 1, fr);
	DirItem dirlist[32];
	int indirlist[256];

	if (father.i_cnt > 320){
		int blockAddr = father.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	for (int i = 0; i < father.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
			int blockAddr;
			if (pos_i < 10) blockAddr = father.i_dirBlock[pos_i];
			else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0){
			fileAddr = dirlist[pos_j].inodeAddr;
			break;
		}
	}
	if (fileAddr == -1) {
		printf("没有找到该文件\n");
		return false;
	}
	Inode cur;
	fseek(fr, fileAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	if (cur.i_cnt > 320){
		int blockAddr = father.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}
	if ((cur.i_mode >> 9) == 1){
		printf("该操作只能对文件而不是文件夹\n");
		return false;
	}
	//1个字符1个字节，每个block的大小为1024字节，每个block可以放1024个字符。
	int len = strlen(text);
	char subtext[1024];
	int new_blocks = len / 1024 + 1;
	for (int i = 0; i < new_blocks; i++){
		if (i == len / 1024){
			for (int j = i * 1024; j <= len; j++) subtext[j - i * 1024] = text[j];
		}
		else
			for (int j = 0; j < 1024; j++) subtext[j] = text[j + i * 1024];
		int blockAddr;
		if (i < 10){
			if (cur.i_dirBlock[i] != -1) blockAddr = cur.i_dirBlock[i];
			else {
				blockAddr = balloc();
				cur.i_dirBlock[i] = blockAddr;
			}
		}
		else{
			if (indirlist[i - 10] != -1) blockAddr = indirlist[i - 10];
			else {
				blockAddr = balloc();
				indirlist[i - 10] = blockAddr;
			}
		}
		fseek(fw, blockAddr, SEEK_SET);
		fwrite(subtext, sizeof(subtext), 1, fw);
	}
	for (int i = new_blocks; i < 10; i++){
		if (cur.i_dirBlock[i] != -1){
			bfree(cur.i_dirBlock[i]);
			cur.i_dirBlock[i] = -1;
		}
		else break;
	}
	cur.i_mtime = time(NULL);
	cur.i_size = sizeof(text);
	fseek(fw, fileAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);
}

void add_write(int address, char text[])
{
	Inode cur;
	fseek(fr, address, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	int len = strlen(text);
	char subtext[1024];
	//先取出最后一块，用新的text填满最后这一块，剩下部分往后面添加新的块来填充。
}

bool cat(int address, char name[])
{
	Inode father;
	int fileAddr = -1;
	fseek(fr, address, SEEK_SET);
	fread(&father, sizeof(Inode), 1, fr);
	DirItem dirlist[32];
	int indirlist[256];
	if (father.i_cnt > 320){
        int blockAddr = father.i_indirBlock_1;
		fseek(fr, blockAddr, SEEK_SET);
		fread(indirlist, sizeof(indirlist), 1, fr);
	}

	for (int i = 0; i < father.i_cnt; i++){
		int pos_i = i / 32;
		int pos_j = i % 32;
		if (pos_j == 0) {
            int blockAddr;
            if (pos_i < 10) blockAddr = father.i_dirBlock[pos_i];
            else blockAddr = indirlist[pos_i - 10];
			fseek(fr, blockAddr, SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
		}
		if (strcmp(dirlist[pos_j].itemName, name) == 0){
			fileAddr = dirlist[pos_j].inodeAddr;
			break;
		}
	}
	if (fileAddr == -1) {
		printf("没有找到该文件\n");
		return false;
	}
	Inode cur;
	fseek(fr, fileAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	char subtext[1024];
	int blocks = cur.i_size / superblock->s_BLOCK_SIZE;
	if (cur.i_size != 0) blocks++;
	for (int i = 0; i < blocks; i++){
		int blockAddr;
		if (i < 10)
			blockAddr = cur.i_dirBlock[i];
		else
			blockAddr = indirlist[i - 10];

		fseek(fr, blockAddr, SEEK_SET);
		fread(subtext, sizeof(subtext), 1, fr);
		printf("%s", subtext);
	}
	printf("\n");
	return true;
}

void help()
{
	printf("ls：显示一个目录中的文件和子目录。\n");
	printf("cd：进入目录\n");
	printf("mkdir：创建文件夹\n");
	printf("pwd：查看当前路径\n");
	printf("rm：删除当前目录下某一文件或文件夹\n");
	printf("create：创建文件\n");
	printf("write：写文件\n");
	printf("cat：显示文件内容\n");
	printf("help：帮助\n");
	printf("show_inode_bitmap：查看inode区位图\n");
	printf("show_block_bitmap：查看block区位图\n");
}

void Login()
{
    char name[20], password[20];
    printf("请输入账号：");
    scanf("%s", name);
    printf("请输入密码：");
    scanf("%s", password);
    if ((strcmp(name, "root") == 0) &&(strcmp(name, "root")) == 0) {
        isLogin = true;
        printf("登陆成功\n");
        cd(Root_Dir_Addr, "root");
    }
}

char p1[100], p2[10000], p3[100];
void cmd(char str[])
{
	sscanf(str, "%s", p1);
	if (strcmp(p1, "ls") == 0){
		ls(Cur_Dir_Addr);
	}
	else if (strcmp(p1, "cd") == 0){
		sscanf(str, "%s%s", p1, p2);
		cd(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "mkdir") == 0){
		sscanf(str, "%s%s", p1, p2);
		mkdir(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "pwd") == 0){
		if (Cur_Dir_Num == 0) printf("/");
		else
			for (int i = 0; i < Cur_Dir_Num; i++)
				printf("/%s", Cur_Dir_Name[i]);
		printf("\n");
	}
	else if (strcmp(p1, "rm") == 0){
		sscanf(str, "%s%s", p1, p2);
		rmdir(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "create") == 0){
		sscanf(str, "%s%s", p1, p2);
		create(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "write") == 0){
		sscanf(str, "%s%s%s", p1, p2, p3);
		re_write(Cur_Dir_Addr, p2, p3);
	}
	else if (strcmp(p1, "cat") == 0){
		sscanf(str, "%s%s", p1, p2);
		cat(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "help") == 0){
		help();
	}
	else if (strcmp(p1, "show_inode_bitmap") == 0){
		show_inode_bitmap();
	}
	else if (strcmp(p1, "show_block_bitmap") == 0){
		show_block_bitmap();
	}
	else printf("没有此命令\n");
}
