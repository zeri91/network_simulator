/*****************************************************************************

			       S T A T . C

	Modified from class SampleStatistic of GNU library

		 *****************************************************************************/
/* la compilazione in prova funziona */

#include "Stat.h"
#include <math.h>
#include "OchInc.h"
#include "OchObject.h"
#ifndef HUGE_VAL
#define HUGE_VAL 1.0e+99
#endif
#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace NS_OCH;

// t-distribution: given p-value and degrees of freedom, return t-value
// adapted from Peizer & Pratt JASA, vol63, p1416
double tval(double p, int df) {
  	double t;
  	int positive = p >= 0.5;
  	p = (positive)? 1.0 - p : p;
  	if (p <= 0.0 || df <= 0) t = HUGE_VAL;
 	else if (p == 0.5) t = 0.0;
  	else if (df == 1) t = 1.0 / tan((p + p) * 1.57079633);
  	else if (df == 2) t = sqrt(1.0 / ((p + p) * (1.0 - p)) - 2.0);
  	else 	{	
    		double ddf = df;
    		double a = sqrt(log(1.0 / (p * p)));
    		double aa = a * a;
    		a = a - ((2.515517 + 0.802853*a + 0.010328*aa) /
             		(1.0 + 1.432788*a + 0.189269*aa + 0.001308*aa*a));
    		t = ddf - 0.666666667 + 1.0 / (10.0 * ddf);
    		t = sqrt(ddf * (exp(a*a * (ddf - 0.833333333)/(t * t)) - 1.0));
  		}
  	return (positive)? t : -t;
	}

void Sstat::reset() {
    	n = 0; 
	x = x2 = last = 0.0;
    	maxValue = -HUGE_VAL;
    	minValue = HUGE_VAL;
    	}

void Sstat::operator+=(double value) {
    	n += 1;
		last = value;
    	x += value;
    	x2 += (value * value);
    	if ( minValue > value) minValue = value;
    	if ( maxValue < value) maxValue = value;
    	}

double Sstat::mean() {
	//cout << "n = " << n;
	if (n > 0)
	{
		//cout << ", x = " << x;
		//cout << " -> x/n = ";
		return(x / n);
	}
	else return(0.0);
}

double Sstat::var() {
    	if (n > 1) return((x2 - ((x * x) / n)) / (n - 1));
    	else return(0.0);
    	}

double Sstat::stddev() {
   	return(sqrt(var()));
    	}

double Sstat::confidence(double p_value) {
  	int df = n - 1;
  	if (df <= 0) return HUGE_VAL;
  	double t = tval((1.0 + p_value) * 0.5, df);
  	if (t == HUGE_VAL) return t;
  	else return (t * stddev()) / sqrt(double(n));
	}

double Sstat::confpercerr(double p_value) {
   	double Mean = mean();
   	if (Mean == 0.0) 
		return 100.0;		// can't compute % error
   	else 
		return((confidence(p_value)*100.0)/Mean);
   	}

void 	Sstat::StatLog(Sstat* stat,double pval,char* st,ostream& out1) const {
	out1 << st << ":" << endl;
	out1 << "- Mean: "; out1 << stat->mean() << endl;
	out1 << "- Variance: "; out1 << stat->var() << endl;
	out1 << "- Standard Deviation: "; out1 << stat->stddev() << endl;
	out1 << "- Error %: "; out1 << stat->confpercerr(pval) << endl;
}