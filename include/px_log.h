#ifndef PX_LOG
#define PX_LOG
#include <iostream>
using std::cout;
using std::endl;
//now it just cout
class px_log {
public:
	static px_log& get_log_instance() {
		static px_log px_log_instance;
		return px_log_instance;
	}
	template<typename T>
	void write(const T& val) { cout << val << endl; }
private:
	px_log() {}
};

#endif // !PX_LOG
