#include "chatroom.h"
/*
   server.c
   server   服务器
 */
//函数功能结构体
typedef struct {
     int  fun_flag; //function flag
     void (*fun)(); // function pointer variable
}proto;

proto p[]={
	{PUBLIC_CHAT,      	public_chat}, 					 	//群聊
	{PRIVATE_CHAT,     	private_chat},					 	//私聊
	{CLIENT_LOGIN,     	server_check_login},		 		//登陆验证
	{CLIENT_REGISTER,  	register_new_client},	        	//注册
	{CLIENT_ONLINE,    	get_all_online_clients},			//获取所有的在线用户
	{CLIENT_EXIT,      	client_exit},					 	//客户退出
	{SEND_FILE,		  	tran_file},							//文件传输 
	{0,0}
};

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;

static int id=CLIENT_ID; //init  client id

int main(int argc ,char *argv[])
{
	int socketfd,newfd;
	socketfd=system_init();
	connect_to_client(socketfd);
	return 0;
}

// 新线程接受客户端数据
void *pthread_fun(void *arg)
{
	int newfd=*((int *)arg);
	char buf[BUF_SIZE]={0};
	int length=0;
	send_info   send,*p;
	p=&send;
 	while(1)
	{
			length=read(newfd,&send,sizeof(send));
			if(length<=0)
			{
				printf("server receive msg is empty !\n");
				break;	
			}
			parse_buf(p, newfd);
			memset(&send,0,sizeof(send));
	}
    client_exit(p, newfd);
}
//解析客户端传过来的字符串数据结构中的type 成员来匹配调用不同的函数。
void parse_buf(send_info *send,int newfd)
{  
DEBUG("send->type=%d\n",send->type);
	int i=0;
    for(i=0;p[i].fun_flag!=0;i++)
    {
        if(send->type==p[i].fun_flag)
        {
            p[i].fun(send,newfd);	
            break; 
        }
     }   
}


//初始化连接
int system_init()
{
	int socketfd;
	// /usr/include/linux/in.h +186
	struct sockaddr_in  server_addr ;
	if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1)
		err("server socket()  err"); 
	//	memset(&addr ,0,sizeof(struct sockaddr_in));
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	inet_pton(AF_INET,IP,&server_addr.sin_addr);
	// set options on sockets 
	int opt,optlen=sizeof(optlen);
	// 设置端口重用
	if( (setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&opt,optlen))==-1)
		err(" server setsockopt()  error ");
	if(-1==(bind(socketfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))))
		err(" server bind()   error"); 

	if( (listen(socketfd,QUEUELEN))==-1)
		err("server listen()   error"); 
	fprintf(stderr,"waiting client connection....\n");
	// return 新的socket 描述符 
	return socketfd ;
 }

//等待客户端连接
void  connect_to_client(int socketfd)
{
	int newfd;
	struct sockaddr_in   acc_addr;
	socklen_t  len=sizeof(struct sockaddr_in);
	int i=0;
	while(1)
	{
		signal(SIGINT, signHandler); // server ctrl +c   ,必要时可以用ctrl + \来关闭服务器
		signal(SIGPIPE,SIG_IGN);// if client  read  write close  server write 接收到 SIGPIPE
		if((newfd=accept(socketfd,(struct sockaddr *)&acc_addr,&len))==-1)
			err("server accept()   error");  
		printf("server :  welcome   %s connect.\n",inet_ntoa(acc_addr.sin_addr));	
		           
		for(i=0;i<QUEUELEN;i++)
		{
			if(clients[i].sockfd==0)
			{
				clients[i].sockfd=newfd;
				DEBUG("clients[i]sockfd=%d\n",clients[i].sockfd);
				break;
			}
		} 
			
		if((pthread_create(&clients[i].tid,NULL,pthread_fun,&clients[i].sockfd))==-1)
			err("server pthread_create() 2  error"); 
	}
	close(socketfd);
	close(newfd);
}

//1群聊
void public_chat (send_info *send ,int newfd)
{
    int j=0;
    char str[BUF_SIZE]={'\0'};
	tran_info tran;
    for(j=0;j<QUEUELEN;j++)
		{
			if(clients[j].sockfd==0 || clients[j].sockfd==newfd) 
				continue;
			else
			{
			  	format_buf (str,send->buf,newfd);
				parse_output_buf(str,&tran,CHAT_FUN);
				write(clients[j].sockfd,&tran,sizeof(tran));
			}
		}
}

