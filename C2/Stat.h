/*****************************************************************************

				S T A T . H

	Contains definition of the following two classes:
	      - Sstat: 	Modified and extended from class SampleStatistic of 
			GNU library. 
	      - SLstat: Derives from class Sstat and adds two list management
			of the inserted samples, according to a construction
			flag:  flag=1: samples ordered in term of insertion time
			       flag=2: samples sorted in ascendent order
			       flag=0: default case, management of both lists
	

 *****************************************************************************/
#ifndef _STAT_H
#define _STAT_H
#include "OchInc.h"
#include "OchObject.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

//----------------------------------------------------------------------------
//				CLASS Sstat
//----------------------------------------------------------------------------
namespace NS_OCH {


class 	Sstat {
	protected:
    		int	n;
    		double 	x;
    		double 	x2;
		double  last;
   		double 	minValue;
		double	maxValue;
    	public :
    		Sstat();
    		virtual ~Sstat();
    		virtual void reset(); 
    		virtual void operator+=(double);
    		int  	num_samples();
		double	last_sample();
   		double  sum();
    		double 	mean();
    		double 	stddev();
    		double 	var();
    		double 	min();
    		double 	max();
    		double 	confidence(int p_percentage);
    		double 	confidence(double p_value);
    		double 	confpercerr(int p_percentage);
    		double 	confpercerr(double p_value);
		bool isconfsatisfied(double perc=1.0, double pconf=.975);
		void 	StatLog(Sstat*,double,char*, ostream&) const;
};


inline 		Sstat :: Sstat() {reset();}
inline 		Sstat::~Sstat() {}

inline 	int 	Sstat::num_samples() {return(n);}
inline		double	Sstat::last_sample() {return(last);}
inline 	double 	Sstat::min() {return(minValue);}
inline 	double 	Sstat::max() {return(maxValue);}
inline 	double 	Sstat::sum() {return(x);}

inline  double	Sstat::confidence(int p_percentage) { 
			return confidence(double(p_percentage)*0.01); }

inline  double	Sstat::confpercerr(int p_percentage) { 
			return confpercerr(double(p_percentage)*0.01); }

inline bool Sstat::isconfsatisfied(double perc, double pconf)
{
#ifdef DEBUGB
	cout << endl << "CHECK CONFIDENCE: " << confpercerr(pconf) << " < " << perc << "?" << endl;
	//cin.get();
#endif // DEBUGB

	return (confpercerr(pconf) < perc);
}


};
#endif  /* _STAT_H */


