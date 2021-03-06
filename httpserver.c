#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <syslog.h>
#include <stdarg.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "lib/httputil.h"
#include "lib/tcputil.h"
#include "lib/logutil.h"
#include "lib/linkedlist.h"
#include "lib/util.h"




#define MAXCONNECTION 50
#define PROCESS_NAME_DEFAULT "http-server"

void start_server(const char* process_name, const char * port, const char * root_path, const int debugflg);
void *serve(void *arg);
void termination_handler (int signum);
extern int http_server_tosyslog;
extern int suppress_debug;
extern int show_thread_id;
extern struct node * fileLinkedList;
extern char *dns_server;
char * root_path;

int main(int argc, char *argv[])
{
    int c, portflg=0, debugflg=0;
    extern char *optarg;
    extern int optind, optopt;
    char *process_name=PROCESS_NAME_DEFAULT, *port;

    char * usage_msg = 
"\nUsage: ./httpserver [-n] <-p> [-r] [-d] [-s] [-h]\n\
    -n    the process name. Argument is mandatory.\n\
    -p    the port it listens on. Argument is mandatory.\n\
    -d    debug model if this is specified.\n\
    -r    document root\n\
    -s    DNS server host name or ip address\n\
    -h    show the usage\n\
Example1: ./server -p 3000\n\
Example2: ./server -p 3000 -r /home/NAME/my_server_root/\n\
Example3: ./server -p 3000 -r /home/NAME/my_server_root/ -s 8.8.8.8\n\n";;

    http_server_tosyslog=1;
    suppress_debug=0;
    show_thread_id=1;
    dns_server=NULL;

    while ((c = getopt(argc, argv, ":hdn:p:r:s:")) != -1) {
        
        switch(c) {
        case 'n':
            process_name = optarg;
            printf("process name: %s.\n", process_name);
            break;
        case 'p':
            portflg++;
            port = optarg;
            printf("port: %s.\n", port);
            break;
        case 'r':
            root_path = optarg;
            printf("document root: %s.\n", root_path);
            break;
        case 'd':
            http_server_tosyslog=0;
            suppress_debug=0;
            debugflg++;
            printf("debug mode is enabled.\n");
            break;
        case 's':
            dns_server=optarg;
            printf("dns server is %s.\n", dns_server);
            break;
        case 'h':
            printf("%s",usage_msg);
	    exit(0);
        case ':':
            if(optopt=='r'){
                printf("%s",usage_msg);
	        exit(0);
            }else if(optopt=='n'){
                process_name="http-server-zhiqi";
            }else{
            printf("-%c argument is missing\n", optopt);
            }
            break;
        case '?':
            printf("Unknown arg %c\n", optopt);
            exit(0);
        }
        
    }

    if(portflg<=0)
    {
        printf("%s",usage_msg);
	exit(0);
    }else{
        start_server(process_name,port,root_path,debugflg);
    }

    return 0;
}




void start_server(const char* process_name, const char * port, const char * root_path, const int debugflg){
    struct sockaddr    cliaddr;
    int    i, listenfd;
    pthread_t tid;
    int connfd[MAXCONNECTION];
    socklen_t    clilen, addrlen;
    log_debug("pid-%d started...\n",(int)getpid());
    if(debugflg<1){
        daemon_init(process_name,LOG_WARNING);
        sleep(3);
    }
    Signal(SIGTERM, termination_handler);
    Signal(SIGKILL, termination_handler);
    Signal(SIGINT, termination_handler);
    Signal(SIGQUIT, termination_handler);
    log_debug("HttpServer-%d started...\n",(int)getpid());
    listenfd = tcp_listen(NULL, port, &addrlen);

    for( i=0; i < MAXCONNECTION; i++)
    {
        connfd[i] = -1;
    }
    for ( ; ; ) {
        int j=0,ret=0;
	
        clilen = addrlen;
        for(j=0; j < MAXCONNECTION; j++)
        {
            if(connfd[j] == -1)
            {
                break;
            }	
        }
		
        connfd[j] = accept(listenfd, &cliaddr, &clilen);
        log_debug("Accepted a new client from %s, sockfd=%d\n",sock_ntop(&cliaddr,clilen),connfd[j]);
        ret=pthread_create(&tid, NULL, &serve, (void *) &(connfd[j]));
	log_debug("new thread-%d is created. pthread_create() return %d\n",(int)tid, ret);
    }
}

void serve_process(int connfd)
{
    serve_http_request(connfd,root_path);
}

void *serve(void *arg)
{
	/*	void	web_child(int);*/
	pthread_detach(pthread_self());
	serve_process(*((int *) arg));
	*((int*) arg) = -1;
	return(NULL);
}


void
termination_handler (int signum)
{
  log_debug("caught signal-%d\n",signum);
  clearFileList(fileLinkedList);
  exit(0);
}


