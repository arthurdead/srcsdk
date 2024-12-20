//===== Copyright © 1996-2009, Valve Corporation, All rights reserved. ======//
//
//  Purpose: Plays a movie and reports on finish
//
//===========================================================================//

#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLogicPlayMovie : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicPlayMovie, CLogicalEntity );
	DECLARE_MAPENTITY();

	CLogicPlayMovie( void ) { }
	~CLogicPlayMovie( void ) { }

	virtual void Precache( void );
	virtual void Spawn( void );
	
private:

	void		InputPlayMovie( inputdata_t &&inputdata );
	void		InputMovieFinished( inputdata_t &&inputdata );

	string_t	m_strMovieFilename;
	bool		m_bAllowUserSkip;

	COutputEvent	m_OnPlaybackFinished;
};

LINK_ENTITY_TO_CLASS( logic_playmovie, CLogicPlayMovie );

BEGIN_MAPENTITY( CLogicPlayMovie )

	DEFINE_KEYFIELD_AUTO( m_strMovieFilename, "MovieFilename" ),
	DEFINE_KEYFIELD_AUTO( m_bAllowUserSkip, "allowskip" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "PlayMovie", InputPlayMovie ),
	DEFINE_INPUTFUNC( FIELD_VOID, "__MovieFinished", InputMovieFinished ),

	DEFINE_OUTPUT( m_OnPlaybackFinished, "OnPlaybackFinished" ),

END_MAPENTITY()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicPlayMovie::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicPlayMovie::Spawn( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicPlayMovie::InputPlayMovie( inputdata_t &&inputdata )
{
	const char *szVideoCommand = ( m_bAllowUserSkip ) ? "playvideo_exitcommand" : "playvideo_exitcommand_nointerrupt";
	// Build the hacked string
	
	char szClientCmd[256];
	Q_snprintf( szClientCmd, sizeof(szClientCmd), 
				"%s %s ent_fire %s __MovieFinished\n", 
				szVideoCommand,
				STRING(m_strMovieFilename), 
				GetEntityNameAsCStr() );

	// Send it on
	engine->ServerCommand( szClientCmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicPlayMovie::InputMovieFinished( inputdata_t &&inputdata )
{
	// Simply fire our output
	m_OnPlaybackFinished.FireOutput( this, this );
}
