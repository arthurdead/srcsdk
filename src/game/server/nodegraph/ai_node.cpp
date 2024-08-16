//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_network.h"
#include "ai_initutils.h"
#include "bitstring.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_moveprobe.h"
#include "fmtstr.h"
#include "game.h"			
#include "ai_networkmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

CAI_Link *CAI_Node::GetLink( int destNodeId )
{
	// Now make sure this node still has a link to the destID
	for ( int link = 0; link < NumLinks(); link++ )
	{
		// If we find the link the dynamic link is valid
		if ( m_Links[link]->DestNodeID(m_iID) == destNodeId )
		{
			return m_Links[link];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Add a link to this node
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Node::AddLink(CAI_Link *newLink)
{
	if ( NumLinks() == AI_MAX_NODE_LINKS )
	{
		DevMsg( "Node %d has too many links\n", m_iID );
		return;
	}

#ifdef _DEBUG
	for (int link=0;link<NumLinks();link++)
	{
		if (m_Links[link] == newLink)
		{
			AssertMsgOnce( 0, "Link added to node multiple times!" );
			return;
		}
	}

	AssertMsg( newLink->m_iDestID == m_iID || newLink->m_iSrcID == m_iID, "Added link to node that doesn't reference the node" );
#endif

	m_Links.AddToTail( newLink );
}


//-----------------------------------------------------------------------------
// Purpose: Returns link if node has a link to node of the given nNodeID.
//			Otherwise returns NULL
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Link* CAI_Node::HasLink(int nNodeID)
{
	for (int link=0;link<NumLinks();link++)
	{
		// If node has link to myself, than add link to my list of links
		if (m_Links[link]->DestNodeID(m_iID) == nNodeID)
		{
			return m_Links[link];
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------
// Purpose : Called before GetShuffeledLinks to change the order in which
//			 links are returned 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Node::ShuffleLinks(void)
{
	m_iFirstShuffledLink++;
	if (m_iFirstShuffledLink >= NumLinks())
	{
		m_iFirstShuffledLink = 0;
	}
}

//------------------------------------------------------------------------------
// Purpose : Used to get links in different order each time.  
//			 Call ShuffleLinks() first
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_Link* CAI_Node::GetShuffeledLink(int nNum)
{
	int nLinkID = m_iFirstShuffledLink + nNum;
	if (nLinkID >= NumLinks())
	{
		nLinkID -= NumLinks();
	}
	return m_Links[nLinkID];
}

//-----------------------------------------------------------------------------
// Purpose: Climb nodes are centered over the climb surface, the must be
//			shifted away from the climb surface according to the hull size
//			of the NPC using the climb
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_Node::GetPosition(int hull)
{
	if (m_eNodeType == NODE_CLIMB) 
	{
		// Shift by the length of the hull and some small fudge
		float  shift  = (0.5*NAI_Hull::Length(hull)) + (NODE_CLIMB_OFFSET);

		Vector offsetDir = Vector(cos(DEG2RAD(m_flYaw)),sin(DEG2RAD(m_flYaw)),0);

		Vector origin;
		if (m_eNodeInfo & bits_NODE_CLIMB_OFF_FORWARD)
		{
			origin = m_vOrigin + (shift * offsetDir);
		}
		else if (m_eNodeInfo & bits_NODE_CLIMB_OFF_LEFT)
		{
			Vector upDir(0,0,1);
			Vector leftDir;
			CrossProduct( offsetDir, upDir, leftDir);
			origin = m_vOrigin - (2 * shift * leftDir) - (shift * offsetDir);
		}
		else if (m_eNodeInfo & bits_NODE_CLIMB_OFF_RIGHT)
		{
			Vector upDir(0,0,1);
			Vector leftDir;
			CrossProduct( offsetDir, upDir, leftDir);
			origin = m_vOrigin + (2 * shift * leftDir) - (shift * offsetDir);
		}
		else 
		{
			origin = m_vOrigin - (shift * offsetDir);
		}

		return Vector( origin.x, origin.y, origin.z + m_flVOffset[hull] );
	}
	else if (m_eNodeType == NODE_GROUND)
	{
		// this is the floor resting position of this hull, adjusted to account for mins.z
		return Vector( m_vOrigin.x, m_vOrigin.y, m_vOrigin.z + m_flVOffset[hull] );
	}
	else
	{
		return m_vOrigin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Node::CAI_Node( int id, const Vector &origin, float yaw  ) 
 :	m_Links( 0, 4 )
{
	m_vOrigin		= origin;
	m_iID			= id;

	for (int i = 0; i < NUM_HULLS; i++)
	{
		m_flVOffset[i] = 0.0;
	}

	m_eNodeType		= NODE_GROUND;
	m_eNodeInfo		= 0;

	m_iFirstShuffledLink	= 0;

	m_pHint					= NULL;
	m_flYaw					= yaw;

	m_flNextUseTime			= 0;

	m_zone = AI_NODE_ZONE_UNKNOWN;
};

