#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_CLNT 256

void * handle_clnt(void * arg);
int send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt=0; // 연결된 client들의 개수
int clnt_socks[MAX_CLNT];  // client들의 소켓을 저장하는 배열
char clnt_names[MAX_CLNT][NAME_SIZE]; // client들의 이름을 저장하는 배열
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL); // mutex 초기화
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); // IPv4, tcp통신 소켓 생성

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); // 자신의 ip주소를 네트워크 바이트로 
	serv_adr.sin_port=htons(atoi(argv[1])); // 입력받은 port주소를 정수-네트워크 바이트로
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1) // 연결을 받기 시작 (최대 5개까지 연결 대기 가능)
		error_handling("listen() error");
	
	while(1)
	{
		char name[NAME_SIZE];
		
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); // 접속할 때까지 대기
		
		pthread_mutex_lock(&mutx); // 임계구역에 한 스레드만 접근할 수 있도록 lock
		clnt_socks[clnt_cnt]=clnt_sock;
		pthread_mutex_unlock(&mutx); 
		
		read(clnt_sock, name, sizeof(name)); // 사용자에게서 이름을 받음
		
		pthread_mutex_lock(&mutx);
		strcpy(clnt_names[clnt_cnt++], name); // 클라이언트 이름 저장
		printf("name: %s\n", clnt_names[clnt_cnt-1]);
		pthread_mutex_unlock(&mutx);
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); // handle_clnt 함수를 스레드로 실행
		pthread_detach(t_id); // t_id 스레드의 종료를 감시
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	char msg[BUF_SIZE];
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
	{
		int op=send_msg(msg, str_len); // 메시지를 전송하는 함수를 호출하고 int값을 리턴받음 
		if(op==0){ // 일치하는 이름 없음
			char errmsg[16]="not found user\n"; 
			write(clnt_sock, errmsg, sizeof(errmsg));
		}
		else if(op==1) // 일치하는 이름 있음
			write(clnt_sock, msg, sizeof(msg));
	}
	
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)   // 클라이언트 연결 종료
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i <clnt_cnt-1)
			{
				clnt_socks[i]=clnt_socks[i+1];
				strcpy(clnt_names[i], clnt_names[i+1]);
				 i++;

			}

			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

int send_msg(char * msg, int len)   // @receiver에게 메시지 전송
{
	char recv[NAME_SIZE];
	
	char msg2[BUF_SIZE];
	int i=0, j=0;
	for(; !isspace(msg[i]); i++){ // 메시지에서 수신자만 가져와 저장
		recv[i]=msg[i];
	} 
	recv[i++]='\0';
	
	for(;msg[i]!='\0';i++,j++){ // 수신자 이후의 메시지 전문을 저장
		msg2[j]=msg[i];
	}
	msg2[j]='\0';
	
	if(!strcmp(recv,"all")){ // 수신자가 "all"이면 모두에게 전송
		pthread_mutex_lock(&mutx);
		for(i=0; i<clnt_cnt; i++)
			write(clnt_socks[i], msg2, len);
		pthread_mutex_unlock(&mutx);
		return 2;
	}
	else{
		pthread_mutex_lock(&mutx);
		for(i=0; i<clnt_cnt; i++){
			if(!strcmp(recv,clnt_names[i])){ // 이름이 일치하는 사용자를 조회
				write(clnt_socks[i], msg2, len);
				pthread_mutex_unlock(&mutx);
				strcpy(msg, msg2);
				return 1;
			}
		}
		pthread_mutex_unlock(&mutx);
	}
	return 0;
	
			
	
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