//2私聊
void private_chat (send_info *send,int newfd)
{
	char str[BUF_SIZE]={'\0'};
	int tag_id=0;
	tran_info tran;
	if((tag_id=get_sockfd(send->id))!=0)
    {
 	    format_buf(str,send->buf,newfd);
		parse_output_buf(str,&tran,CHAT_FUN);
 		write(tag_id,&tran,sizeof(tran));  
 	}
 	else
 		DEBUG("没有此用户，请核对!\n");
}

//3登陆
void server_check_login(send_info *send,int newfd)
{
	pthread_mutex_lock(&mutex);
	//================normal users login read info from file to check========================   
	int fd; char buf[BUF_SIZE]={0};
	if((fd=open(FILENAME,O_RDONLY,0644))==-1)
		err("server_check_login open file  failed!");
	int len=0;
	int ret=0;
	char  *tok_file[5];
	int j=0;
	while(1)
	{
		lseek(fd,len,SEEK_SET);
		if(read(fd,buf,BUF_SIZE)==0) break;
		len+=strlen(buf)+2;
		j=0;
		tok_file[j++]=strtok(buf,":"); 
		while(tok_file[j++]=strtok(NULL,":"));
		if( strcmp(tok_file[0],send ->name) ==0 )
		{
			if( strcmp(tok_file[1],send ->passwd)==0)
			{
				bzero(&ret,sizeof(ret));
				pthread_mutex_unlock(&mutex);
				if(init_clents(tok_file)) //  online ret 2  
				ret = NORMAL_USER_LOGIN_FAILED_ONLINE ;//2 已经在线
				else
				{  
					if(atoi(tok_file[3])==1) 
					ret= ADMIN_LOGIN_SUCCESS;// 3
					else 
					ret= NORMAL_USER_LOGIN_SUCCESS;//1   登录成功
				}
				break;
		    }
		    else //密码错误
		     {	 
		        bzero(&ret,sizeof(ret));
		        ret=NORMAL_USER_LOGIN_PASSWD_ERROR ;//4  passwd error
		        break;
		     }
		}else{//没有此账号
			bzero(&ret,sizeof(ret));
			ret= NORMAL_USER_LOGIN_FAILED ;//0
		}
		memset(buf,0,sizeof(buf));
	}
	write(newfd,&ret,sizeof(ret)); // 1 success 0 failed  2 online  4  passwd error
	pthread_mutex_unlock(&mutex);
	close(fd);
}

//4注册
void  register_new_client(send_info *send,int newfd)
{
	pthread_mutex_lock(&mutex);
	int    ret=0;
	char   all_buf[BUF_SIZE];
	char   read_buf[BUF_SIZE];
	int    fd, n,m;
	if(( fd=open(FILENAME,O_CREAT|O_RDWR|O_APPEND,0644))==-1)
		err("open file  failed or create file failed!\n");
	int len=0;
	int is_account_exited_flag=0;
	int j=0;
	
	char  *tok_file[5];
	while(1)
	{
		lseek(fd,len,SEEK_SET);
		if(read(fd,read_buf,BUF_SIZE)==0)
		{ 
			id++;
			break;
		}
		else
		{ 
			j=0;
			char str[BUF_SIZE]={0};
			strcpy(str,read_buf); 
			tok_file[j++]=strtok(read_buf,":");
			while(tok_file[j++]=strtok(NULL,":")); 
			if(strcmp(tok_file[0],send->name)==0)
			{
            is_account_exited_flag=REGIST_EXITED;
            break;
			}
 			id=atoi(tok_file[2]);   
			len+=strlen(str)+2;
			bzero(str,sizeof(str));
		}

	} 
	if(is_account_exited_flag==REGIST_EXITED)
	{
		ret=REGIST_EXITED;//1  exited
		write(newfd,&ret,sizeof(int));
	}
	else
	{		
		sprintf(all_buf,"%s:%s:%d:%d",send->name,send->passwd,id,NORMAL_USER);
		n=write(fd,all_buf,strlen(all_buf)+1);
		m=write(fd,"\n",1);// write a enter  write  length=2
		if(m!=0 && n!=0)
		{
			ret=id;
			write(newfd,&ret,sizeof(int));
		}
		else   
		{
			ret=REGIST_FALIED;//0  falied
		  	write(newfd,&ret,sizeof(int));
		} 
	}
	pthread_mutex_unlock(&mutex);
	close(fd);
}

