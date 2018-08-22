//
// Created by wangh on 18-4-14.
//

#ifndef PRESSURE_TEST_PRESSURETEST_H
#define PRESSURE_TEST_PRESSURETEST_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include "threadpool.h"
//#define SERV_IP "172.18.72.31"
#define URL "www.baidu.com"
#define LAST_TIME 60
#define _TEST
//#define thread_pool
#define _DEBUG

extern struct epoll_event ev;//epoll
extern struct sockaddr_in srv_addr;//地址
extern pthread_mutex_t mutex;
extern struct timeval start_time;
extern struct timeval end_time;
extern struct connection cn;

//记录转成数字的传入参数
struct parameter
{
    int connect_num;     //连接数
    int thread_num;      //线程数
    int concurrent_num;  //并发数
};

//记录传递给线程的参数
struct Threads_parameters
{
    int concurrency;  //并发数
    int connections;  //连接数
};

//发送Http请求信息
struct connection{
    int fd; //发送请求的fd
    int epfd;
    struct epoll_event event;

    char url[2048];
    char host[512];
    char request[2048];
};

/************************************************************************************
 * @description         处理输入参数。处理结果保存在struct parameter
 * @param argc
 * @param argv
 * @return              返回保存处理后的输入参数struct parameter
 */
struct parameter parameter_init(int argc,char *argv[]);


/************************************************************************************
 * @description         设置socket套接字为非阻塞,这样在connect的时候直接返回
 * @param sockfd        socket套接字
 */
void setnonblocking(int sockfd);


/************************************************************************************
 * @description         初始化epoll
 * @return              返回epoll的文件描述符
 */
int init_epoll();


/************************************************************************************
 * @description         像epoll中添加socket套接字
 * @param epfd          epoll文件描述符
 * @param fd            socket套接字
 */
void add_epoll(int epfd,int fd);


/************************************************************************************
 *　@description         开始定时
 */
void time_start();


/************************************************************************************
 * @description         创建一个socket，并connect，并讲socket添加到epoll监听
 * @param epfd          epoll文件描述符
 * @return              是否创建成功
 */
int start_connect(int epfd);


/************************************************************************************
 * @description         等待epoll事件响应
 * @param epfd          epoll文件描述符
 * @param tp            struct Threads_parameters
 */
void wait_epoll(int epfd,struct Threads_parameters tp);


/************************************************************************************
 * @description         线程回调函数
 * @param args          传入参数为（void*）struct Threads_parameters＊
 * @return
 */
void* start_thread(void* args);


/*************************************************************************************
 * @description         初始化线程需要的参数struct Threads_parameters
 * @param connect_num   连接数
 * @param thread_num    线程数
 * @param concurrent_num　并发数
 * @param tp             [out] struct Threads_parameters
 */
void thread_init(int connect_num,int thread_num,int concurrent_num,struct Threads_parameters tp[]);


/*************************************************************************************
 * @description         打印最终结果
 * @param connect_num   传入总连接数，用以计算每秒连接数
 */
void print_reult(int connect_num);


/*************************************************************************************
 *　@description         等待所有线程全部结束
 * @param tid            所有线程id
 * @param thread_num     线程总数
 */
void wait_thread(pthread_t tid[],int thread_num);


/*************************************************************************************
 * @description          在正式创建socket前的准备工作，用来获取地址
 * @return
 */
int socket_init();


/*************************************************************************************
 * @description          如果程序长时间不结束，则触发该函数
 * @param sig
 */
void time_exit(int sig);

/*************************************************************************************
 * @description          输入参数错误，打印结果
 */
void print_error();

/*************************************************************************************
 * @description          发送http请求
 * */
void write_request(int fd);

/*************************************************************************************
 * @description          解析URL,得出http请求的url和host
 * @param url            要解析的URL
 * */
int parse_url(char *url);


void close_connection(struct connection c);

#endif //PRESSURE_TEST_PRESSURETEST_H
