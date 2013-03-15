#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <list>
#include <stdio.h>

template<typename T>
struct PQueueElement
{
	PQueueElement() : key(0) {}
	~PQueueElement() {}
	bool operator==(const PQueueElement& lhs)
	{
		return data==lhs.data;
	}
	T data;
	float key;
};

template<typename T>
class PriorityQueue	//for uniform-cost search
{
public:
	PriorityQueue() {}
	~PriorityQueue() {queue.clear();}

	T pop();
	void push(T value, int key);
	void clear();
	bool isEmpty() const;
	bool doesBelong(T value);
	void replaceExisting(T value, int key);
private:
	std::list<PQueueElement<T>> queue;
};


#endif //PRIORITYQUEUE_H