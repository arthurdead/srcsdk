//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLOR_H
#define COLOR_H

#pragma once

class Color24
{
public:
	// constructors
	Color24()
	{
		SetColor(255, 255, 255);
	}
	Color24(unsigned char _r,unsigned char _g,unsigned char _b)
	{
		SetColor(_r, _g, _b);
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		r_ = _r;
		g_ = _g;
		b_ = _b;
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b) const
	{
		_r = r();
		_g = g();
		_b = b();
	}

	void SetR(unsigned char _r)
	{
		r_ = _r;
	}

	void SetG(unsigned char _g)
	{
		r_ = _g;
	}

	void SetB(unsigned char _b)
	{
		r_ = _b;
	}

	inline unsigned char r() const	{ return r_; }
	inline unsigned char g() const	{ return g_; }
	inline unsigned char b() const	{ return b_; }

	bool operator==(const Color24 &rhs) const
	{
		return r_ == rhs.r_ && g_ == rhs.g_ && b_ == rhs.b_;
	}

	bool operator!=(const Color24 &rhs) const
	{
		return r_ != rhs.r_ || g_ != rhs.g_ || b_ != rhs.b_;
	}

	Color24( const Color24 &rhs )
		: r_(rhs.r_), g_(rhs.g_), b_(rhs.b_)
	{
	}

	Color24 &operator=( const Color24 &rhs )
	{
		r_ = rhs.r_;
		g_ = rhs.g_;
		b_ = rhs.b_;
		return *this;
	}

private:
	unsigned char r_;
	unsigned char g_;
	unsigned char b_;
};

typedef Color24 color24;

class FatColor32;

//-----------------------------------------------------------------------------
// Purpose: Basic handler for an rgb set of colors
//			This class is fully inline
//-----------------------------------------------------------------------------
class Color32
{
public:
	// constructors
	Color32()
	{
		SetColor(255, 255, 255, 255);
	}
	Color32(unsigned char _r,unsigned char _g,unsigned char _b)
	{
		SetColor(_r, _g, _b, 255);
	}
	Color32(unsigned char _r,unsigned char _g,unsigned char _b,unsigned char _a)
	{
		SetColor(_r, _g, _b, _a);
	}
	Color32(Color24 clr_,unsigned char _a)
	{
		SetColor(clr_.r(), clr_.g(), clr_.b(), _a);
	}
	Color32(FatColor32 other);
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		_clr = static_cast<unsigned int>(_r);
		_clr |= (static_cast<unsigned int>(_g) << 8);
		_clr |= (static_cast<unsigned int>(_b) << 16);
	}

	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
	{
		_clr = static_cast<unsigned int>(_r);
		_clr |= (static_cast<unsigned int>(_g) << 8);
		_clr |= (static_cast<unsigned int>(_b) << 16);
		_clr |= (static_cast<unsigned int>(_a) << 24);
	}

	void SetColor(Color24 other)
	{
		SetColor( other.r(), other.g(), other.b() );
	}

	void SetColor(FatColor32 other);

	void SetR(unsigned char _r)
	{
		_clr &= ~static_cast<unsigned int>(0xFF);
		_clr |= static_cast<unsigned int>(_r);
	}

	void SetG(unsigned char _g)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 8);
		_clr |= (static_cast<unsigned int>(_g) << 8);
	}

	void SetB(unsigned char _b)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 16);
		_clr |= (static_cast<unsigned int>(_b) << 16);
	}

	void SetA(unsigned char _a)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 24);
		_clr |= (static_cast<unsigned int>(_a) << 24);
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b, unsigned char &_a) const
	{
		_r = r();
		_g = g();
		_b = b();
		_a = a();
	}

	inline unsigned char r() const	{ return _clr & 0xFF; }
	inline unsigned char g() const	{ return (_clr >> 8) & 0xFF; }
	inline unsigned char b() const	{ return (_clr >> 16) & 0xFF; }
	inline unsigned char a() const	{ return (_clr >> 24) & 0xFF; }

	bool operator==(const Color32 &rhs) const
	{
		return _clr == rhs._clr;
	}

	bool operator!=(const Color32 &rhs) const
	{
		return _clr != rhs._clr;
	}

	Color32( const Color32 &rhs )
		: _clr(rhs._clr)
	{
	}

	Color32 &operator=( const Color32 &rhs )
	{
		_clr = rhs._clr;
		return *this;
	}

	operator FatColor32() const;
	operator Color24() const { return Color24( r(), g(), b() ); }

private:
	unsigned int _clr;
};

typedef Color32 Color;
typedef Color32 color32;

