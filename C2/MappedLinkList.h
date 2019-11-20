#ifndef MAPPEDLINKLIST_H
#define MAPPEDLINKLIST_H

#include <list>
#include <map>

namespace NS_OCH {

class OchObject;
template <class Key, class T>
class MappedLinkList: public OchObject {
public:
	MappedLinkList();
	~MappedLinkList();

	virtual void dump(ostream &) const;

	void push_front(const Key&, const T&);	// inserted at the head
	void erase(const Key&);
	void remove(const T&);
	T find(const Key&) const;

	typename list<T>::iterator begin() {
		return m_hList.begin();
	}
	typename list<T>::const_iterator begin() const {
		return m_hList.begin();
	}
	typename list<T>::iterator end() {
		return m_hList.end();
	}
	typename list<T>::const_iterator end() const {
		return m_hList.end();
	}
	typename list<T>::size_type size() const {
		return m_hList.size();
	}
	void clear() {
		m_hList.clear(); 
		m_hMap.clear();
	}
public:
	list<T> m_hList;

	typedef pair<Key, typename list<T>::iterator> KeyIteratorPair;
	typedef map<Key, typename list<T>::iterator> KeyIteratorMap;
protected:
	KeyIteratorMap	m_hMap;

};

};

using namespace NS_OCH;

template<class Key, class T>
MappedLinkList<Key, T>::MappedLinkList()
{
}

template<class Key, class T>
MappedLinkList<Key, T>::~MappedLinkList()
{
}

template<class Key, class T>
void MappedLinkList<Key, T>::dump(ostream &out) const
{
	assert(false);
}

template<class Key, class T>
void MappedLinkList<Key, T>::push_front(const Key& hKey, const T& hValue)
{
	m_hList.push_front(hValue);
	m_hMap.insert(KeyIteratorPair(hKey, m_hList.begin()));
}

template<class Key, class T>
void MappedLinkList<Key, T>::erase(const Key& hKey)
{
	typename KeyIteratorMap::iterator itr = m_hMap.find(hKey);
	if (itr == m_hMap.end())
		return;
	m_hList.erase(itr->second);
	m_hMap.erase(itr);
}

//-B: added by me, but it doesn't work
/*
template<class Key, class T>
inline void NS_OCH::MappedLinkList<Key, T>::remove(const T & hValue)
{
	typename KeyIteratorMap::iterator itr;
	for (UINT hKey = 0; itr != m_hMap.end(); hKey++)
	{
		itr = m_hMap.find(hKey);
		if (*itr->second == hValue)
			break;
	}
	m_hList.erase(itr->second);
	m_hMap.erase(itr);
}
*/

template<class Key, class T>
T MappedLinkList<Key, T>::find(const Key& hKey) const
{
	typename KeyIteratorMap::const_iterator itr = m_hMap.find(hKey);
	if (itr == m_hMap.end())
		return (T)(NULL);	// NB: this may not work for non-pointer class T
	return (*(itr->second));
}
#endif
