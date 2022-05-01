#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define TIMEOUT 5

int ttl=1,port;
char sendbuf[]="hello world";
char recvbuf[2048];

int ip_location(char *ii){
	int sock,n;
	char headerbuf[1080];
	sprintf(headerbuf,"GET /line/%s?fields=217 HTTP/1.1\r\nHost: ip-api.com\r\nUser-Agent: curl/7.68.0\r\nAccept: */*\r\n\r\n",ii);
	char locrecv[2048];
	struct addrinfo hints,*res;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	char service[]="http";
	char ip[]="ip-api.com";

	if(getaddrinfo(ip,service,&hints,&res)!=0){
		printf("getaddrinfo error\n");
		return -1;
	}
	
	if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
		perror("socket error");
	}


	if(connect(sock,res->ai_addr,res->ai_addrlen)==-1){
		perror("connect error");
		return -1;
	}

	if((n=send(sock,headerbuf,sizeof(headerbuf),0))==-1){
		perror("send error");
	}
	read(sock,locrecv,sizeof(recvbuf));
	//printf("%s",recvbuf);

	
	char array[5][20];
	int i=170;
	for(int row=0;row<6;row++){
		for(int col=0;;col++){
			if(locrecv[i]=='\n'){
				array[row][col]='\0';
				i++;
				break;
			}else{
				array[row][col]=locrecv[i];
			}
			i++;
		}
	}
	printf("	%s,%s,%s",array[2],array[1],array[0]);
}


int getport(int s){
	struct sockaddr_in foo;
	foo.sin_family=AF_INET;
	foo.sin_addr.s_addr=INADDR_ANY;
	foo.sin_port=0;
	bind(s,(struct sockaddr *)&foo,sizeof(foo));

	struct sockaddr_in m;
	socklen_t size=sizeof(m);
	getsockname(s,(struct sockaddr *)&m,&size);

	return m.sin_port;
}


struct addrinfo * getaddr(char *ip){
	int e;
	struct addrinfo hints,*res;

	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	
	if((e=getaddrinfo(ip,0,&hints,&res))!=0){
		gai_strerror(e);
		exit(0);
	}
	struct sockaddr_in *s=(struct sockaddr_in *)res->ai_addr;
	s->sin_port=htons(6969);
	return res;
} 

void send_udp(int socket,struct addrinfo *a){
	if(setsockopt(socket,IPPROTO_IP,IP_TTL,&ttl,sizeof ttl)==-1){
		perror("setsockopt error");
	}
	if(sendto(socket,sendbuf,sizeof(sendbuf),0,a->ai_addr,a->ai_addrlen)==-1){
                perror("sendto error");
        }
	ttl+=1;	
}

void recv_icmp(int sock){
	fd_set fds;
	struct timeval t={TIMEOUT,0};
	int r=-2;
	struct sockaddr sender;
	socklen_t size=sizeof(sender);
	
	FD_ZERO(&fds);
	FD_SET(sock,&fds);

	if(select(sock+1,&fds,NULL,NULL,&t)==-1){
		perror("select error");
	}
	
	if(FD_ISSET(sock,&fds)){
		r=recvfrom(sock,recvbuf,sizeof(recvbuf),0,&sender,&size);
		if(r==-1){
			perror("recvfrom error");
			return;
		}
	}
	if(r==-2){ printf("\n*");return;}
	if(r>0){		
		struct ip *ip;
		struct icmp *icmp;
		icmp=(struct icmp *)(recvbuf+20);
		
		if(icmp->icmp_type==ICMP_TIMXCEED){
			struct udphdr *dudp;
			dudp=(struct udphdr *)(recvbuf+20+8+20);

			if(dudp->uh_sport==port){
				struct sockaddr_in *s=(struct sockaddr_in *)&sender;
				printf("\n%s",inet_ntoa(s->sin_addr));
				if(ttl>2){
					ip_location(inet_ntoa(s->sin_addr));	
				}else{
					printf("	your router");
				}
			}
		}else if(icmp->icmp_type==ICMP_UNREACH){
			if(icmp->icmp_code==ICMP_UNREACH_PORT){
				struct udphdr *final;
				final=(struct udphdr *)(recvbuf+20+8+20);
				if(final->uh_sport==port){
					struct sockaddr_in *s=(struct sockaddr_in *)&sender;
                                	printf("\n%s",inet_ntoa(s->sin_addr));
					ip_location(inet_ntoa(s->sin_addr));
					printf("\n");
					exit(0);
				}
			}
		}
	}

}


int main(int argc, char **argv){
	int udpsock,rawsock;
	struct addrinfo *addr=getaddr(argv[1]);

	if((udpsock=socket(addr->ai_family,SOCK_DGRAM,0))==-1){
		perror("socket error ");
		return -1;
	}
	if((rawsock=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP))==-1){
                perror("socket error ");
                return -1;
        }
	
	port=getport(udpsock);

	for(;;){
		send_udp(udpsock,addr);
		recv_icmp(rawsock);
	}

}
