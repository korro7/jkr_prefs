#if !defined(cubicroot_h)
#define cubicroot_h


#include <math.h>


void cubic(double a[4], double rr[3], double ri[3])
{
                int i;
                double a0, a1, a2, a3;
                double g, h, y1, sh, xk, theta, pi, xy1, xy2, xy3;
                double y2, z1, z2, z3, z4;

                for (i = 0; i < 3; i ++)
                {
                                rr[i] = 0.0;
                                ri[i] = 0.0;
                }
                a0 = a[0];
                a1 = a[1]/3.0;
                a2 = a[2]/3.0;
                a3 = a[3];
                g = (a0 * a0) * a3 - 3.0 * a0 * a1 * a2 + 2.0 * pow(a1, 3);
                h = a0 * a2 - a1 * a1;
                y1 = g * g + 4.0 * pow(h, 3);
              
                if (y1 < 0.0)
                {
                                sh = sqrt(-h);
                                xk = 2.0 * sh;
                                theta = acos(g / (2.0 * h * sh)) / 3.0;
                                xy1 = 2.0 * sh * cos(theta);
                                pi = 3.1416;
                                xy2 = 2.0 * sh * cos(theta + (2.0 * pi / 3.0));
                                xy3 = 2.0 * sh * cos(theta + (4.0 * pi / 3.0));
                                rr[0] = (xy1 - a1) / a0;
                                rr[1] = (xy2 - a1) / a0;
                                rr[2] = (xy3 - a1) / a0;
                                return;
                }
                else
                {
                                y2 = sqrt(y1);
                                z1 = (g + y2) / 2.0;
                                z2 = (g - y2) / 2.0;
                                if (z1 < 0.0)
                                {
                                    z3 = pow(-z1, 1.0/3.0);
                                    z3 = -z3;
                                }
                                else 
                                                z3 = pow(z1, 1.0/3.0);
                                if (z2 < 0.0)
                                {
                                                z4 = pow(-z2, 1.0/3.0);
                                                z4 = - z4;
                                }
                                else
                                                z4 = pow(z2, 1.0/3.0);
                                rr[0] = -(a1 + z3 + z4) / a0;
                                rr[1] = (-2.0 * a1 + z3 + z4) / (2.0 * a0);
                                ri[1] = sqrt(3.0) * (z4 - z3) / (2.0 * a0);
                                rr[2] = rr[1];
                                ri[2] = -ri[1];
                                return;
                }
}


#endif
