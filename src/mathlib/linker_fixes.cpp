#include "tier0/platform.h"
#include <cmath>
#include <mathlib/ssemath.h>

extern "C" {
	LIB_LOCAL SELECTANY fltx4 CDECL _Z28Pow_FixedPoint_Exponent_SIMDRKU8__vectorfi(const fltx4 &x, int exponent)
	{ return Pow_FixedPoint_Exponent_SIMD(x, exponent); }
}

LIB_EXPORT SELECTANY double CDECL __exp_finite(double arg)
{ return exp(arg); }
LIB_EXPORT SELECTANY float CDECL __expf_finite(float arg)
{ return expf(arg); }
LIB_EXPORT SELECTANY double CDECL __pow_finite(double base, double exp)
{ return pow(base, exp); }
LIB_EXPORT SELECTANY float CDECL __powf_finite(float base, float exp)
{ return powf(base, exp); }
LIB_EXPORT SELECTANY double CDECL __log_finite(double arg)
{ return log(arg); }
LIB_EXPORT SELECTANY float CDECL __logf_finite(float arg)
{ return logf(arg); }
LIB_EXPORT SELECTANY double CDECL __asin_finite(double arg)
{ return asin(arg); }
LIB_EXPORT SELECTANY double CDECL __acos_finite(double arg)
{ return acos(arg); }
LIB_EXPORT SELECTANY float CDECL __acosf_finite(float arg)
{ return acosf(arg); }
LIB_EXPORT SELECTANY double CDECL __atan2_finite(double y, double x)
{ return atan2(y, x); }
LIB_EXPORT SELECTANY float CDECL __atan2f_finite(float y, float x)
{ return atan2f(y, x); }
