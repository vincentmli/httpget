#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

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
void fork_child(struct host_page *hp);

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
        _exit(0);
   }

/* fork version below */
  int i;
  for(i = 0; i < 10; i++) {
 
    fprintf(stderr, "i:\n%d\n", i);
    fork_child(hpptr);
  }

/*  exit(0); */

  
}

void usage()
{
  fprintf(stderr, "USAGE: httpget host [page]\n\
\thost: the website hostname. ex: www.example.com\n\
\tpage: the page to retrieve. ex: index.http, default: /\n");
}


void fork_child(struct host_page *hpptr) {
     /* Output from both the child and the parent process
    * will be written to the standard output,
    * as they both run at the same time.
    */
   struct host_page *hpp = hpptr; 
   pid_t pid;

   pid = fork();

   if (pid == 0)
   {
      /* Child process:
       * When fork() returns 0, we are in
       * the child process.
       * Here we count up to ten, one each second.
      */
      if(get_page_thread(hpp)){
        perror("Can't get page");
        _exit(0);
      }
   }
   else if (pid > 0)
   {
      /* Parent process:
       * When fork() returns a positive number, we are in the parent process
       * (the fork return value is the PID of the newly-created child process).
       * Again we count up to ten.
       */
      fprintf(stderr, "fork, succeed, pid %d\n", pid);
      /* exit(0); */
   }
   else
   {
      /* Error:
       * When fork() returns a negative number, an error happened
       * (for example, number of processes reached the limit).
       */
      fprintf(stderr, "can't fork, error %d\n", errno);
      /* exit(EXIT_FAILURE); */
   }

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


  if (!(sock=tcp_connect(host))) {
    perror("Could not connect");
    exit(1);
  }

  get = build_get_query(host, page);
  fprintf(stderr, "Query is:\n<<START>>\n%s<<END>>\n", get);

  //Send the query to the server
  int sent = 0;
  while(sent < strlen(get))
  {
    tmpres = send(sock, get+sent, strlen(get)-sent, 0);
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


int tcp_connect(char *host)
{
  struct sockaddr_in *remote;
  int sock;
  int tmpres;
  char *ip;

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

  if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
    perror("Could not connect");
    return 0;
  }
  return sock;
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

