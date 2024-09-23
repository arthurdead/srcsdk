#ifndef EXTENT_H
#define EXTENT_H

#pragma once

#include "vector.h"

struct Extent
{
	Vector lo, hi;

	void Init( void )
	{
		lo.Init();
		hi.Init();
	}

	void Init( const Vector &lo_, const Vector &hi_ )
	{
		lo = lo_;
		hi = hi_;
	}

	float SizeX( void ) const	{ return hi.x - lo.x; }
	float SizeY( void ) const	{ return hi.y - lo.y; }
	float SizeZ( void ) const	{ return hi.z - lo.z; }
	float Area( void ) const	{ return SizeX() * SizeY(); }

	// Increase bounds to contain the given point
	void Encompass( const Vector &pos )
	{
		for ( int i=0; i<3; ++i )
		{
			if ( pos[i] < lo[i] )
			{
				lo[i] = pos[i];
			}
			else if ( pos[i] > hi[i] )
			{
				hi[i] = pos[i];
			}
		}
	}

	// Increase bounds to contain the given extent
	void Encompass( const Extent &extent )
	{
		Encompass( extent.lo );
		Encompass( extent.hi );
	}

	// return true if 'pos' is inside of this extent
	bool Contains( const Vector &pos ) const
	{
		return (pos.x >= lo.x && pos.x <= hi.x &&
				pos.y >= lo.y && pos.y <= hi.y &&
				pos.z >= lo.z && pos.z <= hi.z);
	}
	
	// return true if this extent overlaps the given one
	bool IsOverlapping( const Extent &other ) const
	{
		return (lo.x <= other.hi.x && hi.x >= other.lo.x &&
				lo.y <= other.hi.y && hi.y >= other.lo.y &&
				lo.z <= other.hi.z && hi.z >= other.lo.z);
	}

	// return true if this extent completely contains the given one
	bool IsEncompassing( const Extent &other, float tolerance = 0.0f ) const
	{
		return (lo.x <= other.lo.x + tolerance && hi.x >= other.hi.x - tolerance &&
				lo.y <= other.lo.y + tolerance && hi.y >= other.hi.y - tolerance &&
				lo.z <= other.lo.z + tolerance && hi.z >= other.hi.z - tolerance);
	}
};

#endif
