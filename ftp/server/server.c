#include <stdio.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>

int init_server()
{

	// 1. socket 创建了一个基于数据报 TCP 的 套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in saddr;

	// 初始化 服务器的地址
	saddr.sin_family = AF_INET; // ipv4 协议
	saddr.sin_port = htons(3001);
	saddr.sin_addr.s_addr = INADDR_ANY; // 让内核自己选一个合适的IP来绑定
	// 2. bind
	// 设置sockfd可以重复使用port
	int on = 1;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
	printf("ret = %d\n", ret);

	if (bind(sockfd, (const struct sockaddr *)&saddr, sizeof(saddr)))
	{
		perror("bind");
	}

	listen(sockfd, 5);

	return sockfd;
}

int main()
{
	struct sockaddr_in caddr;
	socklen_t addrlen = sizeof(caddr);
	int newfd;
	pid_t pid;
	char cmd[20];

	int sockfd = init_server();
	while (1)
	{
		// newfd  用来跟 客户 通信的套接字
		newfd = accept(sockfd, (struct sockaddr *)&caddr, &addrlen);
		printf("有一个客户端连接服务器:newfd=%d\n", newfd);
		pid = fork();
		if (pid == 0) // 子进程 跟 newfd客户端 通信
		{
			int fd, n;
			char buf[1024];
			char fbuf[1024];
			char *p;
			while (1)
			{
				memset(cmd, 0, sizeof(cmd));
				recv(newfd, cmd, 20, 0); // 接收客户端的命令:ls   get         put   quit
				if (0 == strcmp(cmd, "ls"))
				{
					system("ls > 1.txt");
					fd = open("1.txt", O_RDONLY);
					n = read(fd, buf, 1024);
					send(newfd, buf, n, 0); // 发送服务器的文件列表给 客户端
					close(fd);
				}
				else if (0 == strncmp(cmd, "get", 3)) // get 1.txt
				{
					p = strtok(cmd, " ");
					p = strtok(NULL, " ");
					printf("客户要下载的文件:%s\n", p);
					// 读 1.txt 这个文件
					fd = open(p, O_RDONLY);
					if (fd < 0)
					{
						printf("没有这个文件\n");
						send(newfd, "error", 5, 0); // 告诉客户端
						continue;
					}
					while (1)
					{
						n = read(fd, buf, 1024);

						send(newfd, buf, n, 0);
						printf("have send %d byte\n", n);
						if (n < 1024)
							break;
					}
					close(fd);
				}
				else if (0 == strncmp(cmd, "put", 3)) // put 1.txt
				{
					p = strtok(cmd, " ");
					p = strtok(NULL, " ");
					printf("客户要上传的文件为：%s\n", p);
					p[strlen(p)] = 0;
					fd = open(p, O_WRONLY | O_CREAT, 0644); // 创建文件
					if (fd < 0)
						perror("open");
					while (1)

					{
						n = recv(newfd, fbuf, sizeof(fbuf), 0);
						if (strncmp(fbuf, "error", 5) == 0)
						{
							printf("没有这个文件\n");
							break;
						}
						write(fd, fbuf, n);
						printf("have write %d bytes\n", n);
						if (n < sizeof(fbuf))
							break;
					}
					close(fd);
					break;
				}
				else if (0 == strncmp(cmd, "quit", 4)) // quit
				{
					printf("%d socket leave\n", newfd);
					close(newfd);
					exit(0); // 子进程退出
				}
			}
		}
	}
}
