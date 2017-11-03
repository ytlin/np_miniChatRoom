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
void yell_c(char *msg, int indexOfSelf, int *client,  struct clientProfile *clientProfiles, char *sendbuf)
{
	snprintf(sendbuf, BUFMAX, "[Server] %s yell %s\n", clientProfiles[indexOfSelf].name, msg);
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(client[i] != -1)
			Writen(client[i], sendbuf, strlen(sendbuf));
	}
}
//
void tell_c(char *target, char *msg, int indexOfSelf,  int *client,  struct clientProfile *clientProfiles, char *sendbuf)
{
	/* check name and exitence */
	if(strcmp(clientProfiles[indexOfSelf].name, "anonymous") == 0)
        {
                snprintf(sendbuf, BUFMAX, "[Server] ERROR: You are anonymous\n");
                Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
                return;
        }
	if(strcmp(target, "anonymous") == 0)
        {
                snprintf(sendbuf, BUFMAX, "[Server] ERROR: The client to which you sent is anonymous\n");
                Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
                return;
        }
	int foundFlg = 0;
	int target_fd;
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(client[i] != -1 && strcmp(clientProfiles[i].name, target) ==0)
		{
			target_fd = client[i];
			foundFlg = 1;
			break;
		}
	}
	if(!foundFlg)
	{
		snprintf(sendbuf, BUFMAX, "[Server] ERROR: The receiver doesn't exist\n");
                Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
                return;
	}
	/* let's send the msg and notice to self*/
	snprintf(sendbuf, BUFMAX, "[Server] %s tell you %s\n", clientProfiles[indexOfSelf].name, msg);
        Writen(target_fd, sendbuf, strlen(sendbuf));
	snprintf(sendbuf, BUFMAX, "[Server] SUCCESS: Your message has been sent\n");
        Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
}
//
void name_c(char *nameToChange, int indexOfSelf,  int *client,  struct clientProfile *clientProfiles, char *sendbuf)
{
	/* check name format */
	if(strcmp(nameToChange, "anonymous") == 0)	
	{
		snprintf(sendbuf, BUFMAX, "[Server] ERROR: Username cannot be anonymous\n");
		Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
		return;
	}
	int lenOfNameToChange = strlen(nameToChange);
	if(lenOfNameToChange < 2 || lenOfNameToChange > 12)
	{
		snprintf(sendbuf, BUFMAX, "[Server] ERROR: Username can only consists of 2~12 English letters\n");
                Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
                return;
	}
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(i != indexOfSelf && client[i] != -1 && strcmp(nameToChange, clientProfiles[i].name) == 0)
		{
			snprintf(sendbuf, BUFMAX, "[Server] ERROR: %s has been used by others\n", nameToChange);
                	Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
                	return;
		}
	}
	/* legal name format, time to change it */
	snprintf(sendbuf, BUFMAX, "[Server] You're now known as %s\n", nameToChange);
        Writen(client[indexOfSelf], sendbuf, strlen(sendbuf));
	for(int i=0;i<FD_SETSIZE;i++)
        {
		if(i != indexOfSelf && client[i] != -1)
		{
			snprintf(sendbuf, BUFMAX, "[Server] %s is now known as %s\n", clientProfiles[indexOfSelf].name, nameToChange);
        		Writen(client[i], sendbuf, strlen(sendbuf));
		}	
	}
	strcpy(clientProfiles[indexOfSelf].name, nameToChange);
}
//
void who_c(int indexOfSelf,  int *client,  struct clientProfile *clientProfiles, char *sendbuf)
{
	for(int i=0;i<FD_SETSIZE;i++)
	{
		if(client[i] != -1)
		{
			char *address = clientProfiles[i].ip_addr;
			int port = clientProfiles[i].port;
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
		name_c(command[1], indexOfSelf, client,  clientProfiles, sendbuf);
	}
	if(strcmp(command[0], "tell")== 0)
	{
		tell_c(command[1], command[2], indexOfSelf, client,  clientProfiles, sendbuf);
	}
	if(strcmp(command[0], "yell")== 0)
	{
		yell_c(command[1], indexOfSelf, client,  clientProfiles, sendbuf);
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
                *errorMsg = malloc(sizeof(char) * (strlen(err)+1));
                strcpy(*errorMsg, err);
                return 0;
	}
	if(strcmp(command[0], "who") == 0 && numOfParam != 0 )
	{
		char err[] = "[Server] invalid parameter(s). try 'who'\n";
		*errorMsg = malloc(sizeof(char) * (strlen(err)+1));
		strcpy(*errorMsg, err);
		return 0;
	}
	if(strcmp(command[0], "name") == 0 && numOfParam != 1)
	{
		fflush(stdout);
                char err[] = "[Server] invalid parameter(s). try 'name <NEW USERNAME>'\n";
                *errorMsg = malloc(sizeof(char) * (strlen(err)+1));
                strcpy(*errorMsg, err);
		return 0;
        }
	if(strcmp(command[0], "tell") == 0 && numOfParam != 2)
	{
                char err[] = "[Server] invalid parameter(s). try 'tell <USERNAME> <MESSAGE>'\n";
                *errorMsg = malloc(sizeof(char) * (strlen(err)+1));
                strcpy(*errorMsg, err);
		return 0;
        }
	if(strcmp(command[0], "yell") == 0 && numOfParam != 1)
	{
                char err[] = "[Server] invalid parameter(s). try 'yell <MESSAGE>'\n";
                *errorMsg = malloc(sizeof(char) * (strlen(err)+1));
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
	char *rawData = malloc(sizeof(char) * (n+1));
	int indexOfCommand = -1;
	strncpy(rawData, recvbuf, n);
	char delimiter[] = " \n\r";
	token = strtok(rawData, delimiter);
	while(token != NULL && indexOfCommand < MAX_PARAM_NUM)
	{
		command[++indexOfCommand] = malloc(sizeof(char) * (strlen(token)+1));
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
int getRandomPortNum()
{
	srand(time(NULL));
	return rand()%49000 + 1024;
}
//
void randomBind(int sockfd, struct sockaddr_in *my_addr, int addrlen)
{
	my_addr->sin_port = htons(getRandomPortNum());
	while(0 != bind(sockfd, (SA *) my_addr, addrlen))
	{
		my_addr->sin_port = htons(getRandomPortNum());
	}
	printf("Server bind on: %s/%d\n", inet_ntoa(my_addr->sin_addr), (int)ntohs(my_addr->sin_port));
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
	randomBind(listenfd, &servaddr, sizeof(servaddr));
	//servaddr.sin_port = htons(2222);

	//Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

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
