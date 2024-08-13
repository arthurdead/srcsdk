#include <cmath>
#include <mathlib/ssemath.h>

extern "C" {
	#pragma GCC visibility push(hidden)

	[[using gnu: weak]] fltx4 _Z28Pow_FixedPoint_Exponent_SIMDRKU8__vectorfi(const fltx4 &x, int exponent)
	{ return Pow_FixedPoint_Exponent_SIMD(x, exponent); }

	#pragma GCC visibility pop
}

extern "C" {
	#pragma GCC visibility push(default)

	[[using gnu: weak,cdecl]] double __exp_finite(double arg)
	{ return exp(arg); }
	[[using gnu: weak,cdecl]] float __expf_finite(float arg)
	{ return expf(arg); }
	[[using gnu: weak,cdecl]] double __pow_finite(double base, double exp)
	{ return pow(base, exp); }
	[[using gnu: weak,cdecl]] float __powf_finite(float base, float exp)
	{ return powf(base, exp); }
	[[using gnu: weak,cdecl]] double __log_finite(double arg)
	{ return log(arg); }
	[[using gnu: weak,cdecl]] float __logf_finite(float arg)
	{ return logf(arg); }
	[[using gnu: weak,cdecl]] double __asin_finite(double arg)
	{ return asin(arg); }
	[[using gnu: weak,cdecl]] double __acos_finite(double arg)
	{ return acos(arg); }
	[[using gnu: weak,cdecl]] float __acosf_finite(float arg)
	{ return acosf(arg); }
	[[using gnu: weak,cdecl]] double __atan2_finite(double y, double x)
	{ return atan2(y, x); }
	[[using gnu: weak,cdecl]] float __atan2f_finite(float y, float x)
	{ return atan2f(y, x); }

	#pragma GCC visibility pop
}
