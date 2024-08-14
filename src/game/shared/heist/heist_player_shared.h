#ifndef HEIST_PLAYER_SHARED_H
#define HEIST_PLAYER_SHARED_H

#pragma once

enum HeistPlayerState
{
	STATE_ACTIVE=0,
	STATE_OBSERVER_MODE,

	NUM_PLAYER_STATES
};

#ifdef CLIENT_DLL
class C_HeistPlayer;
	#define CHeistPlayer C_HeistPlayer
#endif

class CHeistPlayer;

#endif
