//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#include "cbase.h"
#include <vgui/IVGui.h>
#include "vgui/IInput.h"
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui_video.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static CUtlVector< VideoPanel * > g_vecVideoPanels;

// Thiis is a hack due to the fact that the user can type quit with the video panel up, but it's parented to the GameUI dll root panel, which is already gone so
//  we would crash in the destructor
void VGui_ClearVideoPanels()
{
	for ( int i = g_vecVideoPanels.Count() - 1; i >= 0; --i )
	{
		if ( g_vecVideoPanels[ i ] )
		{
			delete g_vecVideoPanels[ i ];
		}
	}
	g_vecVideoPanels.RemoveAll();
}

VideoPanel::VideoPanel( unsigned int nXPos, unsigned int nYPos, unsigned int nHeight, unsigned int nWidth, bool allowAlternateMedia ) : 
	BaseClass( NULL, "VideoPanel" ),
	m_VideoMaterial( NULL ),
	m_nPlaybackWidth( 0 ),
	m_nPlaybackHeight( 0 ),
	m_bAllowAlternateMedia( allowAlternateMedia ),
	m_nShutdownCount( 0 ),
	m_bLooping( false ),
	m_bStopAllSounds( true ),
	m_bAllowInterruption( true ),
	m_bStarted( false )
{
	vgui::VPANEL pParent = enginevgui->GetPanel( PANEL_ROOT );
	SetParent( pParent );
	SetVisible( false );
	
	// Must be passed in, off by default
	m_szExitCommand[0] = '\0';

	m_bBlackBackground = true;

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( false );

	SetProportional( false );
	SetVisible( true );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	
	// Set us up
	SetTall( nHeight );
	SetWide( nWidth );
	SetPos( nXPos, nYPos );

	SetScheme(vgui::scheme()->LoadSchemeFromFile( "resource/VideoPanelScheme.res", "VideoPanelScheme"));
	LoadControlSettings("resource/UI/VideoPanel.res");

	// Let us update
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	g_vecVideoPanels.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Properly shutdown out materials
//-----------------------------------------------------------------------------
VideoPanel::~VideoPanel( void )
{
	g_vecVideoPanels.FindAndRemove( this );

	SetParent( (vgui::Panel *) NULL );

	// Shut down this video, destroy the video material
	if ( g_pVideo != NULL && m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keeps a tab on when the movie is ending and allows a frame to pass to prevent threading issues
//-----------------------------------------------------------------------------
void VideoPanel::OnTick( void ) 
{ 
	if ( m_nShutdownCount > 0 )
	{
		m_nShutdownCount++;

		if ( m_nShutdownCount > 10 )
		{
			OnClose();
			m_nShutdownCount = 0;
		}
	}

	BaseClass::OnTick(); 
}

void VideoPanel::OnVideoOver()
{
	StopPlayback();
}

//-----------------------------------------------------------------------------
// Purpose: Begins playback of a movie
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel::BeginPlayback( const char *pFilename )
{
	if ( !pFilename || pFilename[ 0 ] == '\0' )
		return false;

	// need working video services
	if ( g_pVideo == NULL )
		return false;

	// Create a new video material
	if ( m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}

	m_VideoMaterial = g_pVideo->CreateVideoMaterial( "VideoMaterial", pFilename, "GAME",
													VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
													VideoSystem::DETERMINE_FROM_FILE_EXTENSION, m_bAllowAlternateMedia );
	
	if ( m_VideoMaterial == NULL )
		return false;

	m_bStarted = true;

	// We want to be the sole audio source
	if ( m_bStopAllSounds )
	{
		enginesound->NotifyBeginMoviePlayback();
	}

	int nWidth, nHeight;
	m_VideoMaterial->GetVideoImageSize( &nWidth, &nHeight );
	m_VideoMaterial->GetVideoTexCoordRange( &m_flU, &m_flV );
	m_pMaterial = m_VideoMaterial->GetMaterial();


	float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	float flVideoRatio = ( (float) nWidth / (float) nHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::Activate( void )
{
	MoveToFront();
	RequestFocus();
	SetVisible( true );
	SetEnabled( true );

	vgui::surface()->SetMinimized( GetVPanel(), false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::DoModal( void )
{
	MakePopup();
	Activate();

	vgui::input()->SetAppModalSurface( GetVPanel() );
	vgui::surface()->RestrictPaintToSinglePanel( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	bool bInterruptKeyPressed = ( code == KEY_ESCAPE );
	if ( m_bAllowInterruption && bInterruptKeyPressed )
	{
		StopPlayback();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle keys that should cause us to close
//-----------------------------------------------------------------------------
void VideoPanel::OnKeyCodePressed( vgui::KeyCode keycode )
{
	vgui::KeyCode code = GetBaseButtonCode( keycode );

	// All these keys will interrupt playback
	bool bInterruptKeyPressed =	  ( code == KEY_ESCAPE || 
									code == KEY_BACKQUOTE || 
									code == KEY_SPACE || 
									code == KEY_ENTER ||
									code == KEY_XBUTTON_A || 
									code == KEY_XBUTTON_B ||
									code == KEY_XBUTTON_X || 
									code == KEY_XBUTTON_Y || 
									code == KEY_XBUTTON_START || 
									code == KEY_XBUTTON_BACK );
	
	// These keys cause the panel to shutdown
	if ( m_bAllowInterruption && bInterruptKeyPressed )
	{
		StopPlayback();
	}
	else
	{
		BaseClass::OnKeyCodePressed( keycode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::StopPlayback( void )
{
	SetVisible( false );

	// Start the deferred shutdown process
	m_nShutdownCount = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::OnClose( void )
{
	if ( m_bStopAllSounds )
	{
		enginesound->NotifyEndMoviePlayback();
	}

	BaseClass::OnClose();

	if ( vgui::input()->GetAppModalSurface() == GetVPanel() )
	{
		vgui::input()->ReleaseAppModalSurface();
	}

	vgui::surface()->RestrictPaintToSinglePanel( vgui::INVALID_VPANEL );

	// Fire an exit command if we're asked to do so
	if ( m_szExitCommand[0] )
	{
		engine->ClientCmd( m_szExitCommand );
	}

	SetVisible( false );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::GetPanelPos( int &xpos, int &ypos )
{
	xpos = ( (float) ( GetWide() - m_nPlaybackWidth ) / 2 );
	ypos = ( (float) ( GetTall() - m_nPlaybackHeight ) / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void VideoPanel::Paint( void )
{
	BaseClass::Paint();

	if ( m_VideoMaterial == NULL )
		return;

	if ( m_VideoMaterial->Update() == false )
	{
		// Issue a close command
		OnVideoOver();
		OnClose();
	}

	// Sit in the "center"
	int xpos, ypos;
	GetPanelPos( xpos, ypos );
	LocalToScreen( xpos, ypos );

	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	if ( m_bBlackBackground )
	{
		vgui::surface()->DrawSetColor(  0, 0, 0, 255 );
		vgui::surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	}

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( m_pMaterial, NULL );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float flLeftX = xpos;
	float flRightX = xpos + (m_nPlaybackWidth-1);

	float flTopY = ypos;
	float flBottomY = ypos + (m_nPlaybackHeight-1);

	// Map our UVs to cut out just the portion of the video we're interested in
	float flLeftU = 0.0f;
	float flTopV = 0.0f;

	// We need to subtract off a pixel to make sure we don't bleed
	float flRightU = m_flU - ( 1.0f / (float) m_nPlaybackWidth );
	float flBottomV = m_flV - ( 1.0f / (float) m_nPlaybackHeight );

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	float alpha = ((float)GetFgColor().a()/255.0f);

	for ( int corner=0; corner<4; corner++ )
	{
		bool bLeft = (corner==0) || (corner==3);
		meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f );
		meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
		meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
		meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
		meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
		meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
		meshBuilder.AdvanceVertex();
	}
	
	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: Create and playback a video in a panel
// Input  : nWidth - Width of panel (in pixels) 
//			nHeight - Height of panel
//			*pVideoFilename - Name of the file to play
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel_Create( unsigned int nXPos, unsigned int nYPos, 
						unsigned int nWidth, unsigned int nHeight, 
						const char *pVideoFilename, 
						const char *pExitCommand /*= NULL*/,
						bool bAllowInterruption /*= true*/ )
{
	// Create the base video panel
	VideoPanel *pVideoPanel = new VideoPanel( nXPos, nYPos, nHeight, nWidth, false  );
	if ( pVideoPanel == NULL )
		return false;

	// Toggle if we want the panel to allow interruption
	pVideoPanel->SetAllowInterrupt( bAllowInterruption );

	// Set the command we'll call (if any) when the video is interrupted or completes
	pVideoPanel->SetExitCommand( pExitCommand );

	// Start it going
	if ( pVideoPanel->BeginPlayback( pVideoFilename ) == false )
	{
		pVideoPanel->MarkForDeletion();
		pVideoPanel = NULL;
		return false;
	}

	// Take control
	pVideoPanel->DoModal();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Take a raw filename and ensure it points to the correct directory and file extension
//-----------------------------------------------------------------------------
void ComposeWebmFilename( const char *lpszFilename, char *lpszOut, int nOutSize )
{
	Q_strncpy( lpszOut, "media/", nOutSize );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( lpszFilename, strFilename, MAX_PATH );
	Q_strncat( lpszOut, strFilename, nOutSize );
	Q_strncat( lpszOut, ".webm", nOutSize );		// Assume we're a .bik extension type
}

//-----------------------------------------------------------------------------
// Purpose: Create a video panel with the supplied commands
//-----------------------------------------------------------------------------
void CreateVideoPanel( const char *lpszFilename, const char *lpszExitCommand, int nWidth, int nHeight, bool bAllowInterruption )
{
	char strFullpath[MAX_PATH];
	ComposeWebmFilename( lpszFilename, strFullpath, sizeof(strFullpath) );

	// Use the full screen size if they haven't specified an override
	unsigned int nScreenWidth = ( nWidth != 0 ) ? nWidth : ScreenWidth();
	unsigned int nScreenHeight = ( nHeight != 0 ) ? nHeight : ScreenHeight();

	// Create the panel and go!
	if ( VideoPanel_Create( 0, 0, nScreenWidth, nScreenHeight, strFullpath, lpszExitCommand, bAllowInterruption ) == false )
	{
		Warning( "Unable to play video: %s\n", strFullpath );

		if(lpszExitCommand && lpszExitCommand[0] != '\0')
			engine->ClientCmd( lpszExitCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback (Debug) -
//  user must include file extension
//-----------------------------------------------------------------------------
CON_COMMAND( playvideo, "Plays a video: <filename> [width height]" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = Q_atoi( args[2] );
	unsigned int nScreenHeight = Q_atoi( args[3] );
	
	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( args[1], strFilename, MAX_PATH );
	Q_strncat( strFullpath, args[1], MAX_PATH );
	
	if ( nScreenWidth == 0 )
	{
		nScreenWidth = ScreenWidth();
	}
	
	if ( nScreenHeight == 0 )
	{
		nScreenHeight = ScreenHeight();
	}

	// Create the panel and go!
	CreateVideoPanel( strFullpath, NULL, nScreenWidth, nScreenHeight, true );
}

CON_COMMAND( playvideo_nointerrupt, "Plays a video: <filename> [width height]" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = Q_atoi( args[2] );
	unsigned int nScreenHeight = Q_atoi( args[3] );
	
	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( args[1], strFilename, MAX_PATH );
	Q_strncat( strFullpath, args[1], MAX_PATH );
	
	if ( nScreenWidth == 0 )
	{
		nScreenWidth = ScreenWidth();
	}
	
	if ( nScreenHeight == 0 )
	{
		nScreenHeight = ScreenHeight();
	}

	// Create the panel and go!
	CreateVideoPanel( strFullpath, NULL, nScreenWidth, nScreenHeight, false );
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback and fire a command on completion
//-----------------------------------------------------------------------------
CON_COMMAND( playvideo_exitcommand, "Plays a video and fires and exit command when it is stopped or finishes: <filename> <exit command>" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = ScreenWidth();
	unsigned int nScreenHeight = ScreenHeight();

	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( args[1], strFilename, MAX_PATH );
	Q_strncat( strFullpath, args[1], MAX_PATH );

	char *pExitCommand = Q_strstr( args.GetCommandString(), args[2] );

	// Create the panel and go!
	CreateVideoPanel( strFullpath, pExitCommand, nScreenWidth, nScreenHeight, true );
}

CON_COMMAND( playvideo_exitcommand_nointerrupt, "Plays a video and fires and exit command when it is stopped or finishes: <filename> <exit command>" )
{
	if ( args.ArgC() < 2 )
		return;

	unsigned int nScreenWidth = ScreenWidth();
	unsigned int nScreenHeight = ScreenHeight();

	char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension( args[1], strFilename, MAX_PATH );
	Q_strncat( strFullpath, args[1], MAX_PATH );

	char *pExitCommand = Q_strstr( args.GetCommandString(), args[2] );

	// Create the panel and go!
	CreateVideoPanel( strFullpath, pExitCommand, nScreenWidth, nScreenHeight, false );
}

//-----------------------------------------------------------------------------
// Purpose: Cause all playback to stop
//-----------------------------------------------------------------------------

CON_COMMAND( stopvideos, "Stops all videos playing to the screen" )
{
	FOR_EACH_VEC( g_vecVideoPanels, itr )
	{
		g_vecVideoPanels[itr]->StopPlayback();
	}
}