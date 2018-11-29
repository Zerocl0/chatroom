#include "chatroom.h"
/*
   client.c
   client    客户端
 */
 
int socketfd;  //套接字描述符
int admin_flag=NORMAL_USER;   //用户的权限标记，默认为0

//菜单提示结构体   
typedef struct {
	char cmd[40];			//命令格式
	char explain[100];		//说明
	int  admin;				//权限
}usage;

//0为普通用户具有的执行权限 ，1为管理员（目前还没做管理员相关功能）
 usage help_menu[] = {
	{"格式", 			        "\t\t说明",0},
	{"/(content)", 			    "\t向所有人发送消息(eg /abc)",0},
	{":(user)/(content)",		"向某指定用户发送消息(eg :1001/abc)",0},
	{"-online" ,            	"\t\t显示在线用户",0},
	{"-help" ,              	"\t\t显示帮助菜单",0},
	{"-file" ,  				"\t\t传输文件给某用户"},
	{"exit",		            "\t\t离开聊天室",0},
	{0,0,0}
}; 

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc ,char *argv[])
{
	signal(SIGINT, signHandler); 
	init_conn();
	close(socketfd);
	return 0;
}

// 新线程接收服务端数据
void *pthread_fun(void *arg)
{
	char buf[BUF_SIZE]={0};
	int length=0;
	tran_info   tran,*p;
	p=&tran;
	while(1)
	{
		length=read(socketfd,&tran,sizeof(tran));
		if(length<=0)
		{
			printf("服务器已关闭!\n");
			exit(EXIT_SUCCESS);	
			break;
		}
		if(p->type==FILE_FUN)
			recv_file(p);
		else
			printf("%s\n",p->buf);
		memset(&tran,0,sizeof(tran));
		memset(buf,0,sizeof(buf)); // clear  buf
	}
	close(socketfd);
	pthread_exit(NULL);
	exit(EXIT_SUCCESS);	
}

//初始化连接
void  init_conn()
{
	struct sockaddr_in   server;
	if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1)
		print_err("socket error ");
	memset(&server,0,sizeof(server));
	// bzero(&(server.sin_zero),8);  
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT);
	inet_pton(AF_INET,IP,&server.sin_addr);
	if(( connect(socketfd,(struct sockaddr *)&server,sizeof(struct sockaddr)))==-1)
		print_err("connect error ,server not start,");
	
	reg_log(); //登陆或注册
	
	pthread_t tid;
	if((pthread_create(&tid,NULL,pthread_fun,&socketfd))==-1)
		print_err("client pthread_create() error");
	
   	send_fun();
}

//进行选择注册或登陆 。
void reg_log()
{
	char ch;
	printf("\tl-->>(login)\tr-->>(register)\n");
	printf("请输入'l' 或 'r' :\n");
	while(1)
	{
		ch=getchar();
		if(ch=='l')
		{
			int get_ret=0;
			while(1)
			{ 
				send_info  send;
				printf("请输入用户名：\n"); 
				isvalid (send.name);
				printf("请输入密码  ：\n");
				isvalid (send.passwd);
				send.type=CLIENT_LOGIN;
				write(socketfd,&send,sizeof(send));
				read(socketfd,&get_ret,sizeof(get_ret));
				if(get_ret==NORMAL_USER_LOGIN_SUCCESS)//1
				{
					printf("登陆成功，欢迎来到聊天室....\n\n");
					admin_flag =NORMAL_USER;//0普通用户
					show_menu();
					break;
				}				
				else if(get_ret==ADMIN_LOGIN_SUCCESS)//3
				{ 
					printf("管理员登录成功，欢迎来到聊天室....\n\n");
					admin_flag =ADMIN_USER;//1管理员
					show_menu();
					break;
				}
				else if(get_ret==NORMAL_USER_LOGIN_FAILED_ONLINE)//2
				{
					printf("登录失败，此用户已在线！\n");
					exit(EXIT_SUCCESS);
					break;
				}
				else if(get_ret==NORMAL_USER_LOGIN_PASSWD_ERROR) //4
					printf("登录失败，密码错误，请重新登陆！\n");
				else//0
					printf("登录失败，账户不存在，请重新登陆！\n");
			}
			break;
		}
		else if(ch=='r')
		{
			int get_ret=0;
			while(1)
			{
				pthread_mutex_lock(&mutex);
				send_info  send;
				printf("注册用户名：\n");
				isvalid (send.name);
				printf("注册密码  ：\n");
				isvalid (send.passwd);
				send.type=CLIENT_REGISTER;
				write(socketfd,&send,sizeof(send));
				pthread_mutex_unlock(&mutex);
				read(socketfd,&get_ret,sizeof(int));
				if(get_ret==REGIST_EXITED)//账号已经存在 1
					printf("注册失败,此帐号已存在,请更换用户名再试!\n ");
 				else if(get_ret==REGIST_FALIED)//注册失败 0
					printf("注册失败,请重试!\n ");
				else 
				{
					printf("注册成功,你的帐号ID为：%d ,请重新登陆即可聊天了。\n ",get_ret);
					exit(EXIT_SUCCESS);	
					break;
				}
			}
			break;
		}
		else
		{
			printf("enter error!please try again,enter 'l'or 'r' .\n");
		}
		//清空输入流.
		for(  ;   (ch=getchar())!='\n'  && ch !=EOF; )
			continue;
	}      
}

