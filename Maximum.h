#if !defined(Maximum_h)
#define Maximum_h


#include <math.h>

double max(double x1, double x2, double x3)
{
                double m;
	               m = x1;
                if (x2 > m)
                    m = x2;
                if (x3 > m)
                    m = x3;

                return m;
}
#endif