// compressed color format 
struct ColorRGBExp32
{
	// constructors
	ColorRGBExp32()
	{
		SetColor(255, 255, 255, 127);
	}
	ColorRGBExp32(unsigned char _r,unsigned char _g,unsigned char _b)
	{
		SetColor(_r, _g, _b, 127);
	}
	ColorRGBExp32(unsigned char _r,unsigned char _g,unsigned char _b,signed char _e)
	{
		SetColor(_r, _g, _b, _e);
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		_clr = static_cast<unsigned int>(_r);
		_clr |= (static_cast<unsigned int>(_g) << 8);
		_clr |= (static_cast<unsigned int>(_b) << 16);
	}

	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, signed char _e)
	{
		_clr = static_cast<unsigned int>(_r);
		_clr |= (static_cast<unsigned int>(_g) << 8);
		_clr |= (static_cast<unsigned int>(_b) << 16);
		_clr |= (static_cast<unsigned int>(_e) << 24);
	}

	void SetColor(Color24 other)
	{
		SetColor( other.r(), other.g(), other.b() );
	}

	void SetR(unsigned char _r)
	{
		_clr &= ~static_cast<unsigned int>(0xFF);
		_clr |= static_cast<unsigned int>(_r);
	}

	void SetG(unsigned char _g)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 8);
		_clr |= (static_cast<unsigned int>(_g) << 8);
	}

	void SetB(unsigned char _b)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 16);
		_clr |= (static_cast<unsigned int>(_b) << 16);
	}

	void SetE(signed char _e)
	{
		_clr &= ~(static_cast<unsigned int>(0xFF) << 24);
		_clr |= (static_cast<unsigned int>(_e) << 24);
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b, signed char &_e) const
	{
		_r = r();
		_g = g();
		_b = b();
		_e = e();
	}

	inline unsigned char r() const	{ return _clr & 0xFF; }
	inline unsigned char g() const	{ return (_clr >> 8) & 0xFF; }
	inline unsigned char b() const	{ return (_clr >> 16) & 0xFF; }
	inline signed char e() const	{ return (_clr >> 24) & 0xFF; }

	bool operator==(const ColorRGBExp32 &rhs) const
	{
		return _clr == rhs._clr;
	}

	bool operator!=(const ColorRGBExp32 &rhs) const
	{
		return _clr != rhs._clr;
	}

	ColorRGBExp32( const ColorRGBExp32 &rhs )
		: _clr(rhs._clr)
	{
	}

	ColorRGBExp32 &operator=( const ColorRGBExp32 &rhs )
	{
		_clr = rhs._clr;
		return *this;
	}

	operator Color24() const { return Color24( r(), g(), b() ); }

private:
	unsigned int _clr;
};

class FatColor32
{
public:
	// constructors
	FatColor32()
	{
		SetColor(255, 255, 255, 255);
	}
	FatColor32(unsigned char _r,unsigned char _g,unsigned char _b,unsigned char _a=255)
	{
		SetColor(_r, _g, _b, _a);
	}
	FatColor32(Color32 other)
	{
		SetColor(other);
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255)
	{
		_clr_lo = static_cast<unsigned long long>(_r);
		_clr_lo |= (static_cast<unsigned long long>(_g) << 32);
		_clr_hi = static_cast<unsigned long long>(_b);
		_clr_hi |= (static_cast<unsigned long long>(_a) << 32);
	}

	void SetColor(Color24 other)
	{
		SetColor( other.r(), other.g(), other.b() );
	}

	void SetColor(Color32 other)
	{
		SetColor( other.r(), other.g(), other.b(), other.a() );
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b, unsigned char &_a) const
	{
		_r = r();
		_g = g();
		_b = b();
		_a = a();
	}

	void SetR(unsigned char _r)
	{
		_clr_lo &= ~static_cast<unsigned long long>(0xFFFFFFFF);
		_clr_lo |= static_cast<unsigned long long>(_r);
	}

	void SetG(unsigned char _g)
	{
		_clr_lo &= ~(static_cast<unsigned long long>(0xFFFFFFFF) << 32);
		_clr_lo |= (static_cast<unsigned long long>(_g) << 32);
	}

	void SetB(unsigned char _b)
	{
		_clr_hi &= ~static_cast<unsigned long long>(0xFFFFFFFF);
		_clr_hi |= static_cast<unsigned long long>(_b);
	}

	void SetA(unsigned char _a)
	{
		_clr_hi &= ~(static_cast<unsigned long long>(0xFFFFFFFF) << 32);
		_clr_hi |= static_cast<unsigned long long>(_a);
	}

	inline unsigned char r() const	{ return _clr_lo & 0xFF; }
	inline unsigned char g() const	{ return (_clr_lo >> 32) & 0xFF; }
	inline unsigned char b() const	{ return _clr_hi & 0xFF; }
	inline unsigned char a() const	{ return (_clr_hi >> 32) & 0xFF; }

	bool operator==(const FatColor32 &rhs) const
	{
		return r() == rhs.r() &&
				g() == rhs.g() &&
				b() == rhs.b() &&
				a() == rhs.a();
	}

	bool operator!=(const FatColor32 &rhs) const
	{
		return r() != rhs.r() ||
				g() != rhs.g() ||
				b() != rhs.b() ||
				a() != rhs.a();
	}

	FatColor32( const FatColor32 &rhs )
		: _clr_lo(rhs._clr_lo), _clr_hi(rhs._clr_hi)
	{
	}

	FatColor32 &operator=( const FatColor32 &rhs )
	{
		_clr_lo = rhs._clr_lo;
		_clr_hi = rhs._clr_hi;
		return *this;
	}

	operator Color32() { return Color32( r(), g(), b(), a() ); }
	operator Color24() { return Color24( r(), g(), b() ); }

private:
	unsigned long long _clr_lo;
	unsigned long long _clr_hi;
};

typedef FatColor32 colorVec;

inline Color32::Color32(FatColor32 other)
{ SetColor(other); }

inline Color32::operator FatColor32() const
{ return FatColor32( r(), g(), b(), a() ); }

inline void Color32::SetColor( FatColor32 other )
{ SetColor( other.r(), other.g(), other.b(), other.a() ); }

#endif // COLOR_H
