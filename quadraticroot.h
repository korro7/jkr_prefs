#if !defined(quadraticroot_h)
#define quadraticroot_h


#include <math.h>


void quadra(double a1, double a2, double a3, double *rr1, double *rr2, double *ri1, double *ri2)
{
                double rad, srad;
                rad = a2 * a2 - 4.0 * a1 * a3;
                if (rad >= 0)
                {
                                srad = sqrt(rad);
                                *rr1 = (-a2 - srad)/(2.0 * a1);
                                *rr2 = (-a2 + srad)/(2.0 * a1);
                                *ri1 = 0.0;
                                *ri2 = 0.0;
                                return;
                }
                else
                {
                                srad = sqrt(-rad);
                                *rr1 = -a2 /(2.0 * a1);
                                *rr2 = * rr1;
                                *ri1 = srad/(2.0 * a1);
                                *ri2 = - *ri1;
                                return;
                }
}

#endif
