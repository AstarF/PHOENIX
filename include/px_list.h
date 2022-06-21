//预先分配对象池
#ifndef PX_LIST
#define PX_LIST

/*==Object Container==
* 用来容纳一个对象
* 提供一个void指针用于指向被容纳的对象
* 提供一个容器指针用于构成一个链表
*/
template <typename T>
struct objbox {
	T* pointer = nullptr;
	objbox<T>* next = nullptr;
};

/*==Object Pool==
* 对象池，用于预先为多个对象分配空间并将其初始化
* 内部使用了objbox将多个对象以链表的形式串联起来
* 维护了两个链表：
*	free链表用于保存已经初始化等待取出使用的对象
*	ready链表用于保存内部对象被取出的objbox，当配分配的对象归还时，将使用ready的objbox容纳该对象
* 经实验：Linux，G++编译器环境下与直接new和delete相比能够节省5%~10%的内存分配时间
* Windows VS2019 Release环境下则能节省50%以上的内存分配时间
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
* totalblocks：预先分配管理的对象数
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
* 析构函数会释放对象池中的所有对象
* 所有容器会被清除
* 但是尚未归还的对象不会被清除
*/
template <typename T>
px_objpool<T>::~px_objpool() {
	for (int i = 0; i < totalblocks; ++i) {
		delete ptr[i].pointer;
	}
	delete[] ptr;
}

/*==对象归还函数==
* 尽量不要归还不属于该对象池的对象
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

/*==对象归还函数==
* 尽量不要归还不属于该对象池的对象
* 该函数需要遍历链表，效率低下，非特殊情况不要使用
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

/*==对象借出函数==
* 记得归还借出的对象，与push_back成对使用
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

/*==获取所有可借出对象的数量==
*/
template <typename T>
int px_objpool<T>::getfreeblocks() { return freeblocks; }

/*==获取对象池的容量==
*/
template <typename T>
int px_objpool<T>::getotalblocks() { return totalblocks; }

/*==Object Queue==
* 对象池的变体，用于预先分配多个容纳对象的容器，并将它们以链表的形式组织起来
* 内部使用了objbox将多个对象以链表的形式串联起来
* 维护了两个链表：
*	active链表用于保存被放入等待取出使用的对象
*	ready链表用于保存空闲的objbox
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
	/*对象队列容量不够时，调用此函数进行扩容
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
* totalblocks：预先分配管理的对象数
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
* 所有容器会被清除
*/
template <typename T>
px_objqueue<T>::~px_objqueue() {
	delete[] ptr;
}

/*==将对象存入对象队列首部==
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

/*==将对象存入对象队列尾部==
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

/*==从对象队列中取出对象==
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

/*==获取存入对象的数量==
*/
template <typename T>
int px_objqueue<T>::getcurrnetblocks() { return currnetblocks; }

/*==获取对象队列的容量==
*/
template <typename T>
int px_objqueue<T>::getotalblocks() { return totalblocks; }

#endif // !PX_LIST
