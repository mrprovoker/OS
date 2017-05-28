#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/stat.h>
#include <errno.h>

// Структура процессов
typedef struct 
{
    char command[300];
    char *args[100];
    char filePath[50];
    int type;
    int tries;
    pid_t pid;
} Process;


typedef enum
{
    FORK_ERR=0,
    INVALID_CONF,
    OPEN_ERR,
    EXEC_ERR
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "Форк не удался.\n",
    "Файл конфигурации невалидный.\n",
    "Не удалось открыть файл конфигурации.\n",
    "Exec не удался.\n"
};


FILE* printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}


int syslogError(ErrorCode c) 
{
    syslog(LOG_ERR, errorDesc[c]);
    exit(1);
}

// Получить номер команды
int getCommandType(char symbol) 
{
    if (symbol == 'W') 
    {
        return 0;
    } 
    else if (symbol == 'R') 
    {
        return 1;
    }
    
    return syslogError(INVALID_CONF);
}


void getArguments(Process *pr) 
{
    char fullCommand[297];
    strcpy(fullCommand, pr->command);
    char *buffer = strtok(fullCommand, " ");

    int i = 0;
    for (; i < 99 && buffer != NULL; i++) 
    {
        pr->args[i] = (char *)malloc(strlen(buffer));
        strcpy(pr->args[i], buffer);
        buffer = strtok(NULL, " ");
    };
    pr->args[i] = NULL;
}


// Парсер файла конфига
Process* readConfigFile(FILE *fp, const int linesCount) 
{
    Process *processes = (Process*) malloc(sizeof(Process) * linesCount);

    char line[300] = {'\0'}; 
    for (int i = 0; i < linesCount; i++) 
    {
        fgets(line, 299, fp);
        processes[i].type = getCommandType(line[0]);
        
        char *ch = strchr(line, '\n');
        if (ch != NULL) 
        {
            *ch = '\0'; 
        }
        
        strcpy(processes[i].command, line + 3);
        getArguments(processes + i);
        processes[i].tries = 50;
    }

    return processes;
}



int getCountLines(FILE *fp) {
    int linesCount = 0;
    int symbol;
    while ((symbol = fgetc(fp)) != EOF) 
    {
        if (symbol == '\n') 
        {
            linesCount++;
        };
    };
    fseek(fp, 0, SEEK_SET);

    return linesCount;
}


void processFork(Process *process, int needSleep) 
{
    switch(process->pid = fork()) 
    {
        case -1:
            syslogError(FORK_ERR);
        case 0:
            if (needSleep == 1) 
            {
                sleep(3600);
            };

            if (execvp(process->args[0], process->args) < 0) 
            {
                syslogError(EXEC_ERR);
            };
            exit(0);
            break;
        default:
            sprintf(process->filePath, "/tmp/os5.%d.pid", process->pid);
            
            FILE *fp = fopen(process->filePath, "w");
            if (fp == NULL) 
            {
                syslog(LOG_WARNING, "Ошибка создания файла /tmp/os5.%d.pid", process->pid);
            };
            
            if (fprintf(fp, "%d", process->pid) < 0) 
            {
                syslog(LOG_WARNING, "Ошибка записи в файл /tmp/os5.%d.pid", process->pid);
            };

            fclose(fp);
            break;
    };
}


void watchProcesses(Process *processes, int count) 
{
    while(1) 
    {
        int running = 0;
        for (int i=0; i<count; i++) 
        {
            if (processes[i].pid != 0) 
            {
                running++;
            };
        };
        if (running == 0) 
        {
            break;
        };

        int statusCode;
        pid_t pid = wait(&statusCode);
        for (int i = 0; i < count; i++) 
        {
            if (processes[i].pid == pid) 
            {
                if (remove(processes[i].filePath) < 0) 
                {
                    syslog(LOG_WARNING, "Ошибка удаления файла %s", processes[i].filePath);
                };
                
                if (processes[i].type == 0) 
                {
                    processes[i].pid = 0;
                } 
                else 
                {
                    if (statusCode != 0) 
                    {
                        processes[i].tries -= 1;
                    };
                    int needSleep = 0;
                    if (processes[i].tries < 0) 
                    {
                        processes[i].tries = 50;
                        needSleep = 1;
                        syslog(LOG_NOTICE, "Запуск %s не удался, повторный через 60 минут.", processes[i].command);
                    };
                    processFork(processes + i, needSleep);
                }
                break;
            };
        };
    };
}


int deamonLogic()
{
    FILE *fp = fopen("config", "r");
    if (fp == NULL) 
    {
        printError(OPEN_ERR);
    };
    
    const int linesCount = getCountLines(fp);
    Process *processes = readConfigFile(fp, linesCount);
    
    fclose(fp);
    
    for (int i = 0; i < linesCount; i++) 
    {
        processFork(processes + i, 0);
    };
    
	// Обработчик сигнала HUP
    void HUPHandler(int signal) 
    {
        for (int i = 0; i < linesCount; i++) 
        {
            if (processes[i].pid > 0) 
            {
                kill(processes[i].pid, SIGKILL);
                if (remove(processes[i].filePath) < 0) 
                {
                    syslog(LOG_WARNING, "Ошибка удаления файла %s", processes[i].filePath);
                };
            };
        };
        deamonLogic();
    };
    signal(SIGHUP, HUPHandler);

    watchProcesses(processes, linesCount);
    exit(0);
}


int main(int argc, char *argv[]) 
{
    switch (fork()) 
    {
        case -1:
            printError(FORK_ERR);
        case 0:
            umask(0);
            setsid();
            chdir("/");
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            deamonLogic();
            break;
    }
    return 0;
}
