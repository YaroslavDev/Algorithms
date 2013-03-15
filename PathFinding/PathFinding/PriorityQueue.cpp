#include "PriorityQueue.h"

template<typename T>
T PriorityQueue<T>::pop()
{
	PQueueElement<T> top = queue.front();
	queue.pop_front();
	return top.data;
}

template<typename T>
void PriorityQueue<T>::push(T value, int key)
{
	PQueueElement<T> toPush;
	toPush.data = value;
	toPush.key = key;
	bool wasPut = false;
	std::list<PQueueElement<T>>::iterator i = queue.begin(), end = queue.end();
	if( queue.empty() ) queue.push_back(toPush);
	else
		if( queue.size()==1 )
		{
			if( i->key < toPush.key )
				queue.push_back(toPush);
			else
				queue.push_front(toPush);
		}
		else
		{
			while( i!=end )			//find correspondent place for new value
			{
				if( toPush.key < i->key )
				{
					queue.insert(i, toPush);
					wasPut = true;
					break;
				}
				i++;
			}
			if( !wasPut )
				queue.push_back(toPush);
		}
}

template<typename T>
void PriorityQueue<T>::clear()
{
	queue.clear();
}

template<typename T>
bool PriorityQueue<T>::isEmpty() const
{
	return queue.empty();
}

template<typename T>
bool PriorityQueue<T>::doesBelong(T value) 
{
	bool flag = false;
	std::list<PQueueElement<T>>::iterator i = queue.begin(), end = queue.end();
	while( i!=end )
	{
		if( i->data == value )
		{
			flag = true;
			break;
		}
		i++;
	}
	return flag;
}

template<typename T>
void PriorityQueue<T>::replaceExisting(T value, int key) 
{
	bool flag = false;
	std::list<PQueueElement<T>>::iterator i = queue.begin(), end = queue.end();
	
	while( i!=end )
	{
		if( i->data == value )
			if( i->key > key )
			{
				i->data = value;
				break;
			}
		i++;
	}
}