//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "tier0/vprof.h"

#include "view_scene.h"
#include "viewrender.h"
#include "viewdebug.h"
#include "smoke_fog_overlay.h"
#include "materialsystem/imaterialvar.h"

#ifdef PORTAL
//#include "C_Portal_Player.h"
#include "portal_render_targets.h"
#include "PortalRender.h"
#endif

//-----------------------------------------------------------------------------
// debugging overlays
//-----------------------------------------------------------------------------
static ConVar cl_drawmaterial( "cl_drawmaterial", "", FCVAR_CHEAT, "Draw a particular material over the frame" );
static ConVar mat_showwatertextures( "mat_showwatertextures", "0", FCVAR_CHEAT );
static ConVar mat_wateroverlaysize( "mat_wateroverlaysize", "256" );
static ConVar mat_showframebuffertexture( "mat_showframebuffertexture", "0", FCVAR_CHEAT );
static ConVar mat_framebuffercopyoverlaysize( "mat_framebuffercopyoverlaysize", "256" );
static ConVar mat_showcamerarendertarget( "mat_showcamerarendertarget", "0", FCVAR_CHEAT );
static ConVar mat_camerarendertargetoverlaysize( "mat_camerarendertargetoverlaysize", "256", FCVAR_CHEAT );
static ConVar mat_hsv( "mat_hsv", "0", FCVAR_CHEAT );
static ConVar mat_yuv( "mat_yuv", "0", FCVAR_CHEAT );
static ConVar cl_overdraw_test( "cl_overdraw_test", "0", FCVAR_CHEAT | FCVAR_NEVER_AS_STRING );
static ConVar mat_drawTexture( "mat_drawTexture", "", 0, "Enable debug view texture" );
static ConVar mat_drawTextureScale( "mat_drawTextureScale", "1.0", 0, "Debug view texture scale" );
static ConVar mat_showcamerarendertarget_all( "mat_showcamerarendertarget_all", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// debugging
//-----------------------------------------------------------------------------
// (the engine owns this cvar).
ConVar mat_wireframe( "mat_wireframe", "0", FCVAR_CHEAT );
const ConVar *sv_cheats = NULL;
ConVar	mat_showlightmappage(  "mat_showlightmappage", "-1" ); // set this to the lightmap page that you want to see on screen, set to -1 to show nothing.
ConVar cl_drawshadowtexture( "cl_drawshadowtexture", "0", FCVAR_CHEAT );
ConVar cl_shadowtextureoverlaysize( "cl_shadowtextureoverlaysize", "256", FCVAR_CHEAT );

static ConVar r_flashlightdrawdepth( "r_flashlightdrawdepth", "0" );


//-----------------------------------------------------------------------------
// Lightmap debugging mode view
//-----------------------------------------------------------------------------
class CLightmapDebugView : public CRendering3dView
{
public:
	CLightmapDebugView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	void Draw()
	{
		extern bool s_bCanAccessCurrentView;
		s_bCanAccessCurrentView = true;
		Frustum frustum;
		render->Push3DView( *this, 0, NULL, frustum );
		BuildWorldRenderLists( this, true, true );
		render->PopView( frustum );
		s_bCanAccessCurrentView = false;

		render->DrawLightmaps( m_pWorldRenderList, mat_showlightmappage.GetInt() );
	}


};


//-----------------------------------------------------------------------------
// Renders a material orthographically to screen...
//-----------------------------------------------------------------------------
static void RenderMaterial( const char *pMaterialName )
{
	// So it's not in the very top left
	float x = 100.0f, y = 100.0f;
	// float x = 0.0f, y = 0.0f;

	IMaterial *pMaterial = materials->FindMaterial( pMaterialName, TEXTURE_GROUP_OTHER, false );
	if ( !IsErrorMaterial( pMaterial ) )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( pMaterial );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( x, y, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x + pMaterial->GetMappingWidth(), y, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x + pMaterial->GetMappingWidth(), y + pMaterial->GetMappingHeight(), 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x, y + pMaterial->GetMappingHeight(), 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}

static void OverlayWaterTexture( IMaterial *pMaterial, int xOffset, int yOffset, bool bFlip )
{
	// screen safe
	float xBaseOffset = 0;
	float yBaseOffset = 0;
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	float fFlip0 = bFlip ? 1.0f : 0.0f;
	float fFlip1 = bFlip ? 0.0f : 1.0f;

	if( !IsErrorMaterial( pMaterial ) )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( pMaterial );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		float w = mat_wateroverlaysize.GetFloat();
		float h = mat_wateroverlaysize.GetFloat();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( xBaseOffset + xOffset * w, yBaseOffset + yOffset * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, fFlip1 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + ( xOffset + 1 ) * w, yBaseOffset + yOffset * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, fFlip1 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + ( xOffset + 1 ) * w, yBaseOffset + ( yOffset + 1 ) * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, fFlip0 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + xOffset * w, yBaseOffset + ( yOffset + 1 ) * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, fFlip0 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}

static void OverlayWaterTextures( void )
{
	OverlayWaterTexture( materials->FindMaterial( "debug/debugreflect", NULL ), 0, 0, false );
	OverlayWaterTexture( materials->FindMaterial( "debug/debugrefract", NULL ), 0, 1, true );
}

void OverlayCameraRenderTarget( const char *pszMaterialName, float flX, float flY, float w, float h )
{
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	IMaterial *pMaterial;
	pMaterial = materials->FindMaterial( pszMaterialName, TEXTURE_GROUP_OTHER, true );
	if( !IsErrorMaterial( pMaterial ) )
	{
		// HACKHACK
		pMaterial->IncrementReferenceCount();

		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( pMaterial );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( flX, flY, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX+w, flY, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX+w, flY+h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX, flY+h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();

		// HACKHACK
		pMaterial->DecrementReferenceCount();
	}
}


static void OverlayFrameBufferTexture( int nFrameBufferIndex )
{
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	IMaterial *pMaterial;
	char buf[MAX_PATH];
	Q_snprintf( buf, MAX_PATH, "debug/debugfbtexture%d", nFrameBufferIndex );
	pMaterial = materials->FindMaterial( buf, NULL, true );
	if( !IsErrorMaterial( pMaterial ) )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( pMaterial );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		float w = mat_framebuffercopyoverlaysize.GetFloat();
		float h = mat_framebuffercopyoverlaysize.GetFloat();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( w * nFrameBufferIndex, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * ( nFrameBufferIndex + 1 ), 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * ( nFrameBufferIndex + 1 ), h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * nFrameBufferIndex, h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}


//-----------------------------------------------------------------------------
// Debugging aid to display a texture
//-----------------------------------------------------------------------------
static void OverlayShowTexture( const char* textureName, float scale )
{
	bool			foundVar;
	IMaterial		*pMaterial;
	IMaterialVar	*BaseTextureVar;
	ITexture		*pTex;
	float			x, y, w, h;

	// screen safe
	x = 32;
	y = 32;

	pMaterial = materials->FindMaterial( "___debug", TEXTURE_GROUP_OTHER, true );
	BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	if (!foundVar)
		return;

	CMatRenderContextPtr pRenderContext( materials );

	if ( textureName && textureName[0] )
	{
		pTex = materials->FindTexture( textureName, TEXTURE_GROUP_OTHER, false );
		BaseTextureVar->SetTextureValue( pTex );

		w = pTex->GetActualWidth() * scale;
		h = pTex->GetActualHeight() * scale;
	}
	else
	{
		w = h = 64.0f * scale;
	}

	pRenderContext->Bind( pMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );
	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x+w, y, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x+w, y+h, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x, y+h, 0.0f );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Draws all the debugging info
//-----------------------------------------------------------------------------
void CDebugViewRender::Draw3DDebuggingInfo( const CViewSetup &view )
{
	VPROF("CViewRender::Draw3DDebuggingInfo");

	// Draw 3d overlays
	render->Draw3DDebugOverlays();

	// Draw the line file used for debugging leaks
	render->DrawLineFile();
}


//-----------------------------------------------------------------------------
// Draws all the debugging info
//-----------------------------------------------------------------------------
void CDebugViewRender::Draw2DDebuggingInfo( const CViewSetup &view )
{
	// HDRFIXME: Assert NULL rendertarget
	if ( mat_yuv.GetInt() && (engine->GetDXSupportLevel() >= 80) )
	{
		IMaterial *pMaterial;
		pMaterial = materials->FindMaterial( "debug/yuv", TEXTURE_GROUP_OTHER, true );
		if( !IsErrorMaterial( pMaterial ) )
		{
			pMaterial->IncrementReferenceCount();
			DrawScreenEffectMaterial( pMaterial, view.x, view.y, view.width, view.height );
			pMaterial->DecrementReferenceCount();
		}
	}

	if ( mat_hsv.GetInt() && (engine->GetDXSupportLevel() >= 90) )
	{
		IMaterial *pMaterial;
		pMaterial = materials->FindMaterial( "debug/hsv", TEXTURE_GROUP_OTHER, true );
		if( !IsErrorMaterial( pMaterial ) )
		{
			pMaterial->IncrementReferenceCount();
			DrawScreenEffectMaterial( pMaterial, view.x, view.y, view.width, view.height );
			pMaterial->DecrementReferenceCount();
		}
	}

	// Draw debugging lightmaps
	if ( mat_showlightmappage.GetInt() != -1 )
	{
		CLightmapDebugView clientView( assert_cast<CViewRender *>( GetViewRenderInstance() ) );
		clientView.Setup( view );
		clientView.Draw();
	}

	if ( cl_drawshadowtexture.GetInt() )
	{
		int nSize = cl_shadowtextureoverlaysize.GetInt();
		g_pClientShadowMgr->RenderShadowTexture( nSize, nSize );
	}

	const char *pDrawMaterial = cl_drawmaterial.GetString();
	if ( pDrawMaterial && pDrawMaterial[0] )
	{
		RenderMaterial( pDrawMaterial ); 
	}

	if ( mat_showwatertextures.GetBool() )
	{
		OverlayWaterTextures();
	}

	if ( mat_showcamerarendertarget.GetBool() )
	{
		float w = mat_camerarendertargetoverlaysize.GetFloat();
		float h = mat_camerarendertargetoverlaysize.GetFloat();
#ifdef PORTAL
		g_pPortalRender->OverlayPortalRenderTargets( w, h );
#else
		int iCameraNum = mat_showcamerarendertarget.GetInt();

		if (iCameraNum == 1) // Display the default camera
		{
			OverlayCameraRenderTarget( "debug/debugcamerarendertarget", 0, 0, w, h );
		}
		else if (mat_showcamerarendertarget_all.GetBool()) // Display all cameras
		{
			OverlayCameraRenderTarget( "debug/debugcamerarendertarget", 0, 0, w, h );

			// Already showed one camera
			iCameraNum--;

			// Show Mapbase's cameras
			char szTextureName[48];
			for (int i = 0; i < iCameraNum; i++)
			{
				V_snprintf( szTextureName, sizeof( szTextureName ), "debug/debugcamerarendertarget_camera%i", i );

				// Show them vertically if the cvar is set to 2
				if (mat_showcamerarendertarget_all.GetInt() == 2)
					OverlayCameraRenderTarget( szTextureName, 0, h * (i + 1), w, h );
				else
					OverlayCameraRenderTarget( szTextureName, w * (i + 1), 0, w, h );
			}
		}
		else // Display one of the new cameras
		{
			OverlayCameraRenderTarget( VarArgs( "debug/debugcamerarendertarget_camera%i", iCameraNum-2 ), 0, 0, w, h );
		}
#endif
	}

	if ( mat_showframebuffertexture.GetBool() )
	{
		// HDRFIXME: Get rid of these rendertarget sets assuming that the assert at the top of this function is true.
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->PushRenderTargetAndViewport( NULL );
		OverlayFrameBufferTexture( 0 );
		OverlayFrameBufferTexture( 1 );
		pRenderContext->PopRenderTargetAndViewport( );
	}

	const char *pDrawTexture = mat_drawTexture.GetString();
	if ( pDrawTexture && pDrawTexture[0] )
	{		
		OverlayShowTexture( pDrawTexture, mat_drawTextureScale.GetFloat() );
	}

	if ( r_flashlightdrawdepth.GetBool() )
	{
		shadowmgr->DrawFlashlightDepthTexture( );
	}
}

//-----------------------------------------------------------------------------
// A console command allowing you to draw a material as an overlay
//-----------------------------------------------------------------------------
CON_COMMAND_F( r_screenoverlay, "Draw specified material as an overlay", FCVAR_CHEAT|FCVAR_SERVER_CAN_EXECUTE )
{
	if( args.ArgC() == 2 )
	{
		if ( !Q_stricmp( "off", args[1] ) )
		{
			GetViewRenderInstance()->SetScreenOverlayMaterial( NULL );
		}
		else
		{
			IMaterial *pMaterial = materials->FindMaterial( args[1], TEXTURE_GROUP_OTHER, false );
			if ( !IsErrorMaterial( pMaterial ) )
			{
				GetViewRenderInstance()->SetScreenOverlayMaterial( pMaterial );
			}
			else
			{
				GetViewRenderInstance()->SetScreenOverlayMaterial( NULL );
			}
		}
	}
	else
	{
		IMaterial *pMaterial = GetViewRenderInstance()->GetScreenOverlayMaterial();
		Warning( "r_screenoverlay: %s\n", pMaterial ? pMaterial->GetName() : "off" );
	}
}

//-----------------------------------------------------------------------------
// The same as above, but using the new indexed overlays
//-----------------------------------------------------------------------------
CON_COMMAND_F( r_screenoverlay_indexed, "Draw specified material as an overlay in the specified index", FCVAR_CHEAT|FCVAR_SERVER_CAN_EXECUTE )
{
	if( args.ArgC() == 3 )
	{
		int index = atoi( args[1] );
		if (index < 0 || index >= MAX_SCREEN_OVERLAYS)
		{
			Warning( "r_screenoverlay_indexed: '%i' is out of range (should be 0-9)\n", index );
			return;
		}

		if ( !Q_stricmp( "off", args[2] ) )
		{
			GetViewRenderInstance()->SetIndexedScreenOverlayMaterial( index, NULL );
		}
		else
		{
			IMaterial *pMaterial = materials->FindMaterial( args[2], TEXTURE_GROUP_OTHER, false );
			if ( !IsErrorMaterial( pMaterial ) )
			{
				GetViewRenderInstance()->SetIndexedScreenOverlayMaterial( index, pMaterial );
			}
			else
			{
				GetViewRenderInstance()->SetIndexedScreenOverlayMaterial( index, NULL );
			}
		}
	}
	else if ( args.ArgC() == 2 )
	{
		int index = atoi( args[1] );
		if (index < 0 || index >= MAX_SCREEN_OVERLAYS)
		{
			Warning( "r_screenoverlay_indexed: '%i' is out of range (should be 0-9)\n", index );
			return;
		}

		IMaterial *pMaterial = GetViewRenderInstance()->GetIndexedScreenOverlayMaterial( index );
		Warning( "r_screenoverlay_indexed %i: %s\n", index, pMaterial ? pMaterial->GetName() : "off" );
	}
	else
	{
		Warning( "Format: r_screenoverlay_indexed <index> [<material>]\n" );
	}
}

// Used to verify frame syncing.
void CDebugViewRender::GenerateOverdrawForTesting()
{
	if ( !cl_overdraw_test.GetInt() )
		return;

	for ( int i=0; i < 40; i++ )
	{
		g_SmokeFogOverlayAlpha = 20 / 255.0;
		DrawSmokeFogOverlay();
	}
	g_SmokeFogOverlayAlpha = 0;
}


