#pragma once
#ifndef FILESYSTEM
#define FILESYSTEM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <wait.h>

#define BLOCKSIZE 1024
#define INODENUM 1024
#define BLOCKNUM (1024*64)
#define FILEBLKMAX (1024*4)		//�ļ����Ϊ4M
#define NAMEMAX 20				//������󳤶�
#define PWDMAX 10

#define INODESIZE sizeof(inode)
#define SUPERPOS sizeof(user)			//��0��Ϊ�����飬1-1024Ϊ��ͨ��
#define INODEPOS sizeof(super_block)
#define BLOCKPOS (INODEPOS+INODESIZE*INODENUM)//1025 i�ڵ�֮�����1KB����ͨ��
#define DIRMAXINBLK (BLOCKSIZE / sizeof(directory))	//ÿһ�����ܴ洢��Ŀ¼�����Ŀ
#define DIRMAXNUM (FILEBLKMAX*DIRMAXINBLK)	//ÿһ��Ŀ¼�µ�����ļ���С


typedef struct super_block {
	//�����飬ʹ��λʾͼ������i�ڵ�Ϳ��п�
	int inode_map[INODENUM];//i�ڵ���ʾͼ
	int block_map[BLOCKNUM];
	int inode_free_num;//����i�ڵ���
	int block_free_num;
}super_block;

typedef struct inode {
	//��ͨi�ڵ�
	int block_used[FILEBLKMAX];//��Ÿ��ļ�ռ�õĿ��
	int block_used_num;
	int size;//�ļ���С
	int mode;//�ļ�Ȩ��
	time_t creat_time;//����ʱ��
	time_t modify_time;//���һ���޸ĵ�ʱ��
	int type;//Ϊ0����Ŀ¼��Ϊ1�����ļ�
}inode;

typedef struct directory {
	//Ŀ¼��ṹ
	char name[NAMEMAX];
	int inode_id;//���ļ�����Ӧһ��i�ڵ�ţ��ɴ˵õ��ļ���Ϣ
}directory;

typedef struct user{
	char usrname[NAMEMAX];
	char usrpwd[PWDMAX];
}user;

extern FILE* disk;//ʹ���ļ�ģ�����
extern super_block super;
extern int cur_inode_id;
extern inode cur_inode;
extern directory cur_dir_content[DIRMAXNUM];//���浱ǰĿ¼��Ŀ¼
extern int cur_dir_num;
extern int cur_user_id;

extern char path[128];

/*Func of Inode and Block*/
int alloc_inode();
int free_inode(int ino);
int free_block(int bno);
int init_dir_inode(int new_ino, int ino);
int init_file_node(int new_ino);
int alloc_block();

/*Func of Directory*/
int creat_dir(int ino, char* name, int type);//��Ϊ����Ŀ¼�ʹ����ļ��������ƣ�����һ��
int rm_dir(int ino, char* name);
int cd_dir(int ino, char* name);
int ls_dir();
int open_dir(int ino);
int close_dir(int ino);

/*Func of File*/
int cat_file();
int cp_file(int ino, char* srcfile, char* desfile);
//int mv_file(int ino, char* srcfile, char* desfile);
int open_file(int ino, char* name);
int close_file(int ino, char* name);

/*Func of disk*/
int load_super_block();//����
//void reset_disk();
int fmt_disk();//��ʽ��
int close_disk();//�ر�

void change_path(int old_inode_id, char* name);
int parseline(const char* cmd, char** argv);
#endif // !FILESYSTEM




