#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <time.h>                /* old system? */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants */
#include        <sys/uio.h>             /* for iovec{} and readv/writev */
#include        <unistd.h>
#include        <sys/wait.h>
#include        <sys/un.h>              /* for Unix domain sockets */
#define MAXLINE         1600
#define SA struct sockaddr
#define max(a,b)        ((a) > (b) ? (a) : (b))


void str_cli(FILE *fp, int sockfd)
{
        int                     maxfdp1, stdineof;
        fd_set          rset;
        char            buf[MAXLINE];
        int             n;

        stdineof = 0;
        FD_ZERO(&rset);
        for ( ; ; ) {
                if (stdineof == 0)
                        FD_SET(fileno(fp), &rset);
                FD_SET(sockfd, &rset);
                maxfdp1 = max(fileno(fp), sockfd) + 1;
                select(maxfdp1, &rset, NULL, NULL, NULL);

                if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
                        if ( (n = read(sockfd, buf, MAXLINE)) == 0) {
                                if (stdineof == 1)
                                        return;         /* normal termination */
                                else{
                                       printf("str_cli: server terminated prematurely\n");
                                       // err_quit("str_cli: server terminated prematurely");
					exit(1);
				}
                        }

                        write(fileno(stdout), buf, n);
                }

                if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			n = read(fileno(fp), buf, MAXLINE);
			char tmp_buf[MAXLINE];
			strncpy(tmp_buf, buf, n);
			char *token = strtok(tmp_buf, " \n\r");
                        if (n  == 0  || strncmp("exit", token, 4) == 0) {
                                stdineof = 1;
                                shutdown(sockfd, SHUT_WR);      /* send FIN */
                                FD_CLR(fileno(fp), &rset);
                                continue;
                        }
                        write(sockfd, buf, n);
                }
        }
}

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 3)
		printf("usage: ./client <SERVER_IP> <SERVER_PORT>\n");
		//err_quit("usage: ./client <SERVER_IP> <SERVER_PORT>");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(strtol(argv[2], NULL, 10));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */
	exit(0);
}
