#include "filesystem.h"
int execute(char* func,int argc,char** argv);
int main(void) {
	disk = fopen("my_disk", "r+");//ʹ��һ���ļ�ģ�����
	if (disk == NULL) {
		printf("Disk file not exist\n");
		return -1;
	}
	char cmd[1024];
	//�ն���ʾ
	int ans = load_super_block();
	if (ans == -1)
		return -1;
	printf("-------------------MY FILE SYSTEM--------------------\n\n");
	while (1) {
		printf("%s", path);
		fflush(stdout);
		if ((fgets(cmd, 1024, stdin) == NULL) && ferror(stdin)) {
			printf("fgets error\n");
			return -1;
		}
		if (feof(stdin)) {
			printf("Exit My File System\n");
			fflush(stdout);
			exit(0);
		}
		char* argv[128];
		int argc = parseline(cmd, argv);
		if (argv[0] == NULL)
			return -1;
		//printf("argv[0]:%s,argv[1}:%s\n",argv[0],argv[1]);
		if (!built_cmd(argv)) {
			if (!execute(argv[0], argc, argv))
				printf("command can't find\n");
		}
	}

}
int execute(char* func, int argc, char* argv[]) {
	if (strcmp(func, "mkdir") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int ans=creat_dir(cur_inode_id, argv[1], 0);
		if (ans == -1)
			return -1;
		return 1;
	}
	else if (strcmp(func, "rmdir") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int ans = rm_dir(cur_inode_id, argv[1]);
		if (ans == -1)
			return -1;
		return 1;
	}
	else if (strcmp(func, "cd") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int old_inode = cur_inode_id;
		int ans=cd_dir(cur_inode_id, argv[1]);
		if (ans == -1)
			return -1;
		change_path(old_inode, argv[1]);
		return 1;
	}
	else if (strcmp(func, "ls") == 0) {
		if (argc != 1) {
			printf("Format error\n");
			return -1;
		}
		int ans=ls_dir();
		if (ans == -1)
			return -1;
		return 1;
	}
	else if (strcmp(func, "touch") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int ans=creat_dir(cur_inode_id, argv[1], 1);
		if (ans == -1)
			return -1;
		return 1;
	}
	else if (strcmp(func, "rm") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int ans=rm_dir(cur_inode_id, argv[1]);
		if (ans == -1)
			return 1;
		return 1;
	}
	else if (strcmp(func, "vim") == 0) {
		int pid, status;
		char* vim_arg[] = { "vim","my_tmp_disk",NULL };
		int ans=open_file(cur_inode_id, argv[1]);
		if (ans == -1)
			return -1;
		if ((pid = fork()) == 0)
			execvp("vim", vim_arg);
		wait(&status);
		close_file(cur_inode_id, argv[1]);
		return 1;
	}
	else if (strcmp(func, "cat") == 0) {
		if (argc != 2) {
			printf("Format error\n");
			return -1;
		}
		int ans = open_file(cur_inode_id, argv[1]);
		if (ans == 1) {
			cat_file();
			close_file(cur_inode_id, argv[1]);
			return 1;
		}
		return -1;	
	}
	else if (strcmp(func, "cp") == 0) {
		if (argc != 3) {
			printf("Format error\n");
			return -1;
		}
		int ans = cp_file(cur_inode_id, argv[1], argv[2]);
		return 1;
	}
	return 0;
}

int built_cmd(char* argv[]) {
	if (strcmp(argv[0], "fmt") == 0) {
		//if (cur_user_id == 0)
			fmt_disk();
			return 1;
	}
	else if (strcmp(argv[0], "exit") == 0) {
		close_disk();
		fclose(disk);
		exit(0);
	}
	return 0;
}






