#ifndef _CLIENT_H_
#define _CLIENT_H_

extern void handle_supply(int sock);
extern void handle_consume(int sock);
extern void * suprecv_msg(void * arg);
extern void * conrecv_msg(void * arg);
extern void weight_supply();
extern void weight_consume();
extern void msg_save();
extern int rand_num();
extern void error_handing(char *message);
extern void sendfile(int sock);

#endif