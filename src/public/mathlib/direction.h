#ifndef DIRECTION_H
#define DIRECTION_H

#pragma once

#include "vector.h"

enum DirType
{
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};

enum CornerType
{
	NORTH_WEST = 0,
	NORTH_EAST = 1,
	SOUTH_EAST = 2,
	SOUTH_WEST = 3,

	NUM_CORNERS
};

enum RelativeDirType
{
	FORWARD = 0,
	RIGHT,
	BACKWARD,
	LEFT,
	UP,
	DOWN,

	NUM_RELATIVE_DIRECTIONS
};

//--------------------------------------------------------------------------------------------------------------
inline DirType OppositeDirection( DirType dir )
{
	switch( dir )
	{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST:	return WEST;
		case WEST:	return EAST;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline DirType DirectionLeft( DirType dir )
{
	switch( dir )
	{
		case NORTH: return WEST;
		case SOUTH: return EAST;
		case EAST:	return NORTH;
		case WEST:	return SOUTH;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline DirType DirectionRight( DirType dir )
{
	switch( dir )
	{
		case NORTH: return EAST;
		case SOUTH: return WEST;
		case EAST:	return SOUTH;
		case WEST:	return NORTH;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void AddDirectionVector( Vector *v, DirType dir, float amount )
{
	switch( dir )
	{
		case NORTH: v->y -= amount; return;
		case SOUTH: v->y += amount; return;
		case EAST:  v->x += amount; return;
		case WEST:  v->x -= amount; return;
		default: break;
	}
}

//--------------------------------------------------------------------------------------------------------------
inline float DirectionToAngle( DirType dir )
{
	switch( dir )
	{
		case NORTH:	return 270.0f;
		case SOUTH:	return 90.0f;
		case EAST:	return 0.0f;
		case WEST:	return 180.0f;
		default: break;
	}

	return 0.0f;
}

//--------------------------------------------------------------------------------------------------------------
inline DirType AngleToDirection( float angle )
{
	while( angle < 0.0f )
		angle += 360.0f;

	while( angle > 360.0f )
		angle -= 360.0f;

	if (angle < 45 || angle > 315)
		return EAST;

	if (angle >= 45 && angle < 135)
		return SOUTH;

	if (angle >= 135 && angle < 225)
		return WEST;

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void DirectionToVector2D( DirType dir, Vector2D *v )
{
	switch( dir )
	{
		default: Assert(0);
		case NORTH: v->x =  0.0f; v->y = -1.0f; break;
		case SOUTH: v->x =  0.0f; v->y =  1.0f; break;
		case EAST:  v->x =  1.0f; v->y =  0.0f; break;
		case WEST:  v->x = -1.0f; v->y =  0.0f; break;
	}
}


//--------------------------------------------------------------------------------------------------------------
inline void CornerToVector2D( CornerType dir, Vector2D *v )
{
	switch( dir )
	{
		default: Assert(0);
		case NORTH_WEST: v->x = -1.0f; v->y = -1.0f; break;
		case NORTH_EAST: v->x =  1.0f; v->y = -1.0f; break;
		case SOUTH_EAST: v->x =  1.0f; v->y =  1.0f; break;
		case SOUTH_WEST: v->x = -1.0f; v->y =  1.0f; break;
	}

	v->NormalizeInPlace();
}


//--------------------------------------------------------------------------------------------------------------
// Gets the corner types that surround the given direction
inline void GetCornerTypesInDirection( DirType dir, CornerType *first, CornerType *second )
{
	switch ( dir )
	{
	default:
		Assert(0);
	case NORTH:
		*first = NORTH_WEST;
		*second = NORTH_EAST;
		break;
	case SOUTH:
		*first = SOUTH_WEST;
		*second = SOUTH_EAST;
		break;
	case EAST:
		*first = NORTH_EAST;
		*second = SOUTH_EAST;
		break;
	case WEST:
		*first = NORTH_WEST;
		*second = SOUTH_WEST;
		break;
	}
}

#endif
