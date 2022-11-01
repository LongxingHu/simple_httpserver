#ifndef basefun_h
#define basefun_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//read方法需要的头文件
#include <unistd.h>
//socket方法需要的头文件
#include <sys/socket.h>
#include <sys/types.h>
//htonl 方法需要的头文件
#include <netinet/in.h>
//inet_ntop方法需要的头文件
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int Socket(int family, int type, int protocol);
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);
void Listen(int fd, int backlog);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
void Connect(int fd, const struct sockaddr *sa, socklen_t salen);
void Close(int fd);
void Write(int fd, void *ptr, size_t nbytes);
long Read(int fd, void *ptr, size_t nbytes);
long SSL_Read(SSL *ssl, void *buf, size_t count);
void SSL_Write(SSL *ssl, void *buf, size_t count);
void find_url(char *url, char *filename);
void get_filetype(char *filename, char *filetype);
void http_response(int connfd, char *filename);
void https_response(SSL *ssl, int connfd, char *filename);
SSL *load_SSL(int fd);
#endif