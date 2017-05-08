#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

const int PORTNO = 15015;

typedef enum
{
	OPEN_SOCK_ERR=0,
	GET_HOST_ERR,
	CONNECT_SOCK_ERR
} ErrorCode;

// Текстовое содержание ошибок
const char* errorDesc[] =
{
    "Открыть сокет не удалось.\n",
    "Получить хост не удалось.\n",
    "Подключение не удалось.\n"
};


int printError(ErrorCode c)
{
    fprintf(stderr, "%s", errorDesc[c]);
    exit(1);
}


int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		printError(OPEN_SOCK_ERR);
	}

    server = gethostbyname("localhost");
    if (server == NULL) 
	{
		printError(GET_HOST_ERR);
    };

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORTNO);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
	{
		printError(CONNECT_SOCK_ERR);
    }

    char symbol[2];
    while (read(sockfd, symbol, 1) > 0) 
	{
        symbol[1] = 0;
        if (symbol[0] == 1 || symbol[0] == 0) 
		{
            printf("%d", symbol[0]);
        } 
		else 
		{
            printf("%s", symbol);
        };
    };
    close(sockfd);

    return 0;
};