//5查询在线用户
void get_all_online_clients (send_info *send,int newfd)
{
	int i=0;
	char buf[BUF_SIZE]={'\0'};
	char str[BUF_SIZE]={'\0'};
	char no_client_online[40]={"没有其他用户在线...."};
	tran_info tran;
   	for(i=0;i<QUEUELEN;i++) 
   	{
   		//不包含自己
        if( (clients[i].is_online==1) && (clients[i].sockfd!=newfd) )
        {
			sprintf(buf,"%s(id:%d)\t",clients[i].client_name,clients[i].client_id);
			strcat(str,buf);
        }
    }
	parse_output_buf(str,&tran,CHAT_FUN);
    if(strcmp(str,"")==0){					//没有用户在线
		parse_output_buf(no_client_online,&tran,CHAT_FUN);
		write(newfd,&tran,sizeof(tran));  
	}
    else 
		write(newfd,&tran,sizeof(tran));  
}

//6客户端退出
void  client_exit(send_info *send,int exit_client_socket_fd)
{
	   int i=0;
		for(i=0;i<QUEUELEN;i++)
		{
			if(clients[i].sockfd==exit_client_socket_fd)
			{ 
				break;
			}
		}
		//把该用户的有关信息置为空
		fprintf(stderr,"client name:\t%s\tid:\t%d\t exited!\n",clients[i].client_name,clients[i].client_id);
		memset(&clients[i],0,sizeof(clients[i]));
		close(exit_client_socket_fd);
}

//7服务端文件传输
void tran_file(send_info *send,int newfd)
{	
	DEBUG("tran file\n");
	char dest[BUF_SIZE]={'\0'};
	int tag_id=0;
	tran_info tran;
	if((tag_id=get_sockfd(send->id))!=0)
    {
		parse_output_buf(send->buf,&tran,FILE_FUN);
		DEBUG("tran.buf=%s\n",tran.buf);
 		write(tag_id,&tran,sizeof(tran)); 
 	}
 	else
 		DEBUG("没有此用户，请核对!\n");
}

//判断该用户是否在线
int check_online(char  *tok_file[]) // check client is online  true return 1 or return 0
{ 
//DEBUG("   check_online     tok_file[2]=%s\n",tok_file[2]);
	int i=0;
	for(i=0;i<QUEUELEN;i++)
	{
	if( ( clients[i].client_id==atoi(tok_file[2])) && clients[i].is_online==1  )
			return 1;//online 
	}
	//检查完所有在线用户后,则说明该用户没有在线  return 0!!!!!!!!!!!!! 
	 return 0;  // no online 
} 

//初始化用户结构数组
int init_clents(char  *tok_file[])
{	
	pthread_mutex_lock(&mutex);
	int i=0;
	if(check_online(tok_file))  // if online return 1 or init and return 0
		return 1;
	for(i=0;i<QUEUELEN;i++)
	{
		if(clients[i].client_id==0 && clients[i].is_online!=1)
		{//DEBUG("init_clents     =====%s\n",tok_file[2]);
			strcpy( clients[i].client_name , tok_file[0]);
			strcpy( clients[i].client_passwd,tok_file[1]);
			clients[i].client_id=atoi(tok_file[2]);
			clients[i].is_online=1; 
			if(atoi(tok_file[3])==1)  
				clients[i].admin=ADMIN_USER;//1
			else
				clients[i].admin=NORMAL_USER; //0
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	return  0;
}

//获取用户的sockfd(即accept的返回值)
int get_sockfd(char dest[])
{  
	int i=0;
	for(i=0;i<QUEUELEN;i++)
	{
		if(clients[i].client_id==atoi(dest) || strcmp(clients[i].client_name,dest)==0) 
		{
			return  clients[i].sockfd;
		}               
	}
	return 0;
}

//格式化字符串 (加上时间，用户昵称和ID等)
void format_buf(char *dest,char *content,int newfd)
{  
	int i=0;
    time_t  timep;
    for(;i<QUEUELEN;i++)
    { 
      	if(clients[i].sockfd ==newfd)break;
    }
    time(&timep);
    // DEBUG("%s\n",asctime(gmtime(&timep)));
    sprintf(dest,"%s(%d)\t%s  %s",clients[i].client_name,clients[i].client_id,ctime(&timep),content);  
}
//打包字符串
void parse_output_buf(char *src,  tran_info  *tran,int t)
{
	tran->type=t;
	strcpy(tran->buf,src);
}

//报错函数
void  err(char *err_msg)
{
	perror(err_msg);
	exit(EXIT_FAILURE);
}  
 
//忽略ctrl +c 键的处理函数
void signHandler(int signNo)
{
    //DEBUG("singal:%d \n",signNo);
    printf("若要真关闭服务器，请按CRTL+\\即可关闭。\n");
}



