#include "matrix.h"


inline Slice_iter<double> Matrix::row(size_t i)
{
	return Slice_iter<double>(v,slice(i,d1,d2));
}

inline Cslice_iter<double> Matrix::row(size_t i) const
{
	return Cslice_iter<double>(v,slice(i,d1,d2));
}

inline Slice_iter<double> Matrix::column(size_t i)
{
	return Slice_iter<double>(v,slice(i*d2,d2,1));
}

inline Cslice_iter<double> Matrix::column(size_t i) const
{
	return Cslice_iter<double>(v,slice(i*d2,d2,1));
}

Matrix::Matrix(size_t x, size_t y)
{
	// check that x and y are sensible
	d1 = x;
	d2 = y;
	v = new valarray<double>(x*y);
}

Matrix::~Matrix()
{
	delete v;
}

double& Matrix::operator()(size_t x, size_t y)
{
	return column(x)[y];
}



//-------------------------------------------------------------




double mul(const Cslice_iter<double>& v1, const valarray<double>& v2)
{
	double res = 0;
	for (size_t i = 0; i<v2.size(); i++) res+= v1[i]*v2[i];
	return res;
}


valarray<double> operator*(const Matrix& m, const valarray<double>& v)
{
	if (m.dim1()!=v.size()) cerr << "wrong number of elements in m*v\n";

	valarray<double> res(m.dim2());
	for (size_t i = 0; i<m.dim2(); i++) res[i] = mul(m.row(i),v);
	return res;
}


// alternative definition of m*v

//valarray<double> operator*(const Matrix& m, valarray<double>& v)
valarray<double> mul_mv(const Matrix& m, valarray<double>& v)
{
	if (m.dim1()!=v.size()) cerr << "wrong number of elements in m*v\n";

	valarray<double> res(m.dim2());

	for (size_t i = 0; i<m.dim2(); i++) {
		const Cslice_iter<double>& ri = m.row(i);
		res[i] = inner_product(ri,ri.end(),&v[0],double(0));
	}
	return res;
}



valarray<double> operator*(valarray<double>& v, const Matrix& m)
{
	if (v.size()!=m.dim2()) cerr << "wrong number of elements in v*m\n";

	valarray<double> res(m.dim1());

	for (size_t i = 0; i<m.dim1(); i++) {
		const Cslice_iter<double>& ci = m.column(i);
		res[i] = inner_product(ci,ci.end(),&v[0],double(0));
	}
	return res;
}

Matrix& Matrix::operator*=(double d)
{
	(*v) *= d;
	return *this;
}

ostream& operator<<(ostream& os, Matrix& m)
{
	for(int y=0; y<m.dim2(); y++)
	{
		for(int x=0; x<m.dim1(); x++)
			os<<m[x][y]<<"\t";
		os << "\n";
	}
	return os;
}


//-------------------------------------------------------------


/**
int main()
{
	f(3,4);
	f(4,3);

	g(3,4);
	g(4,3);
}*//*
int main()
{

	deque<ogge> coda(4,ogge(0));


	for (int i=0;i<4;i++)
	{
		coda[i].val=4-i;
		cout<<coda[i].val<<endl;
	

	}

	
	cout<<endl;
	//sort(coda.begin(),coda.end(),conf());
//coda.insert(coda.begin(),5);
	for (i=0;i<4;i++)
	{
		cout<<coda[i].val<<endl;
	}
	
	Matrix mat(6,7);
return 0;

}*/