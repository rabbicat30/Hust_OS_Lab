#include "filesystem.h"

FILE* disk;//使用文件模拟磁盘
super_block super;
int cur_inode_id;
inode cur_inode;
directory cur_dir_content[DIRMAXNUM];
int cur_dir_num;
int cur_user_id;
char path[128];

int alloc_block() {
	//创建普通块
	int i;
	if (super.block_free_num <= 0) {
		printf("Allocte block failed\n");
		return -1;
	}
	for (i = 0; i < BLOCKNUM; i++) {
		if (super.block_map[i] == 0) {
			super.block_map[i] = 1;
			break;
		}
	}
	super.block_free_num--;
	return i;
}

int free_block(int bno) {
	//释放块
	super.block_free_num++;
	super.block_map[bno] = 0;
	return 1;
}

int alloc_inode() {
	//分配i节点
	//返回分配的i节点号
	int i;
	if (super.inode_free_num <= 0) {
		printf("Allocte inode failed\n");
		return -1;
	}
	for (i = 0; i < INODENUM; i++) {
		if (super.inode_map[i] == 0) {
			//当前的inode为空闲
			super.inode_map[i] = 1;
			break;
		}
	}
	super.inode_free_num--;
	return i;
}

int free_inode(int ino) {
	//释放i节点,回收本身的i节点块，同时，回收其所指向的所有普通块
	inode node;
	fseek(disk, INODEPOS + INODESIZE * ino, 0);//找到要释放的i节点的位置
	fread(&node, sizeof(node), 1, disk);//查询i节点信息
	//释放该i节点指向的所有普通块
	for (int i = 0; i < node.block_used_num; i++)
		free_block(node.block_used[i]);
	//回收本i节点
	super.inode_map[ino] = 0;
	super.inode_free_num++;
	return 1;
}

int init_dir_inode(int new_ino, int ino) {
	//目录节点初始化，参数：当前目录项的i节点号，父目录的i节点号
	inode node;
	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fread(&node, sizeof(inode), 1, disk);

	int bno = alloc_block();
	node.block_used[0] = bno;//需要用到一个块保存当前目录和父目录信息
	node.block_used_num = 1;
	time_t timer;
	time(&timer);
	node.creat_time = node.modify_time = timer;
	//node.mode = oct2dec(1755);//权限
	node.size = 2 * sizeof(directory);
	node.type = 0;

	//向所在的i节点中写入i节点的信息
	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fwrite(&node, sizeof(inode), 1, disk);

	directory dirlink[2];
	strcpy(dirlink[0].name, ".");
	dirlink[0].inode_id = new_ino;
	strcpy(dirlink[1].name, "..");
	dirlink[1].inode_id = ino;

	//向所在的块中写入信息: . ..
	fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
	fwrite(dirlink, sizeof(directory), 2, disk);
	return 1;
}

int init_file_inode(int new_ino) {
	//初始化文件i节点信息，和目录i节点类似，但是没有. 和..
	inode node;
	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fread(&node, sizeof(inode), 1, disk);

	node.block_used_num = 0;
	time_t timer;
	time(&timer);
	node.creat_time = node.modify_time = timer;
	//node.mode = oct2dec(644);
	node.size = 0;
	node.type = 1;

	//向所在的i节点中写入i节点的信息
	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fwrite(&node, sizeof(inode), 1, disk);
	return 1;
}

int creat_dir(int ino, char* name, int type) {
	//创建目录
	//参数：父目录i结点号，创建的目录名，类型（为0，代表创建目录，为1代表创建文件）
	if (cur_dir_num >= DIRMAXNUM) {
		printf("Dirctory is full\n");
		return -1;
	}
	//检查要创建的目录名字是否重名
	for (int i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, name) == 0) {
			printf("Directory Exist\n");
			return -1;
		}
	}
	//如果当前块已满，需要开辟新的块存放
	int block_need = 1;
	if (cur_dir_num / DIRMAXINBLK != (cur_dir_num + 1) / DIRMAXINBLK)
		block_need++;
	if (block_need > super.block_free_num) {
		printf("The block num is less\n");
		return -1;
	}
	if (block_need == 2)
		cur_inode.block_used[++cur_inode.block_used_num] = alloc_block();

	int new_ino = alloc_inode();
	if (new_ino == -1) {
		printf("When create dir, alloc inode failed\n");
		return -1;
	}

	if (type == 0)
		init_dir_inode(new_ino, ino);
	else
		init_file_inode(new_ino);

	//初始化创建的新节点
	cur_dir_content[cur_dir_num].inode_id = new_ino;
	strcpy(cur_dir_content[cur_dir_num].name, name);
	time_t timer;
	time(&timer);
	cur_inode.modify_time = timer;
	cur_dir_num++;
	return 1;
}

