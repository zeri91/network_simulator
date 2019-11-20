// Program to test slices and a simple N*M matrix class

// pp 670-674 and 683-684

// No guarantees offered. Constructive comments to bs@research.att.com

#ifndef MATRIX
#define MATRIX
#include<iostream>
#include<valarray>
#include<algorithm>
#include<numeric>	// for inner_product

using namespace std;

// forward declarations to allow friend declarations:
template<class T> class Slice_iter;
template<class T> bool operator==(const Slice_iter<T>&, const Slice_iter<T>&);
template<class T> bool operator!=(const Slice_iter<T>&, const Slice_iter<T>&);
template<class T> bool operator< (const Slice_iter<T>&, const Slice_iter<T>&);

template<class T> class Slice_iter {
public:
	valarray<T>* v;
	slice s;
	size_t curr;	// index of current element

	T& ref(size_t i) const { return (*v)[s.start()+i*s.stride()]; }
public:
	Slice_iter(valarray<T>* vv, slice ss) :v(vv), s(ss), curr(0) { }

	Slice_iter end() const
	{
		Slice_iter t = *this;
		t.curr = s.size();	// index of last-plus-one element
		return t;
	}

	Slice_iter& operator++() { curr++; return *this; }
	Slice_iter operator++(int) { Slice_iter t = *this; curr++; return t; }

	T& operator[](size_t i) { return ref(i); }		// C style subscript
	T& operator()(size_t i) { return ref(i); }		// Fortran-style subscript
	T& operator*() { return ref(curr); }			// current element

	friend bool operator==(const Slice_iter& p, const Slice_iter& q);
	friend bool operator!=(const Slice_iter& p, const Slice_iter& q);
	friend bool operator< (const Slice_iter& p, const Slice_iter& q);

};


template<class T>
bool operator==(const Slice_iter<T>& p, const Slice_iter<T>& q)
{
	return p.curr==q.curr
		&& p.s.stride()==q.s.stride()
		&& p.s.start()==q.s.start();
}

template<class T>
bool operator!=(const Slice_iter<T>& p, const Slice_iter<T>& q)
{
	return !(p==q);
}

template<class T>
bool operator<(const Slice_iter<T>& p, const Slice_iter<T>& q)
{
	return p.curr<q.curr
		&& p.s.stride()==q.s.stride()
		&& p.s.start()==q.s.start();
}


//-------------------------------------------------------------



// forward declarations to allow friend declarations:
template<class T> class Cslice_iter;
template<class T> bool operator==(const Cslice_iter<T>&, const Cslice_iter<T>&);
template<class T> bool operator!=(const Cslice_iter<T>&, const Cslice_iter<T>&);
template<class T> bool operator< (const Cslice_iter<T>&, const Cslice_iter<T>&);


template<class T> class Cslice_iter 
{
	public:
	valarray<T>* v;
	slice s;
	size_t curr; // index of current element
	const T& ref(size_t i) const { return (*v)[s.start()+i*s.stride()]; }
public:
	Cslice_iter(valarray<T>* vv, slice ss): v(vv), s(ss), curr(0){}
	Cslice_iter end() const
	{
		Cslice_iter t = *this;
		t.curr = s.size(); // index of one plus last element
		return t;
	}
	Cslice_iter& operator++() { curr++; return *this; }
	Cslice_iter operator++(int) { Cslice_iter t = *this; curr++; return t; }
	
	const T& operator[](size_t i) const { return ref(i); }
	const T& operator()(size_t i) const { return ref(i); }
	const T& operator*() const { return ref(curr); }

	friend bool operator==(const Cslice_iter& p, const Cslice_iter& q);
	friend bool operator!=(const Cslice_iter& p, const Cslice_iter& q);
	friend bool operator< (const Cslice_iter& p, const Cslice_iter& q);

};

template<class T>
bool operator==(const Cslice_iter<T>& p, const Cslice_iter<T>& q)
{
	return p.curr==q.curr
		&& p.s.stride()==q.s.stride()
		&& p.s.start()==q.s.start();
}

template<class T>
bool operator!=(const Cslice_iter<T>& p, const Cslice_iter<T>& q)
{
	return !(p==q);
}

template<class T>
bool operator<(const Cslice_iter<T>& p, const Cslice_iter<T>& q)
{
	return p.curr<q.curr
		&& p.s.stride()==q.s.stride()
		&& p.s.start()==q.s.start();
}


//-------------------------------------------------------------


class Matrix {
	valarray<double>* v;	// stores elements by column as described in 22.4.5
	size_t d1, d2;	// d1 == number of columns, d2 == number of rows
public:
	Matrix(size_t x, size_t y);		// note: no default constructor
	Matrix(const Matrix&);
	Matrix& operator=(const Matrix&);
	~Matrix();
	
	size_t size() const { return d1*d2; }
	size_t dim1() const { return d1; }
	size_t dim2() const { return d2; }

	Slice_iter<double> row(size_t i);
	Cslice_iter<double> row(size_t i) const;

	Slice_iter<double> column(size_t i);
	Cslice_iter<double> column(size_t i) const;

	double& operator()(size_t x, size_t y);					// Fortran-style subscripts
	double operator()(size_t x, size_t y) const;

	Slice_iter<double> operator()(size_t i) { return column(i); }
	Cslice_iter<double> operator()(size_t i) const { return column(i); }

	Slice_iter<double> operator[](size_t i) { return column(i); }	// C-style subscript
	Cslice_iter<double> operator[](size_t i) const { return column(i); }

	Matrix& operator*=(double);
	
	valarray<double>& array() { return *v; }
};

#endif