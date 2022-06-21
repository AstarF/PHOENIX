#ifndef PX_MYSQL_CONNECT
#define PX_MYSQL_CONNECT
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <iostream>
using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::to_string;
#include "px_log.h"
/*==sqlres==
* ����sql��ѯ���ؽ��������
* ��Ҫ�ṩһ������������ÿһ�������ʵ��
* ��ʵ����Ҫ����const char data[]�������ɽ�����json����
* ����Ҫ��parse����MYSQL_ROW
*/
template<typename T>
class sqlres {
public:
	sqlres() :total(0) {}
    void parse(const MYSQL_ROW& row);
    int total;
    vector<T> list;
};

template<typename T>
void sqlres<T>::parse(const MYSQL_ROW& row) {
    T t;
    t.parse(row);
    list.push_back(t);
    total++;
}

/*==px_mysql_connect==
* mysql���ݿ�����
* ����������һ�����ݿ������
* ͨ��init�������ݿ�
* �������������жϣ���������ϻ���жϿ�
* ͨ��query��������sql��mysql
*/
class px_mysql_connect {
public:
	px_mysql_connect();
	~px_mysql_connect();
	void init(const char* server, const char* username, const char* password, const char* database, int port = 3306);
	void reinit();
	bool execute_sql(const char* querybuffer);
    template<typename T>
	bool execute_sql(const char* querybuffer, sqlres<T>& res);
private:
	MYSQL* mysqlptr;
	string server_name;
	string username;
	string password;
	string database;
	int port;
};

template<typename T>
bool px_mysql_connect::execute_sql(const char* querybuffer, sqlres<T>& queryres) {
	//��ѯ����
	if (mysqlptr == nullptr) {
		printf("nullptr!!\n");
		return false;
	}
	int ret = mysql_query(mysqlptr, querybuffer);
	if (ret) {
		printf("mysql_query failed (%d).reason: ", mysql_errno(mysqlptr));
		px_log::get_log_instance().write(mysql_error(mysqlptr));
		//��������ݿ����ӳ�ʱ��,���ؽ�����
		if (mysql_errno(mysqlptr) == 2006 || mysql_errno(mysqlptr) == 2013) {
			this->reinit();
			int ret = mysql_query(mysqlptr, querybuffer);
			if (ret) {
				px_log::get_log_instance().write("retry mysql_query failed. reason:");
				px_log::get_log_instance().write(mysql_error(mysqlptr));
				return  false;
			}
		}else return false;
	}
	MYSQL_RES* res = mysql_store_result(mysqlptr);
	MYSQL_ROW row;
	//����
	while (row = mysql_fetch_row(res))
	{	
		queryres.parse(row);
	}
	//�ͷŽ����
	mysql_free_result(res);
	return true;
}
#endif // !PX_MYSQL_CONNECT
