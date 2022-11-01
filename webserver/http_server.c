#include "basefun.h"
#include <ctype.h>
#include <pthread.h>

#define MAXLINE 1<<10
#define SERV_PORT1 8000
#define SERV_PORT2 443
#define BACKLOGSIZE 20

void http_server() {
    printf("http..............");
    struct sockaddr_in servaddr,cliaddr;
    socklen_t cliaddr_len;
    int listenfd,connfd;
    char buf[MAXLINE],first_line[MAXLINE],left_line[MAXLINE],method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char str[INET_ADDRSTRLEN];
    char filename[MAXLINE];
    long n;
    int on = 1;
    pid_t pid;
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    //初始化myaddr参数
    bzero(&servaddr, sizeof(servaddr)); //结构体清零
    //对servaddr 结构体进行赋值
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT1);
    
    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, BACKLOGSIZE);

    //死循环中进行accept()
    while (1) {
        cliaddr_len = sizeof(cliaddr);
        
        //accept()函数返回一个connfd描述符
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        pid = fork();
        if(pid < 0) {
            printf("fork error");
        } else if(pid == 0) {   //pid=0表示子进程
            while (1) {
                n = Read(connfd, buf, MAXLINE);
                if (n == 0) {
                    printf("the other side has been closed.\n");
                    break;
                }
                printf("%s\n", buf);        
                sscanf(buf, "%s %s %s", method, url, version);
                printf("method = %s\n", method);
                printf("url = %s\n", url);
                printf("version = %s\n", version);
                
                if (strcasecmp(method, "GET") == 0 && strstr(version, "HTTP")) {
                    //strstr()函数用于判断字符串是否包含子串
                    //strcasecmp()函数用于判断字符串是否相等(不区分大小写)
                    //printf("method = %s, url = %s\n", method, url);
                    find_url(url, filename);    //解析url，获取文件名
                    printf("filename = %s\n", filename);
                    http_response(connfd, filename); //响应客户端
                }
            }
            Close(connfd);
            
        }

        else {  //pid>0表示父进程
            Close(connfd);
        }
    }
}

void https_server() {
    printf("https..............");
    struct sockaddr_in servaddr,cliaddr;
    socklen_t cliaddr_len;
    int listenfd,connfd;
    char buf[MAXLINE],first_line[MAXLINE],left_line[MAXLINE],method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char str[INET_ADDRSTRLEN];
    char filename[MAXLINE];
    long n;
    int i,pid;
    int on = 1;
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    //初始化myaddr参数
    bzero(&servaddr, sizeof(servaddr)); //结构体清零
    //对servaddr 结构体进行赋值
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT2);
    
    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, BACKLOGSIZE);

    //死循环中进行accept()
    while (1) {
        cliaddr_len = sizeof(cliaddr);
        
        //accept()函数返回一个connfd描述符
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        //初始化SSL会话
        SSL *ssl = load_SSL(connfd);
        pid = fork();
        if(pid < 0) {
            printf("fork error");
        } else if(pid == 0) {   //pid=0表示子进程
            while (1) {

                //再次Accept()，获取客户端的请求
                if(SSL_accept(ssl) == -1) {
                    ERR_print_errors_fp(stderr);
                }

                n = SSL_Read(ssl, buf, MAXLINE);
                if (n == 0) {
                    printf("the other side has been closed.\n");
                    break;
                }
                printf("%s\n", buf);        
                sscanf(buf, "%s %s %s", method, url, version);
                printf("method = %s\n", method);
                printf("url = %s\n", url);
                printf("version = %s\n", version);
                
                if (strcasecmp(method, "GET") == 0) {
                    //strstr()函数用于判断字符串是否包含子串
                    //strcasecmp()函数用于判断字符串是否相等(不区分大小写)
                    //printf("method = %s, url = %s\n", method, url);
                    find_url(url, filename);    //解析url，获取文件名
                    printf("filename = %s\n", filename);
                    https_response(ssl, connfd, filename); //响应客户端
                }
            }
            close(connfd);
            SSL_free(ssl);
        }
 
        else {  //pid>0表示父进程
            close(connfd);
            SSL_free(ssl);
        }
    }
    
}


int main(int argc, char * argv[]) {
    pid_t pid1, pid2;
    pid1 = fork();
    if(pid1 < 0) printf("fork1 error\n");
    if(pid1 == 0)   http_server();
    
    pid2 = fork();
    if(pid2 < 0) printf("fork2 error\n");
    if(pid2 == 0)   https_server();

    int st1, st2;
    waitpid(pid1, &st1, 0);
    waitpid(pid2, &st2, 0);
    return 0;
}
