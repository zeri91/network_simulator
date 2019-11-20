#ifndef BINARYHEAP_H
#define BINARYHEAP_H

#include <vector>

namespace NS_OCH {

template<class T,
    class Cont = vector<T>,
    class Pred = less<typename Cont::value_type> >
class BinaryHeap {
public:
//    typedef Cont::allocator_type allocator_type;
    typedef typename Cont::value_type value_type;
    typedef typename Cont::size_type size_type;

	explicit BinaryHeap(const Pred& pr = Pred());

    bool empty() const;
    size_type size() const;
	void insert(const value_type&);
	const value_type& peekMin();
	void popMin();
	void buildHeap();

public:
    Cont m_hContainer;
    Pred m_hPredicate;

private:
	void percolateDown(int);
};

};

using namespace NS_OCH;

template<class T, class Cont, class Pred>
BinaryHeap<T, Cont, Pred>::BinaryHeap(const Pred& hPred): m_hPredicate(hPred)
{
}

template<class T, class Cont, class Pred>
bool BinaryHeap<T, Cont, Pred>::empty() const
{
	return (m_hContainer.empty());
}

template<class T, class Cont, class Pred>
typename BinaryHeap<T, Cont, Pred>::size_type BinaryHeap<T, Cont, Pred>::size() const
{
	return m_hContainer.size();
}

template<class T, class Cont, class Pred>
void BinaryHeap<T, Cont, Pred>::insert(const value_type& hItem)
{
	// Code except from priority_queue
	m_hContainer.push_back(hItem);
	push_heap(m_hContainer.begin(), m_hContainer.end(), m_hPredicate);
	// Percolate up
//	m_hContainer.push_back(hItem);
//	int nHole = m_hContainer.size() - 1;
//	for(; (nHole>0) && m_hPredicate(hItem, m_hContainer[nHole/2]); nHole /= 2)
//		m_hContainer[nHole] = m_hContainer[nHole/2];
//	m_hContainer[nHole] = hItem;
}

template<class T, class Cont, class Pred>
const typename BinaryHeap<T, Cont, Pred>::value_type& BinaryHeap<T, Cont, Pred>::peekMin()
{
	return m_hContainer.front();
}

template<class T, class Cont, class Pred>
void BinaryHeap<T, Cont, Pred>::popMin()
{
	pop_heap(m_hContainer.begin(), m_hContainer.end(), m_hPredicate);
	m_hContainer.pop_back();
}

template<class T, class Cont, class Pred>
void BinaryHeap<T, Cont, Pred>::buildHeap()
{
	make_heap(m_hContainer.begin(), m_hContainer.end(), m_hPredicate);
//	int i;
//	for(i = (m_hContainer.size()-1)/2; i >= 0; i-- )
//		percolateDown( i );
}

template<class T, class Cont, class Pred>
void BinaryHeap<T, Cont, Pred>::percolateDown(int nHole)
{
	T hTemp = m_hContainer[nHole];
	size_type nSize = m_hContainer.size();
	size_type nChild;

	for(; nHole*2 < nSize; nHole=nChild)
    {
		nChild = nHole*2;
		if( (nChild != nSize) && 
			(m_hPredicate(m_hContainer[nChild+1], m_hContainer[nChild])))
			nChild++;
		if(m_hPredicate(m_hContainer[ nChild ], hTemp))
			m_hContainer[nHole] = m_hContainer[nChild];
		else
			break;
	}
	m_hContainer[nHole] = hTemp;
}
#endif
