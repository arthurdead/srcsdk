#include "gametrace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_TRACE, "Trace Server" );
#elif defined CLIENT_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_TRACE, "Trace Client" );
#endif
