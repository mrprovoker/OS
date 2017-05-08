#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

long long int count_numbers = 0; // ������� ���������� ����� � �������

typedef enum
{
    ARG_MISS = 0,
    OPEN_OUT_ERR,
	OPEN_INP_ERR,
	NOT_ENOUGH_MEMORY,
	TOO_LARGE_NUMBER,
    WRITE_ERR
} ErrorCode;

// ��������� ���������� ������
const char* errorDesc[] =
{
    "������ ���� �� ����� ���� ���������� - ���� ������.\n",
    "��� ���� �� ������ ��������� ����� ��� ���� ��� ����������.\n",
    "�� ������� ������� ������� ����.\n",
    "������������ ������ ��� ������ ���������.\n",
    "����� � ����� ������� �������.\n",
    "�� ������� ���������� ������ �����.\n"
};


int printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}

// �������� ���������� �������� �����, ���� �����
int getInputFileDescriptor(char *name)
{
    int fd = open(name, O_RDONLY);
    return (fd < 0) ? printError(OPEN_INP_ERR) : fd;
}

// �������� ���������� ��������� �����, ���� �����
int getOutputFileDescriptor(char *name)
{
    int fd = open(name, O_WRONLY|O_EXCL|O_CREAT, 0666);
    return (fd < 0) ? printError(OPEN_OUT_ERR) : fd;
}

// ������� ��������� �� �����������
int cmpfunc (const void *a, const void *b)
{
   return (*(long long int*)a - *(long long int*)b);
}

// �������� ��� ����� �� ����� � �������� � ������
void getNumbers(int fd, long long int* numbers_arr) 
{	
	char cur_number[21];
	int cur_size = 0;
	char byte;	
	
	while (read(fd, &byte, 1) > 0) 
	{
		 if ((byte >= '0' && byte <= '9') || (byte == '-' && cur_size == 0)) 
		 {
			// ����������� �����
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
				// ��������� ����������� ����� � ������
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
	
	// ��������� ������������ ������
	int input_fd[argc-2];
    for (int i=0; i < argc-2; input_fd[i] = getInputFileDescriptor(argv[i+1]), i++);
	int output_fd = getOutputFileDescriptor(argv[argc-1]);
	
	// ������������� ������� �����
	long long int *numbers_arr = (long long int*) malloc(0 * sizeof(long long int));
    if (numbers_arr == NULL)
	{
		printError(NOT_ENOUGH_MEMORY);
    };
	
	// ��������� ������� ����� �� ���� ������
	for (int i=0; i < argc-2; i++) 
	{
		getNumbers(input_fd[i], numbers_arr);
	}

	qsort(numbers_arr, count_numbers, sizeof(long long int), cmpfunc);
	
	// ������ ���������� � ����
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