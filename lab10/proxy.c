/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

#define NTHREADS 4
#define SBUFSIZE 512

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void doit(int fd, struct sockaddr_in* addr);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
int Rio_writen_w(int fd, void *usrbuf, size_t n);
void* thread(void* vargp);
int open_clientfd_ts(char *hostname, int port);

sem_t dnsSem;
FILE *logFile;

typedef struct {
    int fd;
    struct sockaddr_in addr;
} ClientInfo;

typedef struct {
    ClientInfo** buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} Sbuf_t;

Sbuf_t sbuf;

void sbuf_init(Sbuf_t* sp, int n) {
    sp->buf = Calloc(n, sizeof(Sbuf_t));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(Sbuf_t* sp) {
    Free(sp->buf);
}

void sbuf_insert(Sbuf_t* sp, ClientInfo* item) {
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

ClientInfo* sbuf_remove(Sbuf_t* sp) {
    ClientInfo* item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    Signal(SIGPIPE, SIG_IGN);
    int listenfd, connfd, port, i;
    socklen_t clientlen = sizeof(ClientInfo);
    struct sockaddr_in clientaddr;
    pthread_t tid;
    logFile = fopen("proxy.log", "a");

    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    listenfd = Open_listenfd(port);

    Sem_init(&dnsSem, 0, 1);

    for (i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        ClientInfo* tmp = (ClientInfo*)Malloc(sizeof(ClientInfo));
        tmp->fd = connfd;
        tmp->addr = clientaddr;
        sbuf_insert(&sbuf, tmp);
    }

    exit(0);
}


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }


    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    
    hostname[len] = '\0';

    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
        *port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;	
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
        char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
}


void doit(int clientfd, struct sockaddr_in* addr) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char serverHost[MAXLINE], serverPath[MAXLINE];
    int serverPort;
    rio_t clientRio, serverRio;

    Rio_readinitb(&clientRio, clientfd);
    Rio_readlineb_w(&clientRio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }

    if (parse_uri(uri, serverHost, serverPath, &serverPort) < 0) {
        printf("Error!\n");
        return;
    }

    int serverfd = open_clientfd_ts(serverHost, serverPort);

    if (Rio_writen_w(serverfd, buf, strlen(buf)) < 0) {
            printf("-3\n");
            Close(serverfd);
            return;
    }

    while(strlen(buf) > 2) {
        Rio_readlineb_w(&clientRio, buf, MAXLINE);
        if (Rio_writen_w(serverfd, buf, strlen(buf)) < 0) {
            printf("-2\n");
            Close(serverfd);
            return;
        }
    }

    Rio_readinitb(&serverRio, serverfd);

    int n, size = 0;

    while ((n = Rio_readnb_w(&serverRio, buf, MAXLINE)) > 0) {  
        if (rio_writen(clientfd, buf, n) != n) {
            printf("-1\n");
            break;
        }
        size += n;
    }

    format_log_entry(buf, addr, uri, size);
    fprintf(logFile, "%s", buf);

    Close(serverfd);  
}

void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    if (Rio_writen_w(fd, buf, strlen(buf)) < 0) {
        printf("-4\n");
        return;
    }
    sprintf(buf, "Content-type: text/html\r\n");
    if (Rio_writen_w(fd, buf, strlen(buf)) < 0) {
        printf("-4\n");
        return;
    }
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    if (Rio_writen_w(fd, buf, strlen(buf)) < 0) {
        printf("-4\n");
        return;
    }
    if (Rio_writen_w(fd, body, strlen(body)) < 0) {
        printf("-4\n");
        return;
    }
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        printf("Rio_readnb error\n");
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        printf("Rio_readlineb error\n");
        return 0;
    }
    return rc;
} 

int Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n) {
        printf("Rio_writen error\n");
        return -1;
    }
    return 0;
}

void* thread(void* vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        ClientInfo* tmp = sbuf_remove(&sbuf);
        int fd = tmp->fd;
        struct sockaddr_in addr = tmp->addr;
        doit(fd, &addr);
        free(tmp);
        Close(fd);
    }
}

int open_clientfd_ts(char *hostname, int port) {    
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */

    P(&dnsSem);

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL) {
        V(&dnsSem);
        return -2; /* check h_errno for cause of error */
    }
    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    
    V(&dnsSem);

    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;
    return clientfd;
}     