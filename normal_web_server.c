#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>

#define MAXMSG  9999

int serv_sock; 

void signal_handler(int sig)
{
	if (sig == SIGINT)
	{
		printf("\nShutdown the server\n");
		close (serv_sock);
		exit(0);
	}
}

int hex2dec(char c)  
{  
	if ('0' <= c && c <= '9') {  
		return c - '0';  
	} else if ('a' <= c && c <= 'f') {  
		return c - 'a' + 10;  
	} else if ('A' <= c && c <= 'F') {  
		return c - 'A' + 10;  
	} else {  
		return -1;  
	}  
}

char * urldecode(char url[])
{  
	int i = 0;  
	int len = strlen(url);  
	int res_len = 0;  
	char * res = (char *) malloc(sizeof(char) * 1024);  
	for (i = 0; i < len; ++i) {  
		char c = url[i];  
		if (c != '%') {  
			res[res_len++] = c;  
		} else {  
			char c1 = url[++i];  
			char c0 = url[++i];  
			int num = 0;  
			num = hex2dec(c1) * 16 + hex2dec(c0);  
			res[res_len++] = num;  
		}  
	}  
	res[res_len] = '\0';  
	return res;  
} 

char* get_command(char *raw_data)
{

	//char *re;
	// const char needle[10] = "exec/";
	// re = strstr(raw_data, needle);
	//printf("raw_data: %s\n", raw_data);
	// char *token; 
	//const char *s = " "; 
	// token = strtok(re, s); 
	char *command;
	char *str;
	const char *f = "/";
	str = strstr(raw_data, f);
	command = str+1;
	//command = urldecode(command);
	//printf("test1: %s\r\n", command);
	//command = strtok(command, f);
	//printf("test2: %s\n", command);	
	//command = strtok(NULL, f);

	//printf("test3: %s\n", command);
	if (command[0] == '\0') 	return NULL;
	else 	return command;
}

char * exeCommand(char *command)
{
	FILE *fp;
	char * buf = (char *) malloc(9999);
	char buffer[9999];
	memset(buffer, 0, sizeof(buffer));
	memset(buf, 0, sizeof(buf));
	int ret = strcmp(command, "Control-C");
	if(ret == 0)
	{
		printf("Shutdown the server\n");
		close (serv_sock);
		exit(0);
	}

	else
	{
		fp = popen(command, "r");
		fread(buffer, sizeof(buffer), 1, fp);		
		pclose(fp);
		strcpy(buf, buffer);
		//printf("result:%s\n", buf);    //***********************************************************
		return buf;
	}
}

