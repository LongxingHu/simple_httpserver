#include "basefun.h"


#define MAXLINE 1<<20

/*
带有容错机制的所有socke方法的封装
*/

int Socket(int family, int type, int protocol) {
    int sockfd;
    if((sockfd = socket(family, type, protocol)) < 0)
    {
        perror("socket error");
        exit(1);
    }
    return sockfd;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if(bind(sockfd, addr, addrlen) < 0)
    {
        perror("bind error");
        exit(1);
    }
}

void Listen(int sockfd, int backlog) {
    if(listen(sockfd, backlog) < 0)
    {
        perror("listen error");
        exit(1);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd;
    if((connfd = accept(sockfd, addr, addrlen)) < 0)
    {
        perror("accept error");
        exit(1);
    }
    return connfd;
}

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    //为请求http的客户端准备
    if(connect(sockfd, addr, addrlen) < 0)
    {
        perror("connect error");
        exit(1);
    }
}

long Read(int fd, void *buf, size_t count) {
    long n;
    if((n = read(fd, buf, count)) < 0)
    {
        perror("read error");
        exit(1);
    }
    return n;
}

void Write(int fd, void *buf, size_t count) {
    if(write(fd, buf, count) < 0)
    {
        perror("write error");
        exit(1);
    }
}


long SSL_Read(SSL *ssl, void *buf, size_t count) {
    long n;
    if((n = SSL_read(ssl, buf, count)) < 0)
    {
        perror("read error");
        exit(1);
    }
    return n;
}

void SSL_Write(SSL *ssl, void *buf, size_t count) {
    if(SSL_write(ssl, buf, count) < 0)
    {
        perror("write error");
        exit(1);
    }
}

void Close(int fd) {
    if(close(fd) < 0)
    {
        perror("close error");
        exit(1);
    }
}

SSL * load_SSL(int fd) {

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    printf("加载证书...\n");
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method()); //创建服务端SSL会话环境

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if(SSL_CTX_use_certificate_file(ctx, "cnlab.cert", SSL_FILETYPE_PEM) <= 0) {
        //加载公钥证书
        printf("load public key error");
        exit(1);
    }
    printf("加载私钥...\n");
    if(SSL_CTX_use_PrivateKey_file(ctx, "cnlab.prikey", SSL_FILETYPE_PEM) <= 0) {
        //加载私钥
        printf("load private key error");
        exit(1);
    }
    printf("验证私钥...\n");
    if(SSL_CTX_check_private_key(ctx) <= 0) {
        //检查私钥
        printf("check private key error");
        exit(1);
    }


    SSL *ssl = SSL_new(ctx);

    if(ssl == NULL) {
        printf("SSL_new error");
        exit(1);
    }


    if(SSL_set_fd(ssl, fd) == 0) {
        printf("SSL_set_fd error");
        exit(1);
    }
    return ssl;
}


void find_url(char *url, char *filename) {
    char *ptr;
    if(!strstr(url, "cgi-bin")){
        //说明是静态网页
        sscanf(url, "/%s", filename);
    }
}

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html") || strstr(filename, ".php"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if(strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void http_response(int connfd, char *filename) {
    struct stat sbuf;   //文件状态结构体
    int fd;
    char *srcp;
    char response[MAXLINE], filetype[20];
    printf("开始向客户端发送响应...\n");
    if (stat(filename, &sbuf) < 0) {
        //文件不存在
        sprintf(response, "HTTP/1.1 404 Not Found\r");
        printf("找不到文件\n");
        exit(1);
    }

    else {
        get_filetype(filename, filetype);   //获取文件类型
        
        //Open File
        fd = open(filename, O_RDONLY);
        //Send response 这是是在进行拼接
        strcat(response, "HTTP/1.0 200 OK\r\n");
        Write(connfd, response, strlen(response));
        
        strcat(response, "Server: LongXing's Tiny Web Server\r\n");
        Write(connfd, response, strlen(response));
        sprintf(response, "Content-length: %ld\r\n", sbuf.st_size);
        Write(connfd, response, strlen(response));
        sprintf(response, "Content-type: %s\r\n\r\n", filetype);
        Write(connfd, response, strlen(response));
        
        printf("开始发送文件...\n");
        //mmap()读取filename 的内容写给浏览器 
        srcp = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        
        Write(connfd, srcp, sbuf.st_size);
        munmap(srcp, sbuf.st_size);
        Close(fd);
    }
}


void https_response(SSL *ssl, int connfd, char *filename) {
    struct stat sbuf;   //文件状态结构体
    int fd;
    char *srcp;
    char response[MAXLINE], filetype[20];

    if (stat(filename, &sbuf) < 0) {
        //文件不存在
        sprintf(response, "HTTP/1.1 404 Not Found\r");
        printf("找不到文件\n");
        exit(1);
    }

    else {
        get_filetype(filename, filetype);   //获取文件类型
        
        //Open File
        fd = open(filename, O_RDONLY);
        //Send response 这是是在进行拼接
        strcat(response, "HTTP/1.0 200 OK\r\n");
        SSL_Write(ssl, response, strlen(response));
        
        strcat(response, "Server: LongXing's Tiny Web Server\r\n");
        SSL_Write(ssl, response, strlen(response));
        sprintf(response, "Content-length: %ld\r\n", sbuf.st_size);
        SSL_Write(ssl, response, strlen(response));
        sprintf(response, "Content-type: %s\r\n\r\n", filetype);
        SSL_Write(ssl, response, strlen(response));
        printf("Response headers:\n");
        printf("%s", response);
        //mmap()读取filename 的内容写给浏览器 
        srcp = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        SSL_Write(ssl, srcp, sbuf.st_size);
        munmap(srcp, sbuf.st_size);
        Close(fd);
    }
}