int rm_dir(int ino, char* name) {
	//删除目录
	//检查输入的目录是否是当前目录.或者父目录..
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		printf("Can't delete . and ..\n");
		return -1;
	}
	//检查输入的名字是否存在
	int i;
	for (i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, name) == 0)
			break;
	}
	if (i == cur_dir_num) {
		printf("File Not Exist\n");
		return -1;
	}
	int rm_inode = cur_dir_content[i].inode_id;//要删除目录对应的i节点号
	inode node;
	fseek(disk, INODEPOS + INODESIZE * rm_inode, 0);
	fread(&node, sizeof(inode), 1, disk);

	if (node.type == 0) {
		//如果是目录
		cd_dir(ino, name);//在当前工作目录下，打开要删除的文件目录
		if (cur_dir_num != 2) {//要删除的子目录中还含有文件，不删除
			cd_dir(rm_inode, (char*)"..");
			printf("Dir has Files\n");
			return -1;
		}
		cd_dir(rm_inode, (char*)"..");
	}

	int pos;
	free_inode(rm_inode);

	//修改cur_dir_content内容，所删除位置之后的内容全部往前移
	for (pos = 0; pos < cur_dir_num; pos++) {
		if (strcmp(cur_dir_content[pos].name, name) == 0)
			break;
	}
	for (; pos < cur_dir_num - 1; pos++)
		cur_dir_content[pos] = cur_dir_content[pos + 1];
	cur_dir_num--;

	if (cur_dir_num / DIRMAXINBLK != (cur_dir_num - 1) / DIRMAXINBLK) {
		//如果要删除的文件所在的块只有他一项，则连同块本身一起回收
		cur_inode.block_used_num--;
		free_block(cur_inode.block_used[cur_inode.block_used_num]);
	}

	//更新时间
	time_t timer;
	time(&timer);
	cur_inode.modify_time = timer;
	return 1;
}

int cd_dir(int ino, char* name)
{
	//打开目录

	//检查输入是否存在
	int i;
	for (i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, name) == 0)
			break;
	}
	if (i == cur_dir_num) {
		printf("Dir Not Exist\n");
		return -1;
	}

	int cd_node = cur_dir_content[i].inode_id;
	inode node;
	fseek(disk, INODEPOS + cd_node * INODESIZE, SEEK_SET);
	fread(&node, sizeof(node), 1, disk);
	if (node.type != 0) {
		printf("Is Not a Dir\n");
		return -1;
	}

	close_dir(ino);
	cur_inode_id = cd_node;
	open_dir(cd_node);
	return 1;
}

int ls_dir() {
	//show
	//保存状态
	close_dir(cur_inode_id);
	open_dir(cur_inode_id);

	int i;
	inode node;
	for (i = 0; i < cur_dir_num; i++) {
		fseek(disk, INODEPOS + INODESIZE * cur_dir_content[i].inode_id, 0);
		fread(&node, sizeof(node), 1, disk);
		if (node.type == 0)
			printf("\e[1;34m%s\t\e[0m", cur_dir_content[i].name);
		else
			printf("%s\t", cur_dir_content[i].name);
	}
	printf("\n");
	return 1;
}

int open_dir(int ino) {
	//打开一个目录
	fseek(disk, INODEPOS + INODESIZE * ino, 0);
	int fd = fread(&cur_inode, sizeof(inode), 1, disk);
	if (fd != 1) {
		printf("Open inode failed\n");
		return -1;
	}

	//读取该目录下的所有目录项
	int i;
	for (i = 0; i < cur_inode.block_used_num - 1; i++) {
		fseek(disk, BLOCKPOS + BLOCKSIZE * cur_inode.block_used[i], 0);
		fread(cur_dir_content + i * DIRMAXINBLK, sizeof(directory), DIRMAXINBLK, disk);
	}
	//最后一个块不一定是满的，需要单独列出
	int end_block_dirnum = cur_inode.size / sizeof(directory) - DIRMAXINBLK * (cur_inode.block_used_num - 1);
	fseek(disk, BLOCKPOS + BLOCKSIZE * cur_inode.block_used[i], 0);
	fread(cur_dir_content + i * DIRMAXINBLK, sizeof(directory), end_block_dirnum, disk);

	//修改时间
	time_t timer;
	time(&timer);
	cur_inode.modify_time = timer;
	cur_dir_num = i * DIRMAXINBLK + end_block_dirnum;
	return 1;
}

