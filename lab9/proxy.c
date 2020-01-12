#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400



/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:56.0) Gecko/20100101 Firefox/56.0\r\n";

void doit(int fd);
int powerten(int i);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parseuri(char *uri, char *host, char *path);
void request_header(char *buf, char *host, char *request);

/* individual thread function*/
void *handle_conn(void *pfd) {
	int *pifd = pfd;
	int cfd = *pifd;
	free(pfd);

	pthread_detach(pthread_self());

	doit(cfd);

	close(cfd);
	return NULL;
}

int main(int argc, char **argv) 
{
	int listenfd, connfd;
	char host[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

    /* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}


	/* Create a listening descriptor */
	listenfd = Open_listenfd(argv[1]);

	/* Accept incoming request */
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
		int *temp_connfd = malloc(sizeof(temp_connfd));
		*temp_connfd = connfd;

		Getnameinfo((SA *) &clientaddr, clientlen, host, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", host, port);
		pthread_create(&tid, NULL, handle_conn, temp_connfd);                                        
	}
	printf("%s", user_agent_hdr);
	return 0;
}



/*
 * doit - parse the uri, send a request to the server, recieve data from the server
 */
void doit(int fd)
{
	int *port;
	port = malloc(sizeof(int));
	*port = 80;

	/* Variables for parsing the incoming client request header*/
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

	/* Variables for parsing the uri*/
	char host[MAXLINE], path[MAXLINE];

	rio_t rio_client;
	rio_t rio_server;

	char server_response[MAXLINE];

	char request[MAXLINE];

	int serverfd;

	char cache_buf[MAX_OBJECT_SIZE];
	
	/* Initialize all the buffers with 0 */
	memset(buf, 0, sizeof(buf));
	memset(method, 0, sizeof(method));
	memset(uri, 0, sizeof(uri));
	memset(version, 0, sizeof(version));
	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));
	memset(request, 0, sizeof(request));
	memset(server_response, 0, sizeof(server_response));
	memset(cache_buf, 0, sizeof(cache_buf));	
	
	/* Read the request from the client and parse it in buf */
	Rio_readinitb(&rio_client, fd);
	if (!Rio_readlineb(&rio_client, buf, MAXLINE)) 
		return;
	printf("Client Request: %s\n", buf);

    /* Parse the request into its method, uri, and version */
	sscanf(buf, "%s %s %s", method, uri, version);

   	 /* If the method isn't GET, appologize for the inconinience :)*/
	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented",
			"Proxy does not implement this method");
		return;
	} 

	read_requesthdrs(&rio_client);

	/* Parse the url into a host and path */
	*port = parseuri(uri, host, path);

	sprintf(request, "%s %s %s\r\n", method, path, version);


	request_header(buf, host, request);
	char port2[10];
	sprintf(port2, "%d", *port);
	serverfd = Open_clientfd(host, port2);
	rio_readinitb(&rio_server, serverfd);

	/* Send the request to the server */
	rio_writen(serverfd, request, strlen(request));

	/*Recieve the response from the server */
	int linesize;
	linesize = rio_readnb(&rio_server, server_response, sizeof(server_response));

	rio_writen(fd, server_response, linesize);

	int count = 0;
	while((linesize = rio_readnb(&rio_server, server_response, sizeof(server_response)))>0){
		rio_writen(fd, server_response, linesize);strcat(cache_buf, server_response);
		count++;
		memset(server_response, 0, sizeof(server_response));
	}
	close(serverfd);
}

void request_header(char *buf, char *host, char *request){
	strcat(request, "Host:");
	strcat(request, host);
	strcat(request, "\r\n");
	strcat(request, user_agent_hdr);
	strcat(request, "\r\n");
	strcat(request, "Proxy-Connection: close\r\n");
	strcat(request, "Connection: close\r\n");
}

int parseuri(char *uri, char *host, char *path){
	int port;
	char buf[MAXLINE];

	sscanf(uri, "%*[^:]://%[^/]%s", host, path);
	if(strstr(host, ":")){
		strcpy(buf, host);
		sscanf(buf, "%[^:]:%d", host, &port);
		return port;
	}
	return 80;
}


/*
 * read_requesthdrs - read the client request headers
 */
void read_requesthdrs(rio_t *rp) 
{
	char buf[MAXLINE];
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
	while(strcmp(buf, "\r\n")) { 
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf); 
	}
	return;
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
	char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}

int powerten(int i){
	int temp = 1;
	while(i!=0){
		temp = temp*10;
		i--;
	}
	return temp;
} 