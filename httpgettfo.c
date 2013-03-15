/*
linux kernel implemented TCP Fast Open to accept SYN with data
this is  a simple simple http test code to have GET request in Syn packet
http://marc.info/?l=linux-netdev&m=135248434825909&w=2
*/
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#define MAX_HOST_PAGE 2

struct host_page {
    char *host;
    char *page;
} hp[MAX_HOST_PAGE];

int tcp_connect(char *host);
char *get_ip(char *host);
char *build_get_query(char *host, char *page);
int *get_page_thread(struct host_page *hp);
void usage();

#define HOST "www.example.com"
#define PAGE "/"
#define PORT 80
#define USERAGENT "HTTPGET 1.0"

int main(int argc, char **argv)
{
 
  struct host_page *hpptr; 
 
  if(argc == 1){
    usage();
    exit(2);
  }  
  hp[0].host = argv[1];
  fprintf(stderr, "hp->host:\n%s\n", hp[0].host);
  if(argc > 2){
    hp[0].page = argv[2];
    fprintf(stderr, "host->page:\n%s\n", hp[0].page);
  }else{
    hp[0].page = PAGE;
  }

 hpptr = &hp[0];

    if(get_page_thread(hpptr)){
      perror("Can't get page");
      exit(2);
    }
  
}

void usage()
{
  fprintf(stderr, "USAGE: httpgettfo host [page]\n\
\thost: the website hostname. ex: www.example.com\n\
\tpage: the page to retrieve. ex: index.http, default: /\n");
}

int *get_page_thread(struct host_page *hp){


  struct sockaddr_in *remote;
  int sock;
  int tmpres;
  char *ip;
  char *host;
  char *page;

  host=hp->host;
  page=hp->page;

  char *get;
  char buf[BUFSIZ+1];


  if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    perror("Can't create TCP socket");
    return 0;
  }

  ip = get_ip(host);
  fprintf(stderr, "IP is %s\n", ip);
  remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
  remote->sin_family = AF_INET;
  tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
  if( tmpres < 0)
  {
    perror("Can't set remote->sin_addr.s_addr");
    return 0;
  }else if(tmpres == 0)
  {
    fprintf(stderr, "%s is not a valid IP address\n", ip);
    return 0;
  }
  remote->sin_port = htons(PORT);

  get = build_get_query(host, page);
  fprintf(stderr, "Query is:\n<<START>>\n%s<<END>>\n", get);

//tfo send_len = sendto(sd, msg, 15, MSG_TFO, rver_addr, addr_len);

  int sent = 0;
  while(sent < strlen(get))
  {
    tmpres = sendto(sock, get+sent, strlen(get)-sent, MSG_TFO, (struct sockaddr *)remote, sizeof(struct sockaddr));
    if(tmpres == -1){
      perror("Can't send query");
      exit(1);
    }
    sent += tmpres;
  }
  //now it is time to receive the page
  memset(buf, 0, sizeof(buf));
  int httpstart = 0;
  char * httpcontent;
  while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0){
    if(httpstart == 0)
    {
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
  if(tmpres < 0)
  {
    perror("Error receiving data");
  }
  free(get);
  free(remote);
  free(ip);
  close(sock);
  return 0;

}



char *get_ip(char *host)
{
  struct hostent *hent;
  int iplen = 15; //XXX.XXX.XXX.XXX
  char *ip = (char *)malloc(iplen+1);
  memset(ip, 0, iplen+1);
  if((hent = gethostbyname(host)) == NULL)
  {
    herror("Can't get IP");
    exit(1);
  }
  if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
  {
    perror("Can't resolve host");
    exit(1);
  }
  return ip;
}

char *build_get_query(char *host, char *page)
{
  char *query;
  char *getpage = page;
  char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
/*char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n"; */
  if(getpage[0] == '/'){
    getpage = getpage + 1;
    fprintf(stderr,"Removing leading \"/\", converting %s to %s\n", page, getpage);
  }
  // -5 is to consider the %s %s %s in tpl and the ending \0
  query = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)-5);
  sprintf(query, tpl, getpage, host, USERAGENT);
  return query;
}