int close_dir(int ino) {
	//关闭目录，并将指向的普通块的内容写回到磁盘中
	//和open类似，只是一个read一个write
	int i;
	for (i = 0; i < cur_inode.block_used_num - 1; i++) {
		fseek(disk, BLOCKPOS + cur_inode.block_used[i] * BLOCKSIZE, SEEK_SET);
		fwrite(cur_dir_content + i * DIRMAXINBLK, sizeof(directory), DIRMAXINBLK, disk);//fwrite
	}
	int end_block_dirnum = cur_dir_num - i * DIRMAXINBLK;
	fseek(disk, BLOCKPOS + cur_inode.block_used[i] * BLOCKSIZE, SEEK_SET);
	fwrite(cur_dir_content + i * DIRMAXINBLK, sizeof(directory), end_block_dirnum, disk);

	cur_inode.size = cur_dir_num * sizeof(directory);
	fseek(disk, INODEPOS + ino * INODESIZE, 0);
	int fd = fwrite(&cur_inode, sizeof(inode), 1, disk);
	if (fd != 1) {
		printf("Open current inode failed\n");
		return -1;
	}
	return 1;
}

/*Func of File*/
int open_file(int ino, char* name) {
	//打开文件，将其内容输出到tmp file中
	char block[BLOCKSIZE];
	FILE* pbuf = fopen("my_tmp_disk", "w+");

	//检查要打开的文件是否存在
	int i;
	for (i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, name) == 0)
			break;
	}
	if (i == cur_dir_num) {
		printf("File Not Exist\n");
		return -1;
	}
	int open_inode = cur_dir_content[i].inode_id;//找到对应的i结点
	inode node;
	fseek(disk, INODEPOS + INODESIZE * open_inode, 0);
	fread(&node, sizeof(inode), 1, disk);

	if (node.type == 0) {
		printf("It's a dir instead of a file\n");
		return -1;
	}
	if (node.size == 0) {
		//文件为空,不需要打开
		fclose(pbuf);
		return 1;
	}

	int bno;
	//从磁盘中读取内容，类似于open_dir
	for (i = 0; i < node.block_used_num - 1; i++) {
		memset(block, 0, BLOCKSIZE);	//务必先清零
		bno = node.block_used[i];		//得到对应的块号
		fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
		fread(block, sizeof(char), BLOCKSIZE, disk);//从磁盘中读取块的内容
		fwrite(block, sizeof(char), BLOCKSIZE, pbuf);//将内容复制到pbuf缓冲中
		free_block(bno);//读取完内容后，回收块
		node.size -= BLOCKSIZE;
	}
	//最后一个块，读取实际长度
	bno = node.block_used[i];
	fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
	fread(block, sizeof(char), node.size, disk);//从磁盘中读取块的内容
	fwrite(block, sizeof(char), node.size, pbuf);//将内容复制到pbuf缓冲中
	free_block(bno);//读取完内容后，回收块
	node.size = 0;
	node.block_used_num = 0;

	//保存结点
	fseek(disk, INODEPOS + open_inode * INODESIZE, 0);
	fwrite(&node, sizeof(node), 1, disk);

	fclose(pbuf);
	return 1;
}

int close_file(int ino, char* name) {
	//关闭文件，并将内容写回磁盘中
	char block[BLOCKSIZE];
	FILE* pbuf = fopen("my_tmp_disk", "r");

	//检查要打开的文件是否存在
	int i;
	for (i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, name) == 0)
			break;
	}
	if (i == cur_dir_num) {
		printf("File Not Exist\n");
		return -1;
	}
	int close_inode = cur_dir_content[i].inode_id;//找到对应的i结点
	inode node;
	fseek(disk, INODEPOS + INODESIZE * close_inode, 0);
	fread(&node, sizeof(inode), 1, disk);

	if (node.type == 0) {
		printf("It's a dir instead of a file\n");
		return -1;
	}

	int bno;
	memset(block, 0, BLOCKSIZE);
	int readsize = fread(block, sizeof(char), BLOCKSIZE, pbuf);
	while (readsize != 0) {
		bno = alloc_block();
		if (bno == -1) {
			printf("Allocate block failed\n");
			return -1;
		}
		fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
		fwrite(block, sizeof(char), BLOCKSIZE, disk);
		node.block_used[node.block_used_num] = bno;
		node.size += readsize;
		node.block_used_num++;

		memset(block, 0, BLOCKSIZE);
		readsize = fread(block, sizeof(char), BLOCKSIZE, pbuf);
	}

	fseek(disk, INODEPOS + close_inode * INODESIZE, 0);
	fwrite(&node, sizeof(inode), 1, disk);
	fclose(pbuf);
	return 1;
}

