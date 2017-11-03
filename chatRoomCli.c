/* Use standard echo server; baseline measurements for nonblocking version */
#include	"unp.h"


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
                Select(maxfdp1, &rset, NULL, NULL, NULL);

                if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
                        if ( (n = Read(sockfd, buf, MAXLINE)) == 0) {
                                if (stdineof == 1)
                                        return;         /* normal termination */
                                else
                                        err_quit("str_cli: server terminated prematurely");
                        }

                        Write(fileno(stdout), buf, n);
                }

                if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			n = Read(fileno(fp), buf, MAXLINE);
                        if (n  == 0 || strncmp("exit", buf, n-1)==0) {
                                stdineof = 1;
                                Shutdown(sockfd, SHUT_WR);      /* send FIN */
                                FD_CLR(fileno(fp), &rset);
                                continue;
                        }

                        Writen(sockfd, buf, n);
                }
        }
}

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 3)
		err_quit("usage: ./client <SERVER_IP> <SERVER_PORT>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(strtol(argv[2], NULL, 10));
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */

	exit(0);
}
