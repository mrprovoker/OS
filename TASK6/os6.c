#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

int SLEEP_TIME = 5;

typedef enum
{
    ARG_MISS=0,
    LOCK_WRITE_ERR,
	OPEN_ERR,
	WRITE_ERR,
	RMV_LOCK_ERR
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "Программа принимает на вход 2 аргумента (имя файла и сообщение для записи).\n",
    "Ошибка при записи в файл блокировки.\n",
    "Не удалось открыть файл.\n",
    "Не удалось записать файл.\n",
    "Не удалось удалить файл блокировки.\n"
};


FILE* printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}

// Получить полное имя файла блокировки
char* getLockerName(char* name) 
{
	static char path[100];
	strcpy(path, name);
	strcat(path, ".lck");
	
	return path;
}

// Подождать пока файл освободится и создать блокировку
void waitAndLockFile(char *locker_name) 
{
	while(1) 
	{
		int fd = open(locker_name, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if (fd >= 0) 
		{
            pid_t pid = getpid();
            if (dprintf(fd, "W%d\n", pid) < 0) 
			{
				printError(LOCK_WRITE_ERR);
            };
            close(fd);
            break;
        };
   }
}


int main(int argc, char *argv[]) 
{	
	if (argc != 3)
    {
        printError(ARG_MISS);
    };
	
	char* locker_name = getLockerName(argv[1]);  
	waitAndLockFile(locker_name);
	
    FILE *fp = fopen(argv[1], "a");
    if (fp == NULL) 
	{
		printError(OPEN_ERR);
    };
	
	sleep(SLEEP_TIME);  // создать видимость работы с файлом
	
    if (fprintf(fp, "%s\n", argv[2]) < 0) 
	{
		printError(WRITE_ERR);
    };
	fclose(fp);
	
    if (remove(locker_name) < 0) 
	{
		printError(RMV_LOCK_ERR);
    };

    return 0;
};