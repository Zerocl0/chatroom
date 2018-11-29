/*
 ͷ�ļ������������ͺ궨�� 
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
#include<time.h>	 //asctime() , time() /��ȡʱ��
#include<ctype.h>    //ispunct() /���Ƿ��ַ�
 

#define QUEUELEN     100  			  // �û������ֵ
#define BUF_SIZE     256  
#define IP          "192.168.153.128" // �����ip
#define PORT         1234             // port
#define FILENAME     "userinfo"       // �洢�û������ļ����ƣ�name:passwd:id:authority�� 
#define CLIENT_ID    1000    		  // �û�id��ʼֵ

#define FILE_NAME_SIZE 	512
#define BUF_SIZE 	512
#define FILE_BUF_SIZE	512
//========================functions================================================
#define PUBLIC_CHAT        			1  //Ⱥ��
#define PRIVATE_CHAT       			2  //˽��
#define CLIENT_LOGIN       			3  //��½��֤
#define CLIENT_REGISTER    			4  //ע��
#define CLIENT_ONLINE      			5  //���е������û�
#define CLIENT_EXIT        			6  //�˳�
#define SEND_FILE          			7  //�ļ�����

#define CHAT_FUN			1		//��ʾ������Ϣ
#define FILE_FUN			2		//�ļ�����
//====================================================================
#define NORMAL_USER_LOGIN_FAILED  			0 //��ͨ�û���¼ʧ��
#define NORMAL_USER_LOGIN_SUCCESS 			1 //��ͨ�û���¼�ɹ�
#define NORMAL_USER_LOGIN_FAILED_ONLINE  	2 //��ͨ�û��Ѿ�����
#define ADMIN_LOGIN_SUCCESS              	3 //����Ա��¼�ɹ�
#define NORMAL_USER_LOGIN_PASSWD_ERROR   	4 //��ͨ�û���¼�������

#define REGIST_FALIED   0   //ע��ʧ��
#define REGIST_EXITED   1   //ע����û��Ѿ�����

#define NORMAL_USER     0   //��ͨ�û�
#define ADMIN_USER      1   //����Ա

// ===========�ͻ���Ϣ�ṹ==========================================
typedef struct{
	pthread_t tid;					//�̵߳�������
	int  sockfd;  					//accept�ķ��صĿͻ��˵��µ��׽���������
	char client_name[25]; 			//�˺�
	char client_passwd[25]; 		//����
	int  client_id;					//�û�ID
	int  is_online;					//����״̬ 1 ���� 0 ������
	int  admin;              		//�û�Ȩ�ޣ�1Ϊ����Ա��0Ϊ��ͨ�û�
}client_info;
client_info clients[QUEUELEN];

//============�ͻ��˷������ݰ��ṹ=====================================
typedef struct send_info{
	int  type;					//����
	char id[25];  				//�Է�id
	char buf[BUF_SIZE]; 		//����
	char name[25];				//�û������ǳƣ�
	char passwd[25];			//����
}send_info;

//============�������������ݰ��ṹ=====================================
typedef struct tran_info{
	int  type;					//����
	char buf[BUF_SIZE]; 		//����
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
