#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define PORT 			7896
#define SELECT_HANDLERS   16
#define LISTEN_NUM_MAX    10
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

int tcpListen(int port);

void *pthread_socket(void *arg);


int main(int argc, char **argv)
{
	printf("1\r\n");
	sleep(1);
	pthread_t pthread_select;
	printf("2\r\n");
	tcpListen(PORT);
	printf("3\r\n");

	
	pthread_create(&pthread_select, NULL, pthread_socket, NULL);
	printf("4\r\n");

	pthread_join(pthread_select, NULL);




	return 0;
}
void selectLoop(void);
void *pthread_socket(void *arg)
{
	while(1){
		selectLoop();
	}
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
	printf("%s %d\r\n", __func__, __LINE__);

	if(index == -1)
	{return -1;}
	printf("%s %d\r\n", __func__, __LINE__);

	obj->handlers[i] = (SelectHandlerST*)malloc(sizeof(SelectHandlerST));
	printf("%s %d\r\n", __func__, __LINE__);
	if(!obj->handlers[i])
	{return -1;}
	obj->handlers[i]->fd = fd;
	printf("%s %d\r\n", __func__, __LINE__);
	obj->handlers[i]->cb = recvcb;
	printf("%s %d\r\n", __func__, __LINE__);
	obj->handlers[i]->data = data;
	SELECT_NFDS_UPDATE(obj);
	printf("%s %d\r\n", __func__, __LINE__);
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



void peer_client(void *arg)
{
	char buf[50]={0};
	ssize_t err = read((int)arg, buf, sizeof(buf));
	if(err == 0){
		selectDel((int)arg);
	}
}
void host_listen(void *arg)
{
	struct sockaddr addr;
	socklen_t len;
	int clientfd =  accept((int)arg, &addr, &len);
	if(clientfd < 0){
		perror("accept err");
		return;
	}
	selectAdd(clientfd, peer_client, (void *)clientfd);
}


int tcpListen(int port)
{
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd < 0){
		perror("socket err");
		return -1;
	}
	struct sockaddr_in addr;
	/* 本机字节顺序转化为网络字节顺序,小端转大端 */
	addr.sin_port = htonl(PORT);
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_family = AF_INET;
	printf("%s %d\r\n", __func__, __LINE__);
	bind(socketfd, (struct sockaddr *)&addr, (socklen_t )sizeof(struct sockaddr_in));
	printf("%s %d\r\n", __func__, __LINE__);
	listen(socketfd, LISTEN_NUM_MAX);	
	printf("%s %d\r\n", __func__, __LINE__);
	selectAdd(socketfd, host_listen, (void *)socketfd);
	printf("%s %d\r\n", __func__, __LINE__);
}