//发送数据
void send_fun()
{
	char buf[BUF_SIZE]={'\0'};
	send_info  send; 
 	while(1)
	{
		gets(buf); 
		parse_input_buf(buf, &send); 
		if(strcmp("-help",buf)==0) 
			continue;	
		else if(strcmp("-file",buf)==0)
		{
			send_file();
			continue;
		}
		else
			write(socketfd, &send, sizeof(send));
		if(strcmp("exit",buf)==0) 
	  	{   
	  		close(socketfd);
			exit(EXIT_SUCCESS);
	  	}
		memset(buf,0,sizeof(buf));
	}
}
//解析字符串，填充发送数据包
void parse_input_buf(char *src,  send_info  *send)
{
  switch(src[0]){
	case '/' : 
		send->type=PUBLIC_CHAT;
		strcpy(send->buf,src+1);
		break;
	case ':' :
	    strcpy(send->id,strtok(src+1,"/"));
		DEBUG("%s\n",send->id);
		send->type=PRIVATE_CHAT; 
		strcpy(send->buf,strtok(NULL,"/"));
		break;
	case '-' :
		if(strcmp(src,"-online")==0)
			send->type=CLIENT_ONLINE;  
		else if(strcmp(src,"-file")==0)
			send->type=SEND_FILE; 
		else if(strcmp(src,"-help")==0)
			show_menu();
		break;
	default :
		send->type=0;
		strcpy(send->buf,src); 
		break;
	}
} 

//客户端文件发送
void send_file()
{
	//Input the file name
	send_info send;
	char filename[FILE_NAME_SIZE];
	bzero(filename,FILE_NAME_SIZE);
	printf("请输入你想要发送的文件名:");
	scanf("%s",&filename);
	getchar();
	printf("请输入你想要发送的用户名:");
	scanf("%s",send.id);
	getchar();
	send.type=SEND_FILE;
	DEBUG("send.type=%d\n",send.type);
	
	bzero(send.buf,BUF_SIZE);
	strcpy(send.buf,filename);
	if(write(socketfd,&send,sizeof(send))<0)
		print_err("send file information error");
	
	//read file 
	FILE *fd=fopen(filename,"rb");
	if(fd==NULL)
		printf("File :%s not found!\n",filename);
	else 
	{
		bzero(send.buf,BUF_SIZE);
		int file_block_length=0;
		while((file_block_length=fread(send.buf,sizeof(char),BUF_SIZE,fd)))
		{
			DEBUG("send.type=%d\n",send.type);
			DEBUG("send.buf=%s\n",send.buf);
			printf("file_block_length:%d\n",file_block_length);
			if(write(socketfd,&send,sizeof(send))<0)
				print_err("Send error");
			bzero(send.buf,BUF_SIZE);   
		}
		fclose(fd);
		printf("Transfer file finished !\n");
	}
}
//客户端文件接收
void recv_file(tran_info *tran)
{
DEBUG("recv file\n");
	tran_info recv;
	char filename[FILE_NAME_SIZE];
	char str[BUF_SIZE];
	strcpy(filename,tran->buf);

	//recv file
	FILE *fd=fopen(filename,"wb+");
	if(NULL==fd)
		print_err("open");
	
	bzero(tran->buf,BUF_SIZE);

	int length=0;
	while((length=read(socketfd,&recv,sizeof(recv))))
	{
		DEBUG("recv=%s\n",recv.buf);
		if(length<0)
			print_err("recv");
		strcpy(str,recv.buf);
		int writelen=fwrite(str,sizeof(char),length,fd);
		if(writelen<length)
			print_err("write error");
		bzero(str,BUF_SIZE);
	}

	printf("Receieved file:%s finished!\n",filename);
	fclose(fd);
}

//显示说明菜单
void show_menu()
{
	int i = 0;
	for(i=0; help_menu[i].cmd[0] != 0; i++)
	{
		if(admin_flag==ADMIN_USER) //管理员
			printf("\t\t%s\t\t%s\n", help_menu[i].cmd, help_menu[i].explain);
		else if(help_menu[i].admin==NORMAL_USER)//普通用户
		   	printf("\t\t%s\t\t%s\n", help_menu[i].cmd, help_menu[i].explain);
	}
}

//注册时判断是否含有非法字符（空格，标点或特殊符号）
void isvalid (char   *str)
{
   while(1)
   { 
        scanf("%s",str);
        int  i=0 ,flag=-1;
		for(i=0;str[i]!=0;i++)
		{ 
			if(ispunct(str[i]))
			{
  		        flag=i;
  		        break;		   
  		    }
		}
		if (flag!=-1)
		{
		    printf("包含非法字符，请重试。\n");
			bzero(str,sizeof(str));
		}
		else break;
 	}
}

//报错函数
void  print_err(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

//忽略ctrl +c 键的处理函数
void signHandler(int signNo)
{
	//DEBUG("singal:%d \n",signNo);
    printf("若要退出聊天室，请按输入 exit 即可退出，谢谢.\n");
}

