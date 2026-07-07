#if !defined(quarticroot__h)
#define quarticroot__h

#include "quadraticroot.h"
#include "cubicroot.h"
#include "Maximum.h"
#include <math.h>


void quart(double a[5], double rr[4], double ri[4])

{
                int i;
                double aa[5], b[4], rrc[3], ric[3];
                double x, y, z, c1, c2, c3, qr1, qr2, qi1, qi2;

                aa[0] = a[0];

                for (i = 1; i < 5; i ++)
                                aa[i] = a[i] / a[0];

                b[0] = 1.0;
                b[1] = -aa[2];
                b[2] = aa[3] * aa[1] - 4.0 * aa[4];
                b[3] = aa[4] * (4.0 * aa[2] - aa[1] * aa[1]) - aa[3] * aa[3];

                cubic(b, rrc, ric);

                if (fabs(ric[1]) < 1.0e-6 ) // ric[1] == 0
                {
				x = max(rrc[0], rrc[1], rrc[2]);
                                rrc[0] = x;
                }
                x = rrc[0] / 2.0;
                if ((x * x - aa[4]) > 0.0)
                {
                                y = sqrt(x * x - aa[4]);
                                z = - (aa[3] - aa[1] * x) / (2.0 * y);
                }
                else
                {
                                y = 0.0;
                                z = sqrt(pow(aa[1] / 2.0, 2) + 2.0 * x - aa[2]);
                }

                c1 = 1.0;
                c2 = aa[1] / 2.0 + z;
                c3 = x + y;
             
                quadra(c1, c2, c3, &qr1, &qr2, &qi1, &qi2);

				rr[0] = qr1;
                rr[1] = qr2;
                ri[0] = qi1;
                ri[1] = qi2;

                c1 = 1.0;
                c2 = aa[1] / 2.0 - z;
                c3 = x - y;
             
                quadra(c1, c2, c3, &qr1, &qr2, &qi1, &qi2);
                rr[2] = qr1;
                rr[3] = qr2;
                ri[2] = qi1;
                ri[3] = qi2;

                return;
}



/* end of  quartic root calculation */

#endif
