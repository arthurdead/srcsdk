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
		SetColor( 255, 255, 255 );
	}

	Color24( const Color24 &rhs ) = default;
	Color24 &operator=( const Color24 &rhs ) = default;
	Color24( Color24 &&rhs ) = default;
	Color24 &operator=( Color24 &&rhs ) = default;
	~Color24() = default;

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
		return r() == rhs.r() &&
				g() == rhs.g() &&
				b() == rhs.b();
	}

	bool operator!=(const Color24 &rhs) const
	{
		return r() != rhs.r() ||
				g() != rhs.g() ||
				b() != rhs.b();
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
class Color
{
public:
	// constructors
	Color()
	{
		SetColor( 255, 255, 255, 255 );
	}

	Color( const Color &rhs ) = default;
	Color &operator=( const Color &rhs ) = default;
	Color( Color &&rhs ) = default;
	Color &operator=( Color &&rhs ) = default;
	~Color() = default;

	Color(unsigned char _r,unsigned char _g,unsigned char _b)
	{
		SetColor(_r, _g, _b, 255);
	}
	Color(unsigned char _r,unsigned char _g,unsigned char _b,unsigned char _a)
	{
		SetColor(_r, _g, _b, _a);
	}
	Color(Color24 clr_,unsigned char _a)
	{
		SetColor(clr_.r(), clr_.g(), clr_.b(), _a);
	}
	Color(FatColor32 other);
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		_color[0] = _r;
		_color[1] = _g;
		_color[2] = _b;
	}

	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
	{
		_color[0] = _r;
		_color[1] = _g;
		_color[2] = _b;
		_color[3] = _a;
	}

	void SetColor(Color24 other)
	{
		SetColor( other.r(), other.g(), other.b() );
	}

	void SetColor(FatColor32 other);

	void SetR(unsigned char _r)
	{
		_color[0] = _r;
	}

	void SetG(unsigned char _g)
	{
		_color[1] = _g;
	}

	void SetB(unsigned char _b)
	{
		_color[2] = _b;
	}

	void SetA(unsigned char _a)
	{
		_color[3] = _a;
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b, unsigned char &_a) const
	{
		_r = r();
		_g = g();
		_b = b();
		_a = a();
	}

	inline unsigned char r() const	{ return _color[0]; }
	inline unsigned char g() const	{ return _color[1]; }
	inline unsigned char b() const	{ return _color[2]; }
	inline unsigned char a() const	{ return _color[3]; }

	bool operator==(const Color &rhs) const
	{
		return r() == rhs.r() &&
				g() == rhs.g() &&
				b() == rhs.b() &&
				a() == rhs.a();
	}

	bool operator!=(const Color &rhs) const
	{
		return r() != rhs.r() ||
				g() != rhs.g() ||
				b() != rhs.b() ||
				a() != rhs.a();
	}

	operator FatColor32() const;
	operator Color24() const { return Color24( r(), g(), b() ); }

private:
	unsigned char _color[4];
};

typedef Color Color32;
typedef Color32 color32;

// compressed color format 
struct ColorRGBExp32
{
	// constructors
	ColorRGBExp32()
	{
		SetColor( 255, 255, 255, 127 );
	}

	ColorRGBExp32( const ColorRGBExp32 &rhs ) = default;
	ColorRGBExp32 &operator=( const ColorRGBExp32 &rhs ) = default;
	ColorRGBExp32( ColorRGBExp32 &&rhs ) = default;
	ColorRGBExp32 &operator=( ColorRGBExp32 &&rhs ) = default;
	~ColorRGBExp32() = default;

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
		r_ = _r;
		g_ = _g;
		b_ = _b;
	}

	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, signed char _e)
	{
		r_ = _r;
		g_ = _g;
		b_ = _b;
		e_ = _e;
	}

	void SetColor(Color24 other)
	{
		SetColor( other.r(), other.g(), other.b() );
	}

	void SetR(unsigned char _r)
	{
		r_ = _r;
	}

	void SetG(unsigned char _g)
	{
		g_ = _g;
	}

	void SetB(unsigned char _b)
	{
		b_ = _b;
	}

	void SetE(signed char _e)
	{
		e_ = _e;
	}

	void GetColor(unsigned char &_r, unsigned char &_g, unsigned char &_b, signed char &_e) const
	{
		_r = r();
		_g = g();
		_b = b();
		_e = e();
	}

	inline unsigned char r() const	{ return r_; }
	inline unsigned char g() const	{ return g_; }
	inline unsigned char b() const	{ return b_; }
	inline signed char e() const	{ return e_; }

	bool operator==(const ColorRGBExp32 &rhs) const
	{
		return r() == rhs.r() &&
				g() == rhs.g() &&
				b() == rhs.b() &&
				e() == rhs.e();
	}

	bool operator!=(const ColorRGBExp32 &rhs) const
	{
		return r() != rhs.r() ||
				g() != rhs.g() ||
				b() != rhs.b() ||
				e() != rhs.e();
	}

	operator Color24() const { return Color24( r(), g(), b() ); }

private:
	unsigned char r_;
	unsigned char g_;
	unsigned char b_;
	signed char e_;
};

class FatColor32
{
public:
	// constructors
	FatColor32()
	{
		SetColor( 255, 255, 255, 255 );
	}

	FatColor32( const FatColor32 &rhs ) = default;
	FatColor32 &operator=( const FatColor32 &rhs ) = default;
	FatColor32( FatColor32 &&rhs ) = default;
	FatColor32 &operator=( FatColor32 &&rhs ) = default;
	~FatColor32() = default;

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
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
	{
		r_ = _r;
		g_ = _g;
		b_ = _b;
		a_ = _a;
	}

	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		r_ = _r;
		g_ = _g;
		b_ = _b;
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
		r_ = _r;
	}

	void SetG(unsigned char _g)
	{
		g_ = _g;
	}

	void SetB(unsigned char _b)
	{
		b_ = _b;
	}

	void SetA(unsigned char _a)
	{
		a_ = _a;
	}

	inline unsigned char r() const	{ return r_; }
	inline unsigned char g() const	{ return g_; }
	inline unsigned char b() const	{ return b_; }
	inline unsigned char a() const	{ return a_; }

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

	operator Color32() { return Color32( r(), g(), b(), a() ); }
	operator Color24() { return Color24( r(), g(), b() ); }

private:
	unsigned int r_;
	unsigned int g_;
	unsigned int b_;
	unsigned int a_;
};

typedef FatColor32 colorVec;

inline Color::Color(FatColor32 other)
{ SetColor(other); }

inline Color32::operator FatColor32() const
{ return FatColor32( r(), g(), b(), a() ); }

inline void Color32::SetColor( FatColor32 other )
{ SetColor( other.r(), other.g(), other.b(), other.a() ); }

#endif // COLOR_H
