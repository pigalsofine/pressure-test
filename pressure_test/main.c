#include <stdio.h>
#include "pressureTest.h"


int main(int argc,char *argv[])
{
    pthread_t ntid;
    struct parameter p_num;
    parse_url("http://www.linuxdown.net/install/config/2016/0213/4682.html");//压测的ＵＲＬ

#ifdef _TEST
    p_num.connect_num = 1000; //总连接数
    p_num.thread_num = 1;//线程数
    p_num.concurrent_num = 1000;//并发数
#else
    p_num = parameter_init(argc,argv);
#endif

    printf("连接总数 %d, 并发数 %d, 线程数 %d\n",p_num.connect_num,p_num.concurrent_num,p_num.thread_num);

    signal(SIGALRM, time_exit);
    alarm(LAST_TIME);//定最长运行时间


#ifdef thread_pool//检查是否使用线程池技术
    if(pthread_mutex_init(&mutex, NULL) < 0)
        perror("sem_init error");
    struct threadpool *pool = threadpool_init(10, 20);
#endif
    socket_init();//初始化socket，由URL得到ip地址

    /*记录每个线程应该的并发数和总连接数*/
    struct Threads_parameters tp[p_num.thread_num];
    thread_init(p_num.connect_num,p_num.thread_num,p_num.concurrent_num,tp);//对传递给线程的参数的数据结构初始化

    time_start();//记录开始就时间

    //创建线程
    pthread_t tid[p_num.thread_num+1];
    for (int i = 0; i < p_num.thread_num; i++) {
#ifdef thread_pool//检查是否使用线程池技术
        threadpool_add_job(pool, start_thread, (void*)&(tp[i]));
#else
        int temp;
        if((temp=pthread_create(&ntid,NULL,start_thread,(void*)&(tp[i]))))
        {
            printf("catn't create thread: %s\n",strerror(temp));
            return 1;
        }
        tid[i] = ntid;
#endif
    }


#ifdef thread_pool
    threadpool_destroy(pool);
#else
    wait_thread(tid,p_num.thread_num);//等待所有线程结束
#endif


    print_reult(p_num.connect_num);//打印结果
    return 0;
}