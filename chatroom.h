/*
 头文件，函数声明和宏定义 
 */
 
#ifndef CHATROOM_H
#define CHATROOM_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<signal.h>
#include<time.h>	 //asctime() , time() /获取时间
#include<ctype.h>    //ispunct() /检测非法字符
 

#define QUEUELEN     100  			  // 用户数最大值
#define BUF_SIZE     256  
#define IP          "192.168.153.128" // 服务端ip
#define PORT         1234             // port
#define FILENAME     "userinfo"       // 存储用户数据文件名称（name:passwd:id:authority） 
#define CLIENT_ID    1000    		  // 用户id初始值

#define FILE_NAME_SIZE 	512
#define BUF_SIZE 	512
#define FILE_BUF_SIZE	512
//========================functions================================================
#define PUBLIC_CHAT        			1  //群聊
#define PRIVATE_CHAT       			2  //私聊
#define CLIENT_LOGIN       			3  //登陆验证
#define CLIENT_REGISTER    			4  //注册
#define CLIENT_ONLINE      			5  //所有的在线用户
#define CLIENT_EXIT        			6  //退出
#define SEND_FILE          			7  //文件传输

#define CHAT_FUN			1		//显示聊天消息
#define FILE_FUN			2		//文件传输
//====================================================================
#define NORMAL_USER_LOGIN_FAILED  			0 //普通用户登录失败
#define NORMAL_USER_LOGIN_SUCCESS 			1 //普通用户登录成功
#define NORMAL_USER_LOGIN_FAILED_ONLINE  	2 //普通用户已经在线
#define ADMIN_LOGIN_SUCCESS              	3 //管理员登录成功
#define NORMAL_USER_LOGIN_PASSWD_ERROR   	4 //普通用户登录密码错误

#define REGIST_FALIED   0   //注册失败
#define REGIST_EXITED   1   //注册的用户已经存在

#define NORMAL_USER     0   //普通用户
#define ADMIN_USER      1   //管理员

// ===========客户信息结构==========================================
typedef struct{
	pthread_t tid;					//线程的描述符
	int  sockfd;  					//accept的返回的客户端的新的套接字描述符
	char client_name[25]; 			//账号
	char client_passwd[25]; 		//密码
	int  client_id;					//用户ID
	int  is_online;					//在线状态 1 在线 0 不在线
	int  admin;              		//用户权限，1为管理员，0为普通用户
}client_info;
client_info clients[QUEUELEN];

//============客户端发送数据包结构=====================================
typedef struct send_info{
	int  type;					//类型
	char id[25];  				//对方id
	char buf[BUF_SIZE]; 		//内容
	char name[25];				//用户名（昵称）
	char passwd[25];			//密码
}send_info;

//============服务器发送数据包结构=====================================
typedef struct tran_info{
	int  type;					//类型
	char buf[BUF_SIZE]; 		//内容
}tran_info;

//====================debug============================

#define CHAT_DEBUG
#ifdef  CHAT_DEBUG
#define DEBUG(message...) fprintf(stderr, message)
#else
#define DEBUG(message...)
#endif

// ========fun=========client.c====================
void  	print_err(char *msg);
void  	reg_log();
void  	init_conn();
void  	send_fun();
void  	show_menu();
void  	signHandler(int signNo);
void	send_file();
void  	recv_file(tran_info  *tran);

//  ======fun=======server.c======================
int    	system_init();
void   	connect_to_client(int socketfd );
void   	err(char *err_msg);
int    	init_clents(char  *tok_file[]);
void   	register_new_client(send_info *send,int newfd);
void   	server_check_login(send_info *send,int newfd);
void   	client_exit(send_info *send,int newfd);
int    	get_sockfd(char dest[]);
void   	private_chat (send_info *send,int newfd);
void   	public_chat (send_info *send ,int newfd);
void   	get_all_online_clients (send_info *send ,int newfd);
void  	advanced_client_to_admin (send_info *send,int newfd);
void   	drop_client_to_normal (send_info *send,int newfd) ;
void   	tran_file(send_info *send,int newfd);
void 	parse_output_buf(char *src,  tran_info  *tran,int t);

#endif 
