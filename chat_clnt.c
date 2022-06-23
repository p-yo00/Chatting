#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100
#define NAME_SIZE 20
	
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
	
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
	
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	 }
	
	sprintf(name, "[%s]", argv[3]);
	sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) // server 소켓에 연결 시도
		error_handling("connect() error");
	
	write(sock, argv[3], sizeof(argv[3])); // 이름 (3번째 매개변수) 서버로 전송
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); // send_msg 스레드 실행
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); // recv_msg 스레드 실행
	pthread_join(snd_thread, &thread_return); // snd_thread 종료 감시 (blocking)
	pthread_join(rcv_thread, &thread_return); // rcv_thread 종료 감시 (blocking)
	close(sock);  
	return 0;
}
	
void * send_msg(void * arg)   // 서버로 송신
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+NAME_SIZE+BUF_SIZE+1];
	while(1) 
	{
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
		{
			close(sock);
			exit(0);
		}
		char recv[NAME_SIZE], msg2[BUF_SIZE];
		int i=1, j=0;
		for(; !isspace(msg[i]); i++){ // 메시지에서 수신자만 가져와 저장 (@도 제거)
			recv[i-1]=msg[i];
		} 
		recv[i-1]='\0';
		
		for(;msg[i]!='\0';i++,j++){ // 수신자 이후 메시지 저장
			msg2[j]=msg[i];
		}
		msg2[j]='\0';
		
		sprintf(name_msg,"%s %s%s",recv, name, msg2); // 수신자, 이름, 메시지 순으로 포맷
		
		write(sock, name_msg, sizeof(name_msg));
	}
	return NULL;
}
	
void * recv_msg(void * arg)   // 서버로부터 수신
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	int str_len;
	while(1)
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if(str_len==-1) 
			return (void*)-1;
		name_msg[str_len]=0;
		fputs(name_msg, stdout);
	}
	return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
