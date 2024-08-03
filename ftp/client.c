#include <stdio.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>

int init_client()
{
	//1. socket 
	// sockfd 用来绑定的描述符
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//定义1个 好用的 sockaddr_in
	struct sockaddr_in saddr;
	socklen_t saddrlen = sizeof(saddr);
	
	saddr.sin_family = AF_INET; //ipv4
	saddr.sin_port  = htons(3001);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	//2. 连接服务器 connect 
	
	if(connect(sockfd, (const struct sockaddr *)&saddr,  saddrlen))
		perror("connect");

	

	return sockfd; 
}

int menu()
{
	printf("1. 列表服务器的文件\n");
	printf("2. 列表客户端的文件\n");
	printf("3. 下载服务器的文件\n");
	printf("4. 上传客户端文件到 服务器\n");
	printf("5. 退出\n");
	int ch;
 	scanf("%d", &ch);
	scanf("%*[^\n]");
	scanf("%*c");
	return ch;
}

int main(int argc, char *argv[])
{
	char buf[100]={};
	char fbuf[1024];
	char cmd[50];
	int ch,n,fd;
	char *p;
	
	// 初始化网络
	int sockfd = init_client();
	while(1)
	{
		ch = menu();
		switch(ch)
		{
			case 1: //列表服务器的文件
				send(sockfd, "ls", 2, 0 ); 
				//清空 fbuf  : memset,  bzero
				memset(fbuf,0,1024); 
				//bzero(fbuf,1024);
								
				//接收 服务器发送来的 文件列表信息				
				n = recv(sockfd, fbuf, 1024,0);
				printf("%s\n", fbuf);
				
				break;
			case 2: //列表客户端的文件
				system("ls");
				
				break;
			case 3: //下载服务器的文件
				printf("请输入要下载的文件:例如:get xxx \n");
				fgets(cmd, sizeof(cmd),stdin); // 从键盘读 
				printf("cmd=%s\n", cmd);
				send(sockfd, cmd, strlen(cmd)-1, 0 ); //  -1 去掉最后的 \n  
				p = strtok(cmd, " ");
				p = strtok(NULL, " ");
				printf("客户要下载的文件:%s\n", p);
				p[strlen(p)-1]=0;
				// 我创建一个文件 写
				fd = open(p, O_WRONLY|O_CREAT,0644);
				if(fd < 0)
					perror("open");
				
				while(1)
				{
					n = recv(sockfd, fbuf,sizeof(fbuf),0);
					if(strncmp(fbuf,"error",5)==0) 
					{
						printf("没有这个文件\n");
						break;
					}
					write(fd, fbuf, n);	
					printf("have write %d byte\n",n);
					if(n < sizeof(fbuf))
						break;
				}
				close(fd);
				
				break;
			case 4: //上传客户端文件到 服务器
				printf("请输入要上传的的文件:例如:put xxx \n");
				fgets(cmd, sizeof(cmd),stdin); // 从键盘读 
				printf("cmd=%s\n",cmd);
				send(sockfd, cmd, strlen(cmd)-1, 0 ); //  -1 去掉最后的 \n  
				p = strtok(cmd, " ");
				p = strtok(NULL, " ");
				printf("客户要上传的文件:%s\n", p);
				p[strlen(p)-1]=0;
				fd=open(p,O_RDONLY);
				if(fd<0)
				{
					printf("没有该文件。\n");
					send(sockfd,"error",5,0);
					continue;
				}
				while(1)
				{
					n = read(fd,buf,1024);
					send(sockfd,buf,n,0);
					printf("have send %d bytes\n",n);
					if(n<1024)
					{
						break;
					}
				}
				close(fd);

                




				
				break;
			case 5: //退出
				send(sockfd, "quit", 4, 0 ); 
				printf("I am leave\n");
				exit(0);

		}
	}
}

