#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/wait.h>


const int ROWS = 8;
const int COLS = 8;
const int PORTNO = 15015;

char INIT_FILE[] = "init.txt";
char TEMP_FILE[] = "/tmp/task4.txt";
int field[8][8];

typedef enum
{
    OPEN_INIT_ERR=0,
    INVALID_INIT,
    FORK_ERR,
    OPEN_SOCK_ERR,
    BIND_SOCK_ERR,
    ACCEPT_SOCK_ERR,
    OPEN_TEMP_ERR,
    WRITE_SOCK_ERR,
    WRITE_TEMP_ERR,
    OUT_OF_TIME
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "Не удалось открыть файл инициализации.\n",
    "Файл инициализации содержит неверный символ или не удалось прочитать временный файл\n",
    "Форк не удался.\n",
    "Открыть сокет не удалось.\n",
    "Привязать сокет к порту не удалось.\n",
    "Принять подключение не удалось.\n",
    "Временный файл открыть не удалось.\n",
    "Не удалось записать в сокет.\n",
    "Не удалось записать во временный файл.\n",
    "Функция считает больше секунды.\n"
};


FILE* printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}


// Функция делает шаг игры жизнь
void logicMove() 
{
    time_t start = time(NULL);
    int tmp_field[ROWS][COLS];
    for (int i=0; i<ROWS; i++) 
    {
        for (int j=0; j<COLS; j++) 
        {
            // Считаем количество соседей
            int count = (i>0 && j>0 && field[i-1][j-1] == 1) + (i>0 && field[i-1][j] == 1)
            +(i>0 && j<COLS-1 && field[i-1][j+1] == 1) + (j<COLS-1 && field[i][j+1] == 1)
            +(i<ROWS-1 && j<COLS-1 && field[i+1][j+1] == 1) + (i<ROWS-1 && field[i+1][j] == 1)
            +(i<ROWS-1 && j>0 && field[i+1][j-1] == 1) + (j>0 && field[i][j-1] == 1);

            if (field[i][j] == 0 && count == 3) 
            {
                tmp_field[i][j] = 1;
            } 
            else if (field[i][j] == 1 && (count < 2 || count > 3)) 
            {
                tmp_field[i][j] = 0;
            } 
            else 
            {
                tmp_field[i][j] = field[i][j];
            };
            time_t end = time(NULL);
            double diff_time = difftime(end, start);
            
            if (diff_time >= 1.0) 
            {
                printError(OUT_OF_TIME);
            };
        };
    };
    
    for (int i=0; i<ROWS; i++) 
        for (int j=0; j<COLS; field[i][j] = tmp_field[i][j], j++);

}

// Открыть файл инициализации, если можно
FILE* getInitFile()
{
    FILE *file = fopen(INIT_FILE, "r");
    return (file  == NULL) ? printError(OPEN_INIT_ERR) : file;
}

// Открыть временный файл на запись, если можно
FILE* getTempFileW()
{
    FILE *file = fopen("/tmp/task4.txt", "w");
    return (file  == NULL) ? printError(OPEN_TEMP_ERR) : file;
    
}

// Открыть временный файл на чтение, если можно
FILE* getTempFile()
{
    FILE *file = fopen("/tmp/task4.txt", "r");
    return (file  == NULL) ? printError(OPEN_TEMP_ERR) : file;
}

// Заполнить массив элементами
void initField(FILE* file) 
{
    int symbol;
    for (int i=0; i<ROWS; i++) 
    {
        for (int j=0; j<COLS; j++) 
        {
            symbol = fgetc(file);
            switch(symbol) 
            {
                case '0':
                    field[i][j] = 0;
                    break;
                case '1':
                    field[i][j] = 1;
                    break;
                default:
                    printError(INVALID_INIT);
                    break;
            };
        };
        fgetc(file);
    };
    fclose(file);
}

// Отправить поле клиенту
void writeToNewsock(int sockfd) 
{
    for (int i=0; i<ROWS; i++) 
    {
        for (int j=0; j<COLS; j++) 
        {
            if (write(sockfd, (void*)&field[i][j], 1) < 0)
            {
                printError(WRITE_SOCK_ERR);
            };
        };
        
        if (write(sockfd, "\n", 1) < 0)
        {
            printError(WRITE_SOCK_ERR);
        };
        
    };
    close(sockfd);
}


// Функция раз в секунду пересчитывает поле и записывает в файл
void gameTimer(int sig) 
{
    alarm(1);
    FILE *temp = getTempFileW();
    for (int i=0; i<ROWS; i++) 
    {
        for (int j=0; j<COLS; j++) 
        {
            if (fputc(field[i][j]+48, temp) == EOF) 
            {
                printError(WRITE_TEMP_ERR);
            };
        };
        
        if (fputc('\n', temp) == EOF) 
        {
            printError(WRITE_TEMP_ERR);
        };
    };
    
    fclose(temp);
    logicMove();
    return;
};

int main(int argc, char *argv[]) 
{
    FILE* init = getInitFile();
    initField(init);

    pid_t pid = fork();
    
    switch(pid) 
    {
        case -1:
            printError(FORK_ERR);
            break;
        case 0:;
            int sockfd, newsockfd, clilen;
            struct sockaddr_in serv_addr, cli_addr;
           
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) 
            {
                printError(OPEN_SOCK_ERR);
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(PORTNO);

            if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
            {
                printError(BIND_SOCK_ERR);
            }
            
            listen(sockfd, 5);
            clilen = sizeof(cli_addr);
            while(1) 
            {
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd < 0) 
                {
                    printError(ACCEPT_SOCK_ERR);                
                }
                
                FILE* temp = getTempFile();
                initField(temp);
                writeToNewsock(newsockfd);
            }
            close(sockfd);    
            break;
        default:;
            signal(SIGALRM, gameTimer);
            alarm(1);
            wait(NULL);
            break;            
    }
    
    return 0;
};