int cat_file() {
	//显示文件内容
	char block[BLOCKSIZE];
	FILE* pbuf = fopen("my_tmp_disk", "r");
	memset(block, 0, BLOCKSIZE);

	//从缓冲区文件中读取内容
	int readsize;
	while ((readsize = fread(block, sizeof(char), BLOCKSIZE, pbuf) != 0))
		printf("%s", block);
	fclose(pbuf);
	return 1;
}

int cp_file(int ino, char* srcfile, char* desfile) {
	//从srcfile复制到desfile
	char block[BLOCKSIZE];
	FILE* pbuf = fopen("my_tmp_disk", "w+");

	//检查文件是否存在
	int i;
	for (i = 0; i < cur_dir_num; i++) {
		if (strcmp(cur_dir_content[i].name, srcfile) == 0)
			break;
	}
	if (i == cur_dir_num) {
		printf("File Not Exist\n");
		return -1;
	}

	int src_inode = cur_dir_content[i].inode_id;
	inode node;
	fseek(disk, INODEPOS + INODESIZE * src_inode, 0);
	fread(&node, sizeof(inode), 1, disk);
	if (node.type == 0) {
		printf("It's a dir instead of a file\n");
		return -1;
	}
	if (node.size == 0) {
		fclose(pbuf);
		return 1;
	}
	int bno;
	for (i = 0; i < node.block_used_num - 1; i++) {
		memset(block, 0, BLOCKSIZE);	//务必先清零
		bno = node.block_used[i];		//得到对应的块号
		fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
		fread(block, sizeof(char), BLOCKSIZE, disk);//从磁盘中读取块的内容
		fwrite(block, sizeof(char), BLOCKSIZE, pbuf);//将内容复制到pbuf缓冲中
	}
	//最后一个块，读取实际长度
	bno = node.block_used[i];
	fseek(disk, BLOCKPOS + BLOCKSIZE * bno, 0);
	fread(block, sizeof(char), node.size, disk);//从磁盘中读取块的内容
	fwrite(block, sizeof(char), node.size, pbuf);//将内容复制到pbuf缓冲中
	fclose(pbuf);

	if (desfile[strlen(desfile) - 1] == '/') {//复制到其他目录下,此时desfile是一个目录名
		desfile[strlen(desfile) - 1] = '\0';
		int despos;
		//检查要转的文件是否已经存在
		for (despos = 0; despos < cur_dir_num; despos++) {
			if (strcmp(cur_dir_content[despos].name, desfile) == 0)
				break;
		}
		if (despos == cur_dir_num) {
			printf("Desfile Not Exist\n");
			return -1;
		}
		int desnode = cur_dir_content[despos].inode_id;
		inode des_inode;
		fseek(disk, INODEPOS + INODESIZE * desnode, 0);
		fread(&des_inode, sizeof(inode), 1, disk);

		if (des_inode.type == 1) {
			printf("The desfile is a file instead of a dir\n");
			return -1;
		}
		//进入到要复制的目录
		cd_dir(cur_inode_id, desfile);
		for (int i = 0; i < cur_dir_num; i++) {
			if (strcmp(cur_dir_content[i].name, srcfile) == 0) {
				//如果要粘贴的目录下存在srcfile文件，再次复制失败
				close_dir(cur_inode_id);
				cur_inode_id = ino;//回到原来目录
				open_dir(ino);
				printf("The des dir has had the srcfile\n");
				return -1;
			}
		}
		close_dir(cur_inode_id);
		cur_inode_id = ino;
		open_dir(ino);

		//创建新的目录项
		cd_dir(cur_inode_id, desfile);
		printf("when create new directory, ls:\n");
		ls_dir();
		creat_dir(cur_inode_id, srcfile, 1);
		printf("after creat new directory, ls:\n");
		ls_dir();
		close_file(cur_inode_id, srcfile);//保存
		printf("after close_file(cur_inode_id, srcfile), ls:\n");
		ls_dir();

		close_dir(cur_inode_id);
		cur_inode_id = ino;
		open_dir(ino);	//回到src目录下
	}
	else {
		//目的在当前目录下，直接创建
		for (int i = 0; i < cur_dir_num; i++) {
			if (strcmp(cur_dir_content[i].name, srcfile) == 0) {
				//如果要粘贴的目录下存在srcfile文件，再次复制失败
				printf("The des dir has had the srcfile\n");
				return -1;
			}
		}
		creat_dir(ino, desfile, 1);
		close_file(ino, desfile);
	}
	return 1;
}