int read_from_client (int filedes)
{
	char buffer[MAXMSG];
	char info[MAXMSG];
	int nbytes;
	char *re;
	char *buf;
	const char *s = " ";
	char *ret;
	const char needle[10] = "/exec/";
	const char ndl[10] = "exec/";
	const char nedl[20] = " HTTP/1.1";
	char *NotFound = 
		"HTTP/1.1 404 Not Found\r\n\r\n";

	char* ok_response_template = 
		"HTTP/1.1 200 OK\r\n\r\n";

	static char* response_header = "HTTP/1.1 200 OK\r\n\r\n";
	char *resp;
	resp = (char *)malloc(9999);
	int len = strlen(NotFound);
	memset(buffer, 0, sizeof(buffer));
	nbytes = read (filedes, buffer, MAXMSG);
	printf ("Server: got raw message:%s\n", buffer);
	buf = urldecode(buffer);
	if (nbytes < 0)
	{
		/* Read error. Content-length: 47\n*/
		free(resp);
		perror ("read");
		exit (EXIT_FAILURE);
	}
	else if (nbytes == 0)
		/* End-of-file. */
	{
		printf("end of file\n");
		free(resp);
		return -1;
	}
	else
	{
		/* Data read. */
		//printf ("data:%s\n", buf);        //***********************************************************
		strcpy(info, buf);
		re = strtok(buf, s);
		//printf("info: %s\n", info);
		if (strcmp(re, "GET")==0)
		{
			//printf("get %s\n", re);   //***********************************************************
			ret = strstr(info, needle);
			//printf("ret: %s\n", ret);
			if (ret == NULL)
			{
				/* response 404 */
				//printf("no command\n");      //***********************************************************
				int suc = send(filedes, NotFound, len,0);
				if (suc<1)
				{
					printf("error!\n");
				}
				//printf ("sent message:%s\n", NotFound);  //***********************************************************
			}
			else
			{
				//printf("may have command\n");    //***********************************************************
				char *cmd = (char *)malloc(9999);
				ret = strstr(ret, ndl);
				cmd = strstr(ret, nedl);
				cmd[0] = '\0';
				//ret = strtok(ret, s);
				//printf("ret: %s\n", ret);     //***********************************************************
				//ret = urldecode(ret);
				//ret = strstr(ret, nedl);
				//printf("test2: %s\n", ret);
				// if (ret == NULL)
				// {
				// 	/* response 404 */
				// 	printf("just /exec\n");
				// 	send(filedes, NotFound, len,0);
				// 	printf ("sent message:%s\n", NotFound);
				// }
				//else
				//{
				char *command;
				command = get_command(ret);
				//printf("command: %s\n", command);     //***********************************************************
				if (command == NULL)
				{
					/* response 200 header only */

					int leng = strlen(response_header);
					send(filedes, response_header, leng,0);
					//printf ("sent message:%s\n", response_header); //***********************************************************
				}
				else
				{
					char *out = exeCommand(command);

					//printf("test1\n");
					memset(resp, 0, sizeof(resp));
					strcat(resp, ok_response_template);
					//printf("test1: %s\n", resp);
					strcat(resp, out);
					//printf("test2: %s\n", resp);
					//strcat(resp, response_end_template);
					//printf("test3: %s\n", resp);
					//printf("test2\n");
					//printf("out: %s\n", out);
					//strcat(resp, out);
					int len_out = strlen(resp);
					//printf("test3\n");
					/* response 200 */
					//write(filedes, resp, sizeof(resp));
					send(filedes, resp, len_out,0);
					//printf ("sent message:%s\n", resp);  //***********************************************************
					free(out); 

				} 
				//}   
				//free(cmd); 
			}
		}
		else
		{
			//printf("no GET\n");         //***********************************************************
			/* response 404 */
			//write(filedes, NotFound, sizeof(NotFound));
			send(filedes, NotFound, len,0);
			//printf ("sent message:%s\n", NotFound);    //***********************************************************
		}   
		//memset(res)	
		free(resp);
		return 0;
	}
}


int main(int argc, char *argv[])
{
	/* code */
	struct sockaddr_in clnt_addr;
	int port = atoi(argv[1]);
	//printf("port:%d\n", port);    //***********************************************************
	int true = 1;
	// FILE *fp;
	// char bu[100];
	// memset(bu,0,100);
	// fp = popen("ls", "r");
	// fread(bu, sizeof(bu), 1, fp);
	// printf ("test:%s\n", bu);
	// pclose(fp);

	serv_sock = socket(AF_INET, SOCK_STREAM, 0);  //IPPROTO_TCP
	if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1)
	{
		perror("Setsockopt");
		exit(1);
	}
	fd_set active_fd_set, read_fd_set;
	int i;
	int clnt_sock;
	if (serv_sock < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;   //sin_addr.s_addr = inet_addr(host)
	serv_addr.sin_port = htons(port);
	if (bind (serv_sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
	{
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	if (listen (serv_sock, 10) < 0)
	{
		perror ("listen");
		exit (EXIT_FAILURE);
	}

	FD_ZERO (&active_fd_set);
	FD_SET (serv_sock, &active_fd_set);

	if (signal(SIGINT, signal_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");

	while(1)
	{
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
		{
			perror ("select");
			exit (EXIT_FAILURE);
		}
		for (i = 0; i < FD_SETSIZE; ++i)
		{
			if (FD_ISSET (i, &read_fd_set))
			{
				if (i == serv_sock)
				{
					/* Connection request on original socket. */
					socklen_t clnt_addr_size = sizeof(clnt_addr);
					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
					if (clnt_sock < 0)
					{
						perror ("accept");
						exit (EXIT_FAILURE);
					}
					//printf("CONNECT\n");     //***********************************************************
					// printf ("Server: connect from host %s, port %hd.\n",
					// 		inet_ntoa (clnt_addr.sin_addr),
					// 		ntohs (clnt_addr.sin_port));
					FD_SET (clnt_sock, &active_fd_set);
				}
				else
				{
					/* Data arriving on an already-connected socket. */
					//printf("READ\n");       //***********************************************************
					// if (i == clnt_sock)
					// {
					// 	printf("test\n");
					// }
					read_from_client (i);
					close (i);
					FD_CLR (i, &active_fd_set);
				}
			}
		}
	}
}
