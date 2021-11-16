/*

师傅给的作业：
    工地，甲提供水泥，乙消耗，持续一个月，保存记录（甲乙每日提供或消耗水泥范围0-100）
client端
DATE:  2021-11-07
author：xiaowang

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "client.h"

#define SERIP "127.0.0.1"
#define PORT 8001
#define MAXLEN 1024


int PIV;//身份识别
struct MSG
{
    int weight_sum;
    int weight_yeslack;
    int weight_todaylack;
    int weight_add;
    int weight_min;
    int weight_need;
    int weight_plan;
    int fileflag;
    int filelength;
}msg;
/*
weight_plan：甲计划供给物料
weight_yeslack：昨日乙缺少
weight_add：甲实际供给
weight_need：今日乙需要
weight_min：乙实际消耗
weight_todaylack：今日乙缺少
weight_sum：剩余
*/

static sem_t sem_send;
static sem_t sem_recv;

int main(int argc, char const *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    //pthread_t send_thread,rcv_thread;
    FILE *fp = NULL;
    fp = fopen("./history.txt", "w+"); 
    fclose(fp);

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERIP);
    serv_addr.sin_port = htons(PORT);

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handing("connect() error");   
    }

    //客户端进行的身份识别，并发送给服务器
    printf("***********************************\n");
    printf("**         1:    甲方            **\n");
    printf("**         2:    乙方            **\n");
    printf("**        请输入您的身份:        **\n");
    printf("***********************************\n");

    scanf("%d",&PIV);
    fflush(stdin);
    if(PIV != 1 && PIV != 2)
    {
        printf("输入有误，请重新登录!\n");
        exit(1);
    }
    if(write(sock, &PIV, sizeof(PIV)) == -1)
    {
        error_handing("write() error");
    }

    switch (PIV)
    {
    case 1:
        handle_supply(sock);
        break;
    case 2:
        handle_consume(sock);   
    default:
        break;
    }
    return 0;
}

/*
甲供应处理函数
*/
void handle_supply(int sock)
{
    int i;
    pthread_t suprcv_thread;
    sem_init(&sem_send, 0, 1);
    sem_init(&sem_recv, 0, 0);
    pthread_create(&suprcv_thread, NULL, suprecv_msg, (void*)&sock);
    pthread_detach(suprcv_thread);
    for(i = 0; i <= 30; i++)
    {
        sem_wait(&sem_send);
        weight_supply();
        
        if(write(sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("write() error");
        }
        sem_post(&sem_recv);
    }
    
    sleep(3);
    sendfile(sock);   
}

/*
乙消耗处理函数
*/
void handle_consume(int sock)
{
    int i;
    pthread_t conrcv_thread;
    sem_init(&sem_send, 0, 0);
    sem_init(&sem_recv, 0, 1);
    pthread_create(&conrcv_thread, NULL, conrecv_msg, (void*)&sock);
    pthread_detach(conrcv_thread);
    for (i = 0; i <= 30; i++)
    {
        sem_wait(&sem_send);
        weight_consume();
        if(write(sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("write() error");
        }
        
        sem_post(&sem_recv);
    }
}

/*
供应方接收保存消息线程
*/
void * suprecv_msg(void * arg)
{
    if(arg == NULL)
    {
        printf("form arg NULL\n");
        exit(1);
    }
    int i = 1;
    int sock = *((int*)arg);
    while (1)
    {
        
        sem_wait(&sem_recv);
        if(i > 30)
        {
            break;
        }
        if(read(sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("read() error");
        }
        msg_save(i);
        i++;
        sem_post(&sem_send);
    }    
    return NULL;
}

/*
消耗方接收消息线程
*/
void * conrecv_msg(void * arg)
{
    if(arg == NULL)
    {
        printf("form arg NULL\n");
        exit(1);
    }
    int sock = *((int*)arg);
    while (1)
    {
        sem_wait(&sem_recv);
        if(read(sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("read() error");
        }
        sem_post(&sem_send);
    }
    return NULL;    
}

/*
供应方物料供给函数
*/
void weight_supply()
{
    int num;
    msg.weight_yeslack = msg.weight_todaylack;
    msg.weight_todaylack = 0;
    num = rand_num();
    msg.weight_plan = num;
    msg.weight_add = msg.weight_plan + msg.weight_yeslack;//实际供给
    msg.weight_sum = msg.weight_sum + msg.weight_add;

}

/*
客户物料消耗函数
*/
void weight_consume()
{
    int num;
    sleep(1);
    num = rand_num();
    msg.weight_need = num;
    msg.weight_min = msg.weight_need + msg.weight_yeslack;
    if(msg.weight_min <= msg.weight_sum)
    {
        msg.weight_todaylack = 0;
        msg.weight_sum = msg.weight_sum - msg.weight_min;
    }
    else
    {
        msg.weight_todaylack = msg.weight_min - msg.weight_sum;
        msg.weight_min = msg.weight_sum;
        msg.weight_sum = 0;
    }

}

/*
消息保存函数
*/
void msg_save(int i)
{
    FILE *fp = NULL;
    fp = fopen("./history.txt", "a+");
    if(fp == NULL)
    {
        error_handing("fopen() error");
    }
    if(fprintf(fp, "the<%d>day<**jia:plan:%dsup:%drealadd:%d**>\t\tyi:<**need:%dmin:%dtodaylack:%d**>remain_sum:%d\n", 
    i, msg.weight_plan, msg.weight_yeslack,msg.weight_add,msg.weight_need, msg.weight_min, msg.weight_todaylack, msg.weight_sum) < 0)
    {
        error_handing("fprintf() error");
    }
    fclose(fp);
}

/*
文件传输函数
*/
void sendfile(int sock)
{
    FILE *fp = NULL;
    int retbyte;
    int leftbyte;
    char buffer[MAXLEN];
    memset(&buffer, 0, sizeof(buffer));
    memset(&msg, 0, sizeof(msg));
    fp = fopen("./history.txt", "r");
    if(fp == NULL)
    {
        error_handing("fopen() error");
    }
    fseek(fp, 0, SEEK_END);
    msg.filelength = ftell(fp);
    msg.fileflag = 1;
    if(write(sock, &msg, sizeof(msg)) < 0)
    {
        error_handing("write fileflag error");
    }
    leftbyte = msg.filelength;
    rewind(fp);
    sleep(1);
    
    while((retbyte = fread(buffer, 1, sizeof(buffer), fp)) >0 )
    {
        printf("记录发送中...\n");
        if(write(sock, &buffer, sizeof(buffer)) < 0)
        {
            error_handing("write() error");
        }
        leftbyte -= retbyte;
        memset(&buffer, 0, sizeof(buffer));
        memset(&msg, 0, sizeof(msg));
        printf("文件剩余长度:%d\n",leftbyte);
        sleep(1);

        if(leftbyte <= 0)
        {
            leftbyte = 0;
            printf("文件传送完成\n");
            break;
        }
    }

    printf("success\n");
    fclose(fp);
}

/*
生成0-100随机数
返回值：随机数
*/
int rand_num()
{
    int n;
    srand((unsigned)time(NULL));
    n = rand()%101;
    return n;
}

/*
错误处理函数
*/
void error_handing(char *message)
{
    perror(message);
    fputc('\n',stderr);
    exit(1);
}
