//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHANDLE_H
#define PHANDLE_H

#pragma once

#include <vgui/VGUI.h>

namespace vgui
{

class Panel;

//-----------------------------------------------------------------------------
// Purpose: Safe pointer class for handling Panel or derived panel classes
//-----------------------------------------------------------------------------
class PHandle
{
public:
	PHandle() : m_iPanelID(INVALID_PANEL) {} //m_iSerialNumber(0), m_pListEntry(0) {}

	Panel *Get();
	Panel *Get() const;
	void Set( Panel *pPanel );
	void Set( HPanel hPanel );
	void Set( std::nullptr_t ) { Set(INVALID_PANEL); }

	operator Panel *() const						{ return Get(); }
	Panel * operator ->() const					{ return Get(); }
	PHandle & operator = (Panel *pPanel)		{ Set(pPanel); return *this; }
	PHandle & operator = (std::nullptr_t)		{ Set(INVALID_PANEL); return *this; }

	bool operator == (Panel *pPanel) const		{ return (Get() == pPanel); }
	bool operator != (Panel *pPanel) const		{ return (Get() != pPanel); }
	operator bool () const						{ return Get() != NULL; }
	bool operator ! () const						{ return Get() == NULL; }

private:
	HPanel m_iPanelID;
};

//-----------------------------------------------------------------------------
// Purpose: Safe pointer class to just convert between VPANEL's and PHandle
//-----------------------------------------------------------------------------
class VPanelHandle
{
public:
	VPanelHandle() : m_iPanelID(INVALID_PANEL) {}

	VPANEL Get() const;
	void Set( VPANEL pPanel );
	void Set( std::nullptr_t );

	operator VPANEL () const						{ return Get(); }
	VPanelHandle &operator = (VPANEL pPanel)		{ Set(pPanel); return *this; }
	VPanelHandle &operator = (std::nullptr_t)		{ Set(INVALID_VPANEL); return *this; }

	bool operator == (VPANEL pPanel) const		{ return (Get() == pPanel); }
	bool operator != (VPANEL pPanel) const		{ return (Get() != pPanel); }
	operator bool () const						{ return Get() != INVALID_VPANEL; }
	bool operator ! () const						{ return Get() == INVALID_VPANEL; }

private:
	HPanel m_iPanelID;
};

//-----------------------------------------------------------------------------
// Purpose: DHANDLE is a templated version of PHandle
//-----------------------------------------------------------------------------
template< class PanelType >
class DHANDLE : public PHandle
{
public:
	PanelType *Get() const					{ return (PanelType *)PHandle::Get(); }
	void Set( PanelType *pPanel )	{ PHandle::Set(pPanel); }
	void Set( std::nullptr_t)		{ PHandle::Set(INVALID_PANEL); }

	operator PanelType *() const						{ return (PanelType *)PHandle::Get(); }
	PanelType * operator ->() const					{ return (PanelType *)PHandle::Get(); }
	DHANDLE & operator = (PanelType *pPanel)	{ PHandle::Set(pPanel); return *this; }
	DHANDLE & operator = (std::nullptr_t)	{ PHandle::Set(INVALID_PANEL); return *this; }
	bool operator == (PanelType *pPanel) const			{ return (PHandle::Get() == pPanel); }
	bool operator != (PanelType *pPanel) const			{ return (PHandle::Get() != pPanel); }
	operator bool () const							{ return PHandle::Get() != NULL; }
	bool operator ! () const							{ return PHandle::Get() == NULL; }
};

};

#endif // PHANDLE_H
