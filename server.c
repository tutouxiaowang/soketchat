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
#include "server.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8001
#define MAXLEN 1024

int PIV;//身份识别
int sup_sock, con_sock;

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


static sem_t sem_online;
static sem_t sem_sup;
static sem_t sem_con;

int main(int argc, char const *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    char message[50];
    socklen_t clnt_addr_len;
    pthread_t t_id;

    sem_init(&sem_online, 0, 0);
    sem_init(&sem_sup, 0, 1);
    sem_init(&sem_con, 0, 0);
    msg.fileflag = 0;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
    {
        error_handing("socket() error");
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =  inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handing("bind() error");
    }

    if(listen(serv_sock, 2) == -1)
    {
        error_handing("listen() error");
    }

    while (1)
    {
        clnt_addr_len = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
        if(clnt_sock == -1)
        {
            error_handing("accept() error");
        }
        pthread_create(&t_id,NULL,handle_clnt,(void *)&clnt_sock);
        pthread_detach(t_id);             
    }
    
    return 0;
}
/*
对连接的客户端身份识别
*/
void * handle_clnt(void * arg)
{
    int clnt_sock = *((int*)arg);
    int ret = read(clnt_sock, &PIV, sizeof(PIV));
    switch (PIV)
    {
    case 1:
        handle_clnt_supply(clnt_sock);
        break;
    case 2:
        handle_clnt_consume(clnt_sock);
        break;    
    default:
        break;
    }
}
/*
甲消息
*/
void handle_clnt_supply(int clnt_sock)
{
    printf("甲上线!\n");
    int i;
    sup_sock = clnt_sock;
    sem_wait(&sem_online);
    while(1)
    {
        sem_wait(&sem_sup);
        if(read(sup_sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("read() error");
        }
        printf("甲提供:%d\t",msg.weight_add);

        if(write(con_sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("write() error");
        }
        sem_post(&sem_con);

        if(msg.fileflag == 1)
        {
            sleep(1);
            printf("文件开始传输\n");
            recvfile(sup_sock);
            printf("文件传输完成\n");
            break;
        }
    }
    close(sup_sock);
    printf("甲断开连接\n");
    //read(sup_sock, &msg, sizeof(msg));    
}
/*
乙消息
*/
void handle_clnt_consume(int clnt_sock)
{
    printf("乙上线\n");

    con_sock = clnt_sock;
    sem_post(&sem_online);
    while (1)
    {
        sem_wait(&sem_con);
        if(read(con_sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("read() error");
        }
        printf("消耗 %d\t",msg.weight_min);
        printf("缺少:%d\t",msg.weight_todaylack);
        printf("剩余:%d\n",msg.weight_sum);
        if(write(sup_sock, &msg, sizeof(msg)) == -1)
        {
            error_handing("write() error");
        }
        sem_post(&sem_sup);

        if(msg.fileflag == 1)
        {
            break;
        }
    }
    close(con_sock);
    printf("乙断开连接\n");

}
/*
文件接收
*/
void recvfile(int sock)
{
    FILE * readfp = NULL;
    FILE * fp =NULL;
    int retbyte;
    int leftbyte;
    char buffer[MAXLEN];
    memset(&buffer, 0, sizeof(buffer));

    leftbyte =msg.filelength;
    fp = fopen("./log.txt","w+");
    if(fp == NULL)
    {
        error_handing("fopen() error");
    }
        
    while ((retbyte = read(sock, buffer, sizeof(buffer))) > 0)
    {
        printf("文件接收中...\n");
        if(fwrite(buffer, 1, retbyte, fp) < 0)
        memset(buffer, 0, sizeof(buffer));
        leftbyte -= retbyte;
        printf("文件剩余长度：%d\n",leftbyte);
        if(leftbyte <= 0)
        {
            printf("文件接收完成,长度:%d\n",msg.filelength);
            msg.fileflag = 0;
            leftbyte = 0;   
            break;
        }
        
    }
    fclose(fp);
}

/*
错误处理
*/
void error_handing(char *message)
{
    perror(message);
    fputc('\n',stderr);
    exit(1);
}
