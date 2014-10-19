#ifndef _FLOATINGPOINT_H_
#define _FLOATINGPOINT_H_

char *ecvt(double arg, int ndigits, int *decpt, int *sign);
char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
char *fcvt(double arg, int ndigits, int *decpt, int *sign);
char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);

#endif // _FLOATINGPOINT_H_

