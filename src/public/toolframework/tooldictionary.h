#ifndef TOOLDICTIONARY_H
#define TOOLDICTIONARY_H

#pragma once

#include "appframework/IAppSystem.h"
#include "itooldictionary.h"
#include "itoolsystem.h"

class CToolDictionary : public CBaseAppSystem< IToolDictionary >
{
public:
	virtual int	GetToolCount() const;

	virtual IToolSystem	*GetTool( int index );

	virtual void *QueryInterface( const char *pInterfaceName );

	virtual void CreateTools();

public:

	void RegisterTool( IToolSystem *tool );
private:

	CUtlVector< IToolSystem	* >	m_Tools;
};

extern CToolDictionary g_ToolDictionary;

extern void RegisterTool( IToolSystem *tool );

#endif
