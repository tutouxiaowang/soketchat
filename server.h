#ifndef _SERVER_H_
#define _SERVER_H_

extern void * handle_clnt(void * arg);
extern void handle_clnt_supply(int clnt_sock);
extern void handle_clnt_consume(int clnt_sock);
extern void cement_weightp_add(int weight_add);
extern int cement_weightp_min(int weight_min);
extern void error_handing(char *message);
extern void recvfile(int sock);

#endif
