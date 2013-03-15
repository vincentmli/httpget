#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

/*gcc -o httpgetackpsh htmlgetackpsh.c 
  simiple http client get to push data in 3WHS final ack
*/

char *get_ip(char *host);
int get_page(char *host, char *page);
void usage();

#define HOST "ickernel.blogspot.com"
#define PAGE "/"
#define PORT 80
#define USERAGENT "HTTPGET 1.0"

int main(int argc, char **argv)
{
	struct sockaddr_in *remote;
	int sock;
	int tmpres;
	int val;
	int i;
	char *ip;

	char buf[BUFSIZ+1];
	char *host;
	char *page;

	if(argc == 1){
		usage();
		exit(2);
	}  
	host = argv[1];
	if(argc > 2){
		page = argv[2];
	}else{
		page = PAGE;
	}

	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("Can't create TCP socket");
	return 0;
}

/*  
  val = Fcntl(sock, F_GETFL, 0);
  Fcntl(sock, F_SETFL, val | O_NONBLOCK);
*/
  
	ip = get_ip(host);
	fprintf(stderr, "IP is %s\n", ip);
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
	if( tmpres < 0) {
		perror("Can't set remote->sin_addr.s_addr");
		return 0;
	}else if(tmpres == 0) {
		fprintf(stderr, "%s is not a valid IP address\n", ip);
		return 0;
 	}
	remote->sin_port = htons(PORT);

	if (setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (void *)&i, sizeof(i)) < 0) {
// if (setsockopt(sock, IPPROTO_TCP, TCP_DEFER_ACCEPT, (void *)&i, sizeof(i)) < 0) {
		perror("setsockopt error");
 	}


	if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
		perror("Could not connect");
		return 0;
 	}
 

	char *query = "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n";

	fprintf(stderr, "Query is:\n<<START>>\n%s<<END>>\n", query);

	tmpres = send(sock, query, strlen(query), 0);
//  tmpres = sendto(sock, query, strlen(query), 0, (struct sockaddr *)remote, sizeof(struct sockaddr));
	if (tmpres <0 ) {
		perror("Could not send");
 	}
  
 //now it is time to receive the page
	memset(buf, 0, sizeof(buf));
	int httpstart = 0;
	char * httpcontent;
	while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0){
	if(httpstart == 0) {
      /* Under certain conditions this will not work.
      * If the \r\n\r\n part is splitted into two messages
      * it will fail to detect the beginning of HTML content
      */
		httpcontent = strstr(buf, "\r\n\r\n");
		if(httpcontent != NULL){
			httpstart = 1;
			httpcontent += 4;
      		}
	}else{
		httpcontent = buf;
    	}
	if(httpstart){
     /* fprintf(stdout, httpcontent); */
		fprintf(stdout, "%s", httpcontent);
    	}
 
	memset(buf, 0, tmpres);
	}
	if(tmpres < 0) {
		perror("Error receiving data");
	}
	free(remote);
	free(ip);
	close(sock);
	return 0;
}

void usage() {
	fprintf(stderr, "USAGE: httpget host [page]\n\
	\thost: the website hostname. ex: www.example.com\n\
	\tpage: the page to retrieve. ex: index.http, default: /\n");
}



char *get_ip(char *host) {
	struct hostent *hent;
	int iplen = 15; //XXX.XXX.XXX.XXX
	char *ip = (char *)malloc(iplen+1);
	memset(ip, 0, iplen+1);
	if((hent = gethostbyname(host)) == NULL) {
		herror("Can't get IP");
		exit(1);
	}
	if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL) {
		perror("Can't resolve host");
		exit(1);
	}
	return ip;
}

int Fcntl(int fd, int cmd, int arg)
{
	int n;

        if ( (n = fcntl(fd, cmd, arg)) == -1)
		perror("fcntl");
        return(n);
}

