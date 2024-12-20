//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENT_RENDER_HANDLE_H
#define CLIENT_RENDER_HANDLE_H
#pragma once


//-----------------------------------------------------------------------------
// Foward declarations
//-----------------------------------------------------------------------------
class IClientRenderable;


//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------
enum ClientRenderHandle_t : unsigned short;
inline const ClientRenderHandle_t INVALID_CLIENT_RENDER_HANDLE = (ClientRenderHandle_t)0xffff;


#endif // CLIENT_RENDER_HANDLE_H
