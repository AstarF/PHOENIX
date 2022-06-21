#include "px_mysql.h"
/*Constructor
*/
px_mysql_connect::px_mysql_connect() :mysqlptr(nullptr) {}

/*��ʼ�����ݿ�
* serverΪ������
* usernameΪ�û���
* passwordΪ����
* databaseΪ���ݿ���
* portΪ�˿�
*/
void px_mysql_connect::init(const char* server, const char* username, const char* password, const char* database, int port) {
	//��ʼ������
	this->server_name = server;
	this->username = username;
	this->password = password;
	this->database = database;
	this->port = port;
	//��ʼ�����ݿ�
	mysqlptr = new MYSQL;
	mysql_init(mysqlptr);
	//�����ַ�����
	mysql_options(mysqlptr, MYSQL_SET_CHARSET_NAME, "gbk");
	//�������ݿ�
	if (mysql_real_connect(mysqlptr, server, username, password, database, port, NULL, 0) == NULL)
		//localhostΪ��������rootΪ�û��������룬schoolΪ���ݿ�����3306Ϊ�˿�
	{
		px_log::get_log_instance().write("mysql_real_connect failed. reason:");
		px_log::get_log_instance().write(mysql_error(mysqlptr));
	}
}

void px_mysql_connect::reinit() {
	//���������
	if (mysqlptr) {
		mysql_close(mysqlptr);
		delete mysqlptr;
	}
	//��ʼ�����ݿ�
	mysqlptr = new MYSQL;
	mysql_init(mysqlptr);
	//�����ַ�����
	mysql_options(mysqlptr, MYSQL_SET_CHARSET_NAME, "gbk");
	//�������ݿ�
	if (mysql_real_connect(mysqlptr, server_name.c_str(), username.c_str(), password.c_str(), database.c_str(), port, NULL, 0) == NULL)
		//localhostΪ��������rootΪ�û��������룬schoolΪ���ݿ�����3306Ϊ�˿�
	{
		px_log::get_log_instance().write("mysql_real_connect failed. reason:");
		px_log::get_log_instance().write(mysql_error(mysqlptr));
	}
}

/*Destructor
*/
px_mysql_connect::~px_mysql_connect() {
	if (mysqlptr) {
		mysql_close(mysqlptr);
		delete mysqlptr;
	}
}

/*SQL���ִ��
* querybuffer: SQL���
* data:��ѯ���ɷ���ֵ,Ĭ��Ϊ�գ���û�з���ֵ
*/
bool px_mysql_connect::execute_sql(const char* querybuffer) {
	//��ѯ����
	if (mysqlptr == nullptr) {
		printf("nullptr!!\n");
		return false;
	}
	int ret = mysql_query(mysqlptr, querybuffer);
	if (ret) {
		printf("mysql_query failed (%d).reason: \n", mysql_errno(mysqlptr));
		px_log::get_log_instance().write(mysql_error(mysqlptr));
		//��������ݿ����ӳ�ʱ��,���ؽ�����
		if (mysql_errno(mysqlptr)== 2006 || mysql_errno(mysqlptr) == 2013) {
			this->reinit();
			int ret = mysql_query(mysqlptr, querybuffer);
			if (ret) {
				px_log::get_log_instance().write("retry mysql_query failed. reason:");
				px_log::get_log_instance().write(mysql_error(mysqlptr));
				return  false;
			}
			return true;
		}
		return false;
	}
	return true;
}

