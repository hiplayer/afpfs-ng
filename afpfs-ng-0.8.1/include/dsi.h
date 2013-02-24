
#ifndef __DSI_H_
#define __DSI_H_

#include "afp.h"

struct dsi_request
{
	unsigned short requestid;
	unsigned char subcommand;
	void * other;
	unsigned char wait;
	pthread_mutex_t     mutex;
	pthread_cond_t  condition_cond;
	struct dsi_request * next;
	int return_code;
};

int dsi_receive(struct afp_server * server, void * data, int size);
int dsi_getstatus(struct afp_server * server);

int dsi_opensession(struct afp_server *server);
int afp_disconnectoldsession(struct afp_server * server, int type, struct afp_token* token);

int dsi_send(struct afp_server *server, char * msg, int size,int wait,unsigned char subcommand, void ** other);
int dsi_send_sys(struct afp_server *server, char * msg, int size,int wait,unsigned char subcommand, void ** other);
struct dsi_session * dsi_create(struct afp_server *server);
int dsi_restart(struct afp_server *server);
int dsi_recv(struct afp_server * server);

#define DSI_BLOCK_TIMEOUT 30
#define DSI_DONT_WAIT 0
#define DSI_DEFAULT_TIMEOUT 5
#define DSI_FAST_TIMEOUT 1
#define DSI_RETRY_TIMES 3


#endif
