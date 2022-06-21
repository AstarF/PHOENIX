//Ԥ�ȷ�������
#ifndef PX_LIST
#define PX_LIST

/*==Object Container==
* ��������һ������
* �ṩһ��voidָ������ָ�����ɵĶ���
* �ṩһ������ָ�����ڹ���һ������
*/
template <typename T>
struct objbox {
	T* pointer = nullptr;
	objbox<T>* next = nullptr;
};

/*==Object Pool==
* ����أ�����Ԥ��Ϊ����������ռ䲢�����ʼ��
* �ڲ�ʹ����objbox������������������ʽ��������
* ά������������
*	free�������ڱ����Ѿ���ʼ���ȴ�ȡ��ʹ�õĶ���
*	ready�������ڱ����ڲ�����ȡ����objbox���������Ķ���黹ʱ����ʹ��ready��objbox���ɸö���
* ��ʵ�飺Linux��G++��������������ֱ��new��delete����ܹ���ʡ5%~10%���ڴ����ʱ��
* Windows VS2019 Release���������ܽ�ʡ50%���ϵ��ڴ����ʱ��
*/
template <typename T>
class px_objpool {
public:
	px_objpool(int totalblocks = 500);
	~px_objpool();
	void push_front(T* node);
	void push_back(T* node);
	T* pop_front();
	int getfreeblocks();
	int getotalblocks();
private:
	objbox<T> *active;
	objbox<T> *ready;
	objbox<T> *ptr;
	int totalblocks;
	int freeblocks;
};

/*==Constructor==
* totalblocks��Ԥ�ȷ������Ķ�����
*/
template <typename T>
px_objpool<T>::px_objpool(int totalblocks) :totalblocks(totalblocks), freeblocks(totalblocks), active(nullptr), ready(nullptr) {
	ptr = new objbox<T>[totalblocks];
	active = ptr;
	for (int i = 0; i < totalblocks - 1; ++i) {
		ptr[i].pointer = new T;
		ptr[i].next = &(ptr[i + 1]);
	}
	ptr[totalblocks - 1].pointer = new T;
	ptr[totalblocks - 1].next = nullptr;
}

/*==Destructor==
* �����������ͷŶ�����е����ж���
* ���������ᱻ���
* ������δ�黹�Ķ��󲻻ᱻ���
*/
template <typename T>
px_objpool<T>::~px_objpool() {
	for (int i = 0; i < totalblocks; ++i) {
		delete ptr[i].pointer;
	}
	delete[] ptr;
}

/*==����黹����==
* ������Ҫ�黹�����ڸö���صĶ���
*/
template <typename T>
void px_objpool<T>::push_front(T* node) {
	if (node && ready) {
		objbox<T>* t = ready;
		ready = ready->next;
		t->pointer = node;
		t->next = active;
		active = t;
		freeblocks++;
	}
}

/*==����黹����==
* ������Ҫ�黹�����ڸö���صĶ���
* �ú�����Ҫ��������Ч�ʵ��£������������Ҫʹ��
*/
template <typename T>
void px_objpool<T>::push_back(T* node) {
	if (node && ready) {
		objbox<T>* t = ready;
		ready = ready->next;
		t->pointer = node;
		t->next = nullptr;
		objbox<T>* tail = active;
		if (active) {
			while (tail->next)tail = tail->next;
			tail->next = t;
		}
		else {
			active = t;
		}
		freeblocks++;
	}
}

/*==����������==
* �ǵù黹����Ķ�����push_back�ɶ�ʹ��
*/
template <typename T>
T* px_objpool<T>::pop_front() {
	if (active) {
		objbox<T>* t = active;
		active = active->next;
		T* res = t->pointer;
		t->pointer = nullptr;
		t->next = ready;
		ready = t;
		freeblocks--;
		return res;
	}
	return nullptr;
}

/*==��ȡ���пɽ�����������==
*/
template <typename T>
int px_objpool<T>::getfreeblocks() { return freeblocks; }

/*==��ȡ����ص�����==
*/
template <typename T>
int px_objpool<T>::getotalblocks() { return totalblocks; }

/*==Object Queue==
* ����صı��壬����Ԥ�ȷ��������ɶ���������������������������ʽ��֯����
* �ڲ�ʹ����objbox������������������ʽ��������
* ά������������
*	active�������ڱ��汻����ȴ�ȡ��ʹ�õĶ���
*	ready�������ڱ�����е�objbox
*/
template <typename T>
class px_objqueue {
public:
	px_objqueue(int totalblocks = 100);
	~px_objqueue();
	void push_back(T* node);
	void push_front(T* node);
	T* pop_front();
	int getcurrnetblocks();
	int getotalblocks();
private:
	/*���������������ʱ�����ô˺�����������
	*/
	void addblocks(int num) {
		for (int i = 0; i < num; ++i) {
			objbox<T>* t = new objbox<T>;
			t->next = ready;
			ready = t;
		}
	}
	objbox<T>* active;
	objbox<T>* active_tail;
	objbox<T>* ready;
	objbox<T>* ptr;
	int totalblocks;
	int currnetblocks;
};

/*==Constructor==
* totalblocks��Ԥ�ȷ������Ķ�����
*/
template <typename T>
px_objqueue<T>::px_objqueue(int totalblocks) :totalblocks(totalblocks), currnetblocks(0), active(nullptr), active_tail(nullptr), ready(nullptr) {
	ptr = new objbox<T>[totalblocks];
	ready = ptr;
	for (int i = 0; i < totalblocks - 1; ++i) {
		ptr[i].pointer = nullptr;
		ptr[i].next = &(ptr[i + 1]);
	}
	ptr[totalblocks - 1].next = nullptr;
}

/*==Destructor==
* ���������ᱻ���
*/
template <typename T>
px_objqueue<T>::~px_objqueue() {
	delete[] ptr;
}

/*==����������������ײ�==
*/
template <typename T>
void px_objqueue<T>::push_front(T* node) {
	if (node) {
		if (ready == nullptr)addblocks(totalblocks / 2);
		objbox<T>* t = ready;
		ready = ready->next;
		t->pointer = node;
		t->next = active;
		active = t;
		if (active_tail == nullptr)active_tail = active;
		currnetblocks++;
	}
}

/*==���������������β��==
*/
template <typename T>
void px_objqueue<T>::push_back(T* node) {
	if (node) {
		if (ready == nullptr)addblocks(totalblocks / 2);
		objbox<T>* t = ready;
		ready = ready->next;
		t->pointer = node;
		t->next = nullptr;
		if (active_tail) {
			active_tail->next = t;
			active_tail = t;
		}
		else active = active_tail = t;
		currnetblocks++;
	}
}

/*==�Ӷ��������ȡ������==
*/
template <typename T>
T* px_objqueue<T>::pop_front() {
	if (active) {
		objbox<T>* t = active;
		active = active->next;
		if (active == nullptr)active_tail = nullptr;
		T* res = t->pointer;
		t->pointer = nullptr;
		t->next = ready;
		ready = t;
		currnetblocks--;
		return res;
	}
	return nullptr;
}

/*==��ȡ������������==
*/
template <typename T>
int px_objqueue<T>::getcurrnetblocks() { return currnetblocks; }

/*==��ȡ������е�����==
*/
template <typename T>
int px_objqueue<T>::getotalblocks() { return totalblocks; }

#endif // !PX_LIST
