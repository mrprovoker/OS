#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

long long int count_numbers = 0; // счетчик количества чисел в массиве

typedef enum
{
    ARG_MISS = 0,
    OPEN_OUT_ERR,
	OPEN_INP_ERR,
	NOT_ENOUGH_MEMORY,
	TOO_LARGE_NUMBER,
    WRITE_ERR
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "Должно быть не менее двух аргументов - имен файлов.\n",
    "Нет прав на запись выходного файла или файл уже существует.\n",
    "Не удалось открыть входной файл.\n",
    "Недостаточно памяти для работы программы.\n",
    "Число в файле слишком большое.\n",
    "Не удалось произвести запись файла.\n"
};


int printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}

// Получить дескриптор входного файла, если можно
int getInputFileDescriptor(char *name)
{
    int fd = open(name, O_RDONLY);
    return (fd < 0) ? printError(OPEN_INP_ERR) : fd;
}

// Получить дескриптор выходного файла, если можно
int getOutputFileDescriptor(char *name)
{
    int fd = open(name, O_WRONLY|O_EXCL|O_CREAT, 0666);
    return (fd < 0) ? printError(OPEN_OUT_ERR) : fd;
}

// Функция сравнения по возрастанию
int cmpfunc (const void *a, const void *b)
{
   return (*(long long int*)a - *(long long int*)b);
}

// Получить все числа из файла и добавить в массив
void getNumbers(int fd, long long int* numbers_arr) 
{	
	char cur_number[21];
	int cur_size = 0;
	char byte;	
	
	while (read(fd, &byte, 1) > 0) 
	{
		 if ((byte >= '0' && byte <= '9') || (byte == '-' && cur_size == 0)) 
		 {
			// Накапливаем число
			if (cur_size >= 20) 
			{
				printError(TOO_LARGE_NUMBER);
			};
			
			cur_number[cur_size] = byte;
			cur_size++;
		 }
		 else 
		 {
			if (cur_size > 0 && !(cur_size == 1 && cur_number[0] == '-')) 
			{
				// Добавляем накопленное число в массив
				long long int number = atoll(cur_number);
				count_numbers++;
				numbers_arr = realloc(numbers_arr, count_numbers * sizeof(long long int));
				if (numbers_arr == NULL) 
				{
					printError(NOT_ENOUGH_MEMORY);
				};
				numbers_arr[count_numbers-1] = number;
			};
			cur_size = 0;
			memset(&cur_number[0], 0, sizeof(cur_number));
		 }
	}
	close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printError(ARG_MISS);
    };
	
	// Получение дескрипторов файлов
	int input_fd[argc-2];
    for (int i=0; i < argc-2; input_fd[i] = getInputFileDescriptor(argv[i+1]), i++);
	int output_fd = getOutputFileDescriptor(argv[argc-1]);
	
	// Инициализация массива чисел
	long long int *numbers_arr = (long long int*) malloc(0 * sizeof(long long int));
    if (numbers_arr == NULL)
	{
		printError(NOT_ENOUGH_MEMORY);
    };
	
	// Получение массива чисел из всех файлов
	for (int i=0; i < argc-2; i++) 
	{
		getNumbers(input_fd[i], numbers_arr);
	}

	qsort(numbers_arr, count_numbers, sizeof(long long int), cmpfunc);
	
	// Печать результата в файл
	for (int i=0; i<count_numbers; i++) 
	{
        if (dprintf(output_fd, "%lld\n", numbers_arr[i]) <= 0) 
		{
			printError(WRITE_ERR);
		}
    };
    close(output_fd);
	
    return 0;
};