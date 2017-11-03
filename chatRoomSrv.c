/* include fig01 */
#include "unp.h"
#define BUFMAX 1024
#define MAX_PARAM_NUM 3  
struct clientProfile {
	char name[14];
	char ip_addr[INET_ADDRSTRLEN];
	int port;
};
//
void who_c(int indexOfSelf,  int *client,  struct clientProfile *clientProfiles, char *sendbuf)
{
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(client[i] != -1)
		{
			char *address = clientProfiles[i].ip_addr;
			int port = clientProfiles[i].port;
			printf("[DEBUF] ip: %s\n", address);
			char *name = clientProfiles[i].name;
			snprintf(sendbuf, BUFMAX, "[Server] %s %s/%d", name, address, port);
			if(i == indexOfSelf)
				strcat(sendbuf, " ->me\n");
			else
				strcat(sendbuf, "\n");
			Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
		}
	}
}
//
void execCommand(char **command, int indexOfSelf, int *client, struct clientProfile *clientProfiles, char *sendbuf)
{
	if(strcmp(command[0], "who") == 0)
	{
		who_c(indexOfSelf, client,  clientProfiles, sendbuf);
	}
	if(strcmp(command[0], "name") == 0)
	{
		;//name_c();
	}
	if(strcmp(command[0], "tell")== 0)
	{
		;//tell_c();
	}
	if(strcmp(command[0], "yell")== 0)
	{
		;//yell_c();
	}
}
//
void sendErrorMsg(int sockfd, char *errorMsg)
{
	if(errorMsg != NULL)
	{
		Writen(sockfd, errorMsg, strlen(errorMsg));
	}
}
//
int checkCommand(int numOfParam, char **command, char **errorMsg)
{
	if(strcmp(command[0], "yell") != 0 && strcmp(command[0], "tell") != 0 && strcmp(command[0], "who") != 0 && strcmp(command[0], "name") != 0)
	{
		char err[] = "[Server] ERROR: Error command\n";
                *errorMsg = malloc(sizeof(char) * strlen(err));
                strcpy(*errorMsg, err);
                return 0;
	}
	if(strcmp(command[0], "who") == 0 && numOfParam != 0 )
	{
		char err[] = "[Server] invalid parameter(s). try 'who'\n";
		*errorMsg = malloc(sizeof(char) * strlen(err));
		strcpy(*errorMsg, err);
		return 0;
	}
	if(strcmp(command[0], "name") == 0 && numOfParam != 1)
	{
		printf("[DEBUG: XXXX]\n");
		fflush(stdout);
                char err[] = "[Server] invalid parameter(s). try 'name <NEW USERNAME>'\n";
                *errorMsg = malloc(sizeof(char) * strlen(err));
                strcpy(*errorMsg, err);
		return 0;
        }
	if(strcmp(command[0], "tell") == 0 && numOfParam != 2)
	{
                char err[] = "[Server] invalid parameter(s). try 'tell <USERNAME> <MESSAGE>'\n";
                *errorMsg = malloc(sizeof(char) * strlen(err));
                strcpy(*errorMsg, err);
		return 0;
        }
	if(strcmp(command[0], "yell") == 0 && numOfParam != 1)
	{
                char err[] = "[Server] invalid parameter(s). try 'yell <MESSAGE>'\n";
                *errorMsg = malloc(sizeof(char) * strlen(err));
                strcpy(*errorMsg, err);
		return 0;
        }
	*errorMsg = NULL;
	return 1;
}
//
//ATTENTION: n is important, to limit the c string end position
int parseCommand(int n, char *recvbuf, char **command)
{
	char *token;
	char *rawData = malloc(sizeof(char) * n);
	int indexOfCommand = -1;
	strncpy(rawData, recvbuf, n);
	char delimiter[] = " \n\r";
	token = strtok(rawData, delimiter);
	while(token != NULL && indexOfCommand < MAX_PARAM_NUM)
	{
		command[++indexOfCommand] = malloc(sizeof(char) * strlen(token));
		strcpy(command[indexOfCommand], token);
		token = strtok(NULL, delimiter);
	}
	return indexOfCommand;
}
//
void sendOfflineMsg(int indexOfSelf, int *client, struct clientProfile *clientProfiles ,char *sendbuf)
{
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(client[i] != -1)
		{
			snprintf(sendbuf, BUFMAX, "[Server] %s is offline\n", clientProfiles[indexOfSelf].name);
        		Writen(client[i], sendbuf, strlen(sendbuf));
		}
	}
}
//
void sendHelloMsg(int indexOfSelf, int *client, struct clientProfile *clientProfiles ,char *sendbuf)
{
	/* write to self */
	char *address = clientProfiles[indexOfSelf].ip_addr;
	int port = clientProfiles[indexOfSelf].port;
	snprintf(sendbuf, BUFMAX, "[Server] Hello, anonymous! From: %s/%d\n", address, port);
	Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
	/* write to others */
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(i != indexOfSelf && client[i] != -1)
		{
			snprintf(sendbuf, BUFMAX, "[Server] Someone is coming!\n");
			Writen(client[i], sendbuf, strlen(sendbuf));
		}
	}
}
//
int main(int argc, char **argv)
{
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t	n;
	fd_set rset, allset;
	char buf[MAXLINE];
	char sendbuf[BUFMAX];
	char recvbuf[BUFMAX];
	struct clientProfile clientProfiles[FD_SETSIZE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(2222);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	maxfd = listenfd; /* initialize */
	maxi = -1; /* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;	/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	for ( ; ; ) {
		rset = allset; /* structure assignment */
		nready = Select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset)) { /* new client connection */
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd; /* save descriptor */
					strcpy(clientProfiles[i].name, "anonymous");
					clientProfiles[i].port = ntohs(cliaddr.sin_port);
                        		inet_ntop(AF_INET, &cliaddr.sin_addr, clientProfiles[i].ip_addr, INET_ADDRSTRLEN);
					break;
				}
			if (i == FD_SETSIZE)
				err_quit("too many clients");
			/* main logic */
			sendHelloMsg(i, client, clientProfiles, sendbuf);
			/* end of main logic */
			FD_SET(connfd, &allset); /* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;	/* for select */
			if (i > maxi)
				maxi = i; /* max index in client[] array */

			if (--nready <= 0)
				continue; /* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = Read(sockfd, recvbuf, BUFMAX)) == 0) { /*connection closed by client */
					Close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					sendOfflineMsg(i, client, clientProfiles, sendbuf);
				} else
				{
					/* main logic */
					char *command[MAX_PARAM_NUM];
					char *errorMsg;
					int numOfParam = parseCommand(n, recvbuf, command);
					if(checkCommand(numOfParam, command, &errorMsg))
					{
						execCommand(command, i, client, clientProfiles,sendbuf);
					}else
					{
						sendErrorMsg(sockfd, errorMsg);
					}
					/* end of main logic */
				}	

				if (--nready <= 0)
					break; /* no more readable descriptors */
			}
		}
	}
}
