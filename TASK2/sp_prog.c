#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef enum
{
    ARG_MISS = 0,
    OPEN_ERR,
    WRITE_ERR
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "В аргументах отсутсвует имя файла.\n",
    "Нет прав на запись или файл уже существует.\n",
    "Не удалось произвести запись файла.\n"
};


int printError(ErrorCode c) 
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}

// Получить дескриптор файла, если можно
int getFileDescriptor(char *name) 
{
	int fd = open(name, O_WRONLY|O_EXCL|O_CREAT, 0666);
	return (fd < 0) ? printError(OPEN_ERR) : fd;
}


int main(int argc, char *argv[]) 
{
    if (argc < 2) 
	{
        printError(ARG_MISS);
    };

    int fd = getFileDescriptor(argv[1]);
    char byte;

    // Считываем файл из stdin 
    while (read(STDIN_FILENO, &byte, 1) > 0) 
	{
        if (byte == 0) 
		{
            // Сдвиг, если нулевой байт
            if (lseek(fd, 1, SEEK_CUR) <= 0) 
			{
                printError(WRITE_ERR);
            };
			continue;
        }
		
		if (write(fd, &byte, 1) <= 0) 
		{
			printError(WRITE_ERR);
		}; 
    };

    // Если файл состоит из нулей, нужно хотя бы одним нулевым байтом забить 
    if (byte == 0) 
	{
        if (lseek(fd, -1, SEEK_CUR) <= 0) 
		{
            printError(WRITE_ERR);
        };
        if (write(fd, &byte, 1) <= 0) 
		{
            printError(WRITE_ERR);
        };
    };

    close(fd);
    return 0;
};