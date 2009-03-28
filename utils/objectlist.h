#ifndef OBJECTLIST_H
#define OBJECTLIST_H

// STL
#include <list>

using namespace std;

template<class T>
class ObjectList : public list<T>
{

public:
	ObjectList() {}
	virtual ~ObjectList() { }
			
	virtual void Add(T item) { push_back(item); }		
	virtual void Sort() {}
	virtual void Remove(T& item) { remove((char*)item); }
	virtual void Remove(T* pitem) { remove((char*)pitem); }

};


#endif
