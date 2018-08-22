//
// Created by wangh on 18-4-14.
//

#include "pressureTest.h"

struct epoll_event ev;//epoll
struct sockaddr_in srv_addr;//地址
struct connection cn;
pthread_mutex_t mutex;


struct timeval start_time;
struct timeval end_time;

struct parameter parameter_init(int argc,char *argv[])
{
    struct parameter temp_p;
    temp_p.concurrent_num = 1;
    temp_p.connect_num = 1;
    temp_p.thread_num = 1;

    int temp_argv = 0;

    for (int j = 1; j < argc-1; j++)
    {
        if(1 == j%2)
        {
            if(0 == strcmp("-n",argv[j])){//总连接数
                temp_argv = 1;
            }
            else if (0 == strcmp("-c",argv[j]))
            {//并发数
                temp_argv = 2;
            }
            else if (0 == strcmp("-d",argv[j]))
            {//线程数
                temp_argv = 3;
            }
            else
            {
                print_error();
            }
        }
        else
        {
            if (1 == temp_argv){
                temp_p.connect_num = atoi(argv[j]);
                if(0 == temp_p.connect_num)
                    print_error();
            }
            else if (2 == temp_argv)
            {
                temp_p.concurrent_num = atoi(argv[j]);
                if(0 == temp_p.concurrent_num)
                    print_error();
            }
            else if (3 == temp_argv)
            {
                temp_p.thread_num = atoi(argv[j]);
                if(0 == temp_p.thread_num)
                    print_error();
            }

#ifdef _DEBUG
            printf("%d\n",atoi(argv[j]));
#endif
        }
    }
    parse_url(argv[argc-1]);
    strcat(cn.request,"GET ");
    strcat(cn.request,URL);
    return temp_p;
}

void time_start()
{
    gettimeofday(&start_time,NULL);
}

void time_end()
{
    gettimeofday(&end_time,NULL);
}

void setnonblocking(int sockfd)
{
    int opts;

    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0)
    {
        perror("fcntl(F_GETFL)\n");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, opts) < 0)
    {
        perror("fcntl(F_SETFL)\n");
        exit(1);
    }
}

int init_epoll()
{
    return epoll_create(256);
}

void add_epoll(int epfd,int fd)
{
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLOUT | EPOLLERR | EPOLLET;

    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
}

int start_connect(int epfd)
{
    int sockd,rtn;

    sockd = socket(AF_INET,SOCK_STREAM,0); //创建一个socket
    if(sockd < 0){
        perror("socket error: ");
        return -1;
    }
    setnonblocking(sockd);//设置为非阻塞


    rtn = connect(sockd,(struct sockaddr *)(&srv_addr),sizeof(srv_addr));

  //  write_request(sockd);
    if(rtn < 0)
    {
        if (errno == EINPROGRESS)
        {//表明连接还在进行
            add_epoll(epfd,sockd);
        }
        else
        {//如果连接失败
            close(sockd);
            start_connect(epfd);
            perror("connect error: ");
        }
    }
    else if(0 == rtn)
    {
        printf("success\n");
    }
    return 1;
}

void wait_epoll(int epfd,struct Threads_parameters tp)
{
    int count = 0;
    struct epoll_event events[tp.concurrency];

    while(count <= tp.connections)
    {
        int nfds=epoll_wait(epfd,events,20,500);
        if (nfds > 0){
            for (int i = 0; i < nfds; i++)
            {//如果连接成功
                if(events[i].events & EPOLLIN || events[i].events & EPOLLPRI
                   || events[i].events & EPOLLHUP)
                {
                    ssize_t rtn = 1;
                    char cBuf[512];
                    rtn =  recv(events->data.fd,cBuf,sizeof(cBuf),0);

                    if (rtn >=0 && 0==strncmp(cBuf,"HTTP/1.1 200 OK",15))//返回为200成功状态码
                    {
                        cn.epfd = epfd;
                        cn.event = events[i];
                        count++;
                        printf("%d\n",count);
                        close_connection(cn);
                    }
                    else
                    {
                        printf("bad request\n");
                    }

                }
                if(events[i].events & EPOLLOUT)
                {//TCP建立连接成功，发送ＨTTP请求
                    write_request(events[i].data.fd);
                }
                if(events[i].events & EPOLLERR)//TCP建立连接失败
                {//如果连接失败
                    perror("error: ");
                }

            }

        }
    }
}

