#ifndef HEIST_CLIENTMODE_H
#define HEIST_CLIENTMODE_H

#pragma once

#include "clientmode_shared.h"

class ClientModeHeistNormal : public ClientModeShared 
{
	DECLARE_CLASS(ClientModeHeistNormal, ClientModeShared);

public:
	ClientModeHeistNormal();
	~ClientModeHeistNormal() override;

	void InitViewport() override;

	float GetViewModelFOV() override;

	int GetDeathMessageStartHeight();

	void PostRenderVGui() override;
};

extern IClientMode *GetClientModeNormal();
extern ClientModeHeistNormal *GetClientModeHeistNormal();

#endif