int init_file_node(int new_ino) {
	inode node;
	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fread(&node, sizeof(inode), 1, disk);

	node.block_used_num = 0;
	time_t timer;
	time(&timer);
	node.creat_time = node.modify_time = timer;
	node.size = 0;
	node.type = 1;

	fseek(disk, INODEPOS + INODESIZE * new_ino, 0);
	fwrite(&node, sizeof(inode), 1, disk);
	return 1;
}

int load_super_block() {
	//从磁盘中加载超级管理块
	fseek(disk, SUPERPOS, 0);
	int fd = fread(&super, sizeof(super_block), 1, disk);
	if (fd != 1) {
		printf("Load super block failed\n");
		return -1;
	}
	cur_inode_id = 0;
	fd = open_dir(cur_inode_id);//打开根目录
	if (fd != 1) {
		printf("Super block open failed\n");
		return -1;
	}
	return 1;
}

int fmt_disk() {
	//格式化
	memset(super.inode_map, 0, sizeof(super.inode_map));
	memset(super.block_map, 0, sizeof(super.block_map));
	super.block_free_num = BLOCKNUM - 1;//除去管理块
	super.inode_free_num = INODENUM - 1;
	super.block_map[0] = 1;
	super.inode_map[0] = 1;

	cur_inode_id = 0;
	fseek(disk, INODEPOS, 0);
	int fd = fread(&cur_inode, sizeof(inode), 1, disk);
	if (fd != 1) {
		printf("When fmt disk, open cur_inode failed\n");
		return -1;
	}
	cur_inode.block_used[0] = 0;
	cur_inode.block_used_num = 1;
	time_t timer;
	time(&timer);
	cur_inode.creat_time = cur_inode.modify_time = timer;
	cur_inode.size = 2 * sizeof(directory);
	cur_inode.type = 0;

	cur_dir_num = 2;
	strcpy(cur_dir_content[0].name, (char*)".");
	strcpy(cur_dir_content[1].name, (char*)"..");
	cur_dir_content[0].inode_id = 0;
	cur_dir_content[1].inode_id = 0;//?

	strcpy(path, "root@localhost: / >");
	return 1;
}

int close_disk() {
	fseek(disk, SUPERPOS, 0);
	int fd = fwrite(&super, sizeof(super_block), 1, disk);
	if (fd != 1) {
		printf("Close super block failed\n");
		return -1;
	}
	//cur_inode_id = 0;
	fd = close_dir(cur_inode_id);//打开根目录
	if (fd != 1) {
		printf("Super block close failed\n");
		return -1;
	}
	return 1;
}

void change_path(int old_inode_id, char* name) {
	//当更换目录时，改变路径
	int pos;
	if (!strcmp(name, ".") || (!strcmp(name, "..") && (old_inode_id == 0)))
		return;
	else if (!strcmp(name, "..") && cur_inode_id != 0) {
		for (pos = strlen(path) - 1; pos >= 0; pos--) {
			if (path[pos] == '/') {
				path[pos] = '\0';
				strcat(path, " >");
				break;
			}
		}
	}
	else if (!strcmp(name, "..") && cur_inode_id == 0) {
		for (pos = strlen(path) - 1; pos >= 0; pos--) {
			if (path[pos] == '/') {
				path[pos + 1] = '\0';
				strcat(path, " >");
				break;
			}
		}
	}
	else if (path[strlen(path) - 3] == '/') {
		path[strlen(path) - 2] = '\0';
		strcat(path, name);
		strcat(path, " >");
	}
	else {
		path[strlen(path) - 2] = '\0';
		strcat(path, "/");
		strcat(path, name);
		strcat(path, " >");
	}
}

int parseline(const char* cmd, char** argv) {
	/* Holds local copy of command line */
	static char array[1024];
	char* buf = array;
	char* delim;
	int argc;

	strcpy(buf, cmd);
	buf[strlen(buf) - 1] = ' ';
	/* Ignore leading spaces */
	while (*buf && (*buf == ' '))
		buf++;

	/* Build the argv list */
	argc = 0;
	if (*buf == '\'') {
		buf++;
		delim = strchr(buf, '\'');
	}
	else {
		delim = strchr(buf, ' ');
	}

	while (delim) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' '))
			buf++;

		if (*buf == '\'') {
			buf++;
			delim = strchr(buf, '\'');
		}
		else {
			delim = strchr(buf, ' ');
		}
	}
	argv[argc] = NULL;
	return argc;
}


