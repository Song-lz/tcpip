#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#define PORT 			7896
#define SELECT_HANDLERS   16

#define SELECT_NFDS_UPDATE(__obj) \
{\
	unsigned char __i; \
	for(__obj->nfds = __i = 0; __i < SELECT_HANDLERS; i++){\
		if(__obj->handlers[i]){\
			__obj->nfds = ((__obj->nfds > __obj->handlers[i]->fd) ? __obj->nfds :  __obj->handlers[i]->fd);\
		}\
	}\
}\


typedef struct SelectHandlerST {
	int fd;
	bool activeflag;
	void (*cb)(void *);
	void *data;
} SelectHandlerST;

typedef struct selectObjST {
	int nfds;
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	bool enable;
	SelectHandlerST *handlers[SELECT_HANDLERS];
} selectObjST;


int main(int argc, char **argv)
{

	tcpListen();

	

	return 0;
}


int tcpListen(char *ip, int port)
{
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd < 0){
		perror("socket err");
	}
	struct sockaddr_in addr;
	/* 本机字节顺序转化为网络字节顺序,小端转大端 */
	addr.sin_port = htonl(PORT);
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_family = AF_INET;
	bind(socketfd, (struct sockaddr *)&addr, (socklen_t )sizeof(struct sockaddr_in));
	
}

selectObjST *selectObjGet(void)
{
	static selectObjST obj;

	return &obj;
}


void selectLoop(void)
{
	int i, events = 0;
	SelectHandlerST handler;
	selectObjST *obj = selectObjGet();
	if(obj->enable != true)
	{return;}
	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	FD_ZERO(&obj->rfds); 
	
	for(i=0; i< SELECT_HANDLERS; i++){
		if(obj->handlers[i]){
			FD_SET(obj->handlers[i]->fd, &obj->rfds);
			obj->handlers[i]->activeflag = true;
		}
	}

	events = select(obj->nfds, &obj->rfds, NULL, NULL, &timeout);
	for(i=0; i< SELECT_HANDLERS && events > 0; i++){
		if(obj->handlers[i]){
			if(FD_ISSET(obj->handlers[i]->fd, &obj->rfds)){
				events--;
				if(obj->handlers[i]->cb){
					memcpy(&handler, obj->handlers[i], sizeof(SelectHandlerST));
					handler.cb(handler.data);
				}
			}
		}
	}
	
}

int selectAdd(int fd, void(*recvcb)(void *), void *data)
{
	int i, index =-1;
	selectObjST *obj = selectObjGet();

	if(fd < 0){
		return -1;
	}
	for(i=0; i<SELECT_HANDLERS && index == -1; i++){
		if(!obj->handlers[i])
		{index = i;}
	}

	if(index == -1)
	{return -1;}

	obj->handlers[i] = (SelectHandlerST*)malloc(sizeof(SelectHandlerST));
	if(!obj->handlers[i])
	{return -1;}
	obj->handlers[i]->activeflag = true;
	obj->handlers[i]->fd = fd;
	obj->handlers[i]->cb = recvcb;
	obj->handlers[i]->data = data;
	SELECT_NFDS_UPDATE(obj);
	
}

int selectDel(int fd)
{
	int i;
	selectObjST *obj = selectObjGet();

	if(fd < 0){
		return -1;
	}

	for(i=0; i<SELECT_HANDLERS; i++){
		if(obj->handlers[i])
		{
			if(obj->handlers[i]->fd == fd)
			{
				FD_CLR(fd, &obj->rfds);
				close(fd);
				free(obj->handlers[i]);
				obj->handlers[i] = NULL;
				//SELECT_NFDS_UPDATE(obj);	
			}
		}
	}
	
}


void selectExit(void)
{
	int i;
	selectObjST *obj = selectObjGet();

	FD_ZERO(&obj->rfds);
	for(i=0; i<SELECT_HANDLERS; i++){
		if(obj->handlers[i])
		{
			close(obj->handlers[i]->fd);
			free(obj->handlers[i]);
			obj->handlers[i] = NULL;
			//SELECT_NFDS_UPDATE(obj);	

		}
	}
	
}