void* start_thread(void* args)
{
    struct Threads_parameters tp = *((struct Threads_parameters*)args);

    int epfd = init_epoll();
    for (int i = 0; i < tp.concurrency; i++)
    {//创建concurrency个并发
        int temp = start_connect(epfd);
        if(-1 == temp)
        {//如果创建socket失败，重新创建
            i--;
        }
        else if (1 == temp)
        {
           // tp.connections--;
        }
    }
    wait_epoll(epfd,tp);//等等这concurrency个并发完成了connections次访问
}

void thread_init(int connect_num,int thread_num,int concurrent_num,struct Threads_parameters tp[])
{
    int connect_int = connect_num/thread_num;
    int connect_rem = connect_num%thread_num;
    int concurrent_int = concurrent_num/thread_num;
    int concurrent_rem = concurrent_num%thread_num;

    for (int i  = 0; i < thread_num ; i++)
    {
        if (i < connect_rem){
            tp[i].connections = connect_int+1;
        }
        else
        {
            tp[i].connections = connect_int;
        }

        if (i<concurrent_rem)
        {
            tp[i].concurrency = concurrent_int+1;
        }
        else
        {
            tp[i].concurrency = concurrent_int;
        }

#ifdef _DEBUG
        printf("%d %d\n",tp[i].connections,tp[i].concurrency);
#endif
    }
}

void print_reult(int connect_num)
{
    time_end();

    long time1 = start_time.tv_sec*1000 + start_time.tv_usec/1000;
    long time2 = end_time.tv_sec*1000 + end_time.tv_usec/1000;

    printf("HTTP每秒连接数：%ld\n",connect_num*1000/(time2-time1));
}

void wait_thread(pthread_t tid[],int thread_num)
{
    for (int i = 0; i < thread_num; i++)
    {
        pthread_join(tid[i],NULL);
    }
}

int socket_init(){
    struct in_addr **addr_list;
    struct hostent *ht;

    ht=gethostbyname(cn.host); //将域名转换为ip地址
    if(ht==NULL)
    {
        perror("url is not exit: ");
        exit(-1);
    }

    /*访问地址填充*/
    addr_list=(struct in_addr **)ht->h_addr_list;
    bzero(&srv_addr,sizeof(srv_addr)); //srv_addr填充0
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_port=htons(80);
#ifdef SERV_IP
    inet_pton(AF_INET, SERV_IP, &srv_addr.sin_addr.s_addr);
#else
    srv_addr.sin_addr=*addr_list[0]; //Web的IP地址
#endif

#ifdef _DEBUG
    printf("server ip is: %s\n",inet_ntoa(srv_addr.sin_addr));
#endif
}

void time_exit(int sig)
{
    printf("time is over\n");
    exit(0);
}

void print_error()
{
    printf("参数错误\n");
    printf("  -n 总连接数\n");
    printf("  -c 并发数\n");
    printf("  -d 线程数\n");
    exit(0);
}


void write_request(int fd)
{
    char cBuf[8192];
    send(fd,&cn.request, sizeof(cn.request),0);
}


int parse_url(char *url)
{
    char *cp;
    if(strlen(url)>7 && 0==strncmp(url,"http://",7))
    {
        url+=7;
    }
    if(NULL == (cp = strchr(url,'/')))
        return -1;
    memcpy(cn.host,url,cp-url);
    memcpy(cn.url,cp,strlen(cp));

    strcat(cn.request,"GET ");
    strcat(cn.request,cn.url);
    strcat(cn.request," HTTP/1.1\r\n");

    strcat(cn.request,"Host: ");
    strcat(cn.request,cn.host);
    strcat(cn.request,"\r\n");
    strcat(cn.request,"Connection: Keep-Alive\r\n\r\n");//长连接

    printf("%s\n",cn.host);
    printf("%s\n",cn.url);
    printf("%s\n",cn.request);

}

void close_connection(struct connection c)
{
    epoll_ctl(c.epfd,EPOLL_CTL_DEL,c.event.data.fd,NULL);
    close(c.event.data.fd);
    start_connect(c.epfd);
}