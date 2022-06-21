#include "px_mysql.h"
/*Constructor
*/
px_mysql_connect::px_mysql_connect() :mysqlptr(nullptr) {}

/*初始化数据库
* server为服务器
* username为用户名
* password为密码
* database为数据库名
* port为端口
*/
void px_mysql_connect::init(const char* server, const char* username, const char* password, const char* database, int port) {
	//初始化变量
	this->server_name = server;
	this->username = username;
	this->password = password;
	this->database = database;
	this->port = port;
	//初始化数据库
	mysqlptr = new MYSQL;
	mysql_init(mysqlptr);
	//设置字符编码
	mysql_options(mysqlptr, MYSQL_SET_CHARSET_NAME, "gbk");
	//连接数据库
	if (mysql_real_connect(mysqlptr, server, username, password, database, port, NULL, 0) == NULL)
		//localhost为服务器，root为用户名和密码，school为数据库名，3306为端口
	{
		px_log::get_log_instance().write("mysql_real_connect failed. reason:");
		px_log::get_log_instance().write(mysql_error(mysqlptr));
	}
}

void px_mysql_connect::reinit() {
	//清理旧连接
	if (mysqlptr) {
		mysql_close(mysqlptr);
		delete mysqlptr;
	}
	//初始化数据库
	mysqlptr = new MYSQL;
	mysql_init(mysqlptr);
	//设置字符编码
	mysql_options(mysqlptr, MYSQL_SET_CHARSET_NAME, "gbk");
	//连接数据库
	if (mysql_real_connect(mysqlptr, server_name.c_str(), username.c_str(), password.c_str(), database.c_str(), port, NULL, 0) == NULL)
		//localhost为服务器，root为用户名和密码，school为数据库名，3306为端口
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

/*SQL语句执行
* querybuffer: SQL语句
* data:查询容纳返回值,默认为空，即没有返回值
*/
bool px_mysql_connect::execute_sql(const char* querybuffer) {
	//查询数据
	if (mysqlptr == nullptr) {
		printf("nullptr!!\n");
		return false;
	}
	int ret = mysql_query(mysqlptr, querybuffer);
	if (ret) {
		printf("mysql_query failed (%d).reason: \n", mysql_errno(mysqlptr));
		px_log::get_log_instance().write(mysql_error(mysqlptr));
		//如果是数据库连接超时了,则重建连接
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

