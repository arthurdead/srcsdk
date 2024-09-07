#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API void ISoundEmitterSystemBase::AddSoundsFromFile( const char *filename, bool bPreload, bool bIsOverride )
{

}

HACKMGR_CLASS_API char const *ISoundEmitterSystemBase::GetSoundNameForHash( unsigned int hash )
{
	return NULL;
}

HACKMGR_CLASS_API unsigned int ISoundEmitterSystemBase::HashSoundName( char const *pchSndName )
{
	return -1;
}

HACKMGR_CLASS_API bool ISoundEmitterSystemBase::IsValidHash( unsigned int hash )
{
	return false;
}

HACKMGR_CLASS_API void ISoundEmitterSystemBase::DescribeSound( char const *soundname )
{

}
