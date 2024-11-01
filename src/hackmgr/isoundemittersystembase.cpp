#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_SOUND, "Sound" );

HACKMGR_CLASS_API void ISoundEmitterSystemBase::AddSoundsFromFile( const char *filename, bool bPreload, bool bIsOverride )
{
	Assert(0);
}

HACKMGR_CLASS_API char const *ISoundEmitterSystemBase::GetSoundNameForHash( unsigned int hash )
{
	Assert(0);
	return NULL;
}

HACKMGR_CLASS_API unsigned int ISoundEmitterSystemBase::HashSoundName( char const *pchSndName )
{
	Assert(0);
	return -1;
}

HACKMGR_CLASS_API bool ISoundEmitterSystemBase::IsValidHash( unsigned int hash )
{
	Assert(0);
	return false;
}

HACKMGR_CLASS_API void ISoundEmitterSystemBase::DescribeSound( char const *soundname )
{
	int index = GetSoundIndex( soundname );
	if ( index == InvalidIndex() )
	{
		Log_Msg( LOG_SOUND,"SoundEmitterSystemBase::DescribeSound:  No such sound %s\n", soundname );
		return;
	}

	CSoundParametersInternal *p = InternalGetParametersForSound( index );
	if ( !p )
	{
		Log_Msg( LOG_SOUND, "SoundEmitterSystemBase::DescribeSound:  No such sound %s\n", soundname );
		return;
	}

	Log_Msg( LOG_SOUND, "\"%s\"\n{\n", GetSoundName( index ) );

	Log_Msg( LOG_SOUND, "\t\"channel\"\t\t\"%s\"\n", p->ChannelToString() );
	Log_Msg( LOG_SOUND, "\t\"volume\"\t\t\"%s\"\n", p->VolumeToString() );
	Log_Msg( LOG_SOUND, "\t\"pitch\"\t\t\t\"%s\"\n", p->PitchToString() );
	Log_Msg( LOG_SOUND, "\n" );
	Log_Msg( LOG_SOUND, "\t\"soundlevel\"\t\"%s\"\n", p->SoundLevelToString() );

	if ( p->OnlyPlayToOwner() )
	{
		Log_Msg( LOG_SOUND, "\t\"play_to_owner_only\"\t\"1\"\n" );
	}

	if ( p->GetDelayMsec() != 0 )
	{
		Log_Msg( LOG_SOUND, "\t\"delay_msec\"\t\"%i\"\n", p->GetDelayMsec() );
	}

	int totalCount = 0;

	int waveCount = p->NumSoundNames();
	int convertedCount = p->NumConvertedNames();

	totalCount = ( waveCount - 2 * convertedCount ) + convertedCount;

	if  ( totalCount > 0 )
	{
		Log_Msg( LOG_SOUND, "\n" );

		if ( waveCount == 1 )
		{
			Assert( p->GetSoundNames()[ 0 ].gender == GENDER_NONE );
			Log_Msg( LOG_SOUND, "\t\"wave\"\t\t\t\"%s\"\n", GetWaveName( p->GetSoundNames()[ 0 ].symbol ) );
		}
		else if ( convertedCount == 1 )
		{
			Assert( p->GetConvertedNames()[ 0 ].gender == GENDER_NONE );
			Log_Msg( LOG_SOUND, "\t\"wave\"\t\t\t\"%s\"\n", GetWaveName( p->GetConvertedNames()[ 0 ].symbol ) );
		}
		else
		{
			Log_Msg( LOG_SOUND, "\t\"rndwave\"\n" );
			Log_Msg( LOG_SOUND, "\t{\n" );

			int wave;
			for ( wave = 0; wave < waveCount; wave++ )
			{
				// Skip macro-expanded names
				if ( p->GetSoundNames()[ wave ].gender != GENDER_NONE )
					continue;

				Log_Msg( LOG_SOUND, "\t\t\"wave\"\t\"%s\"\n", GetWaveName( p->GetSoundNames()[ wave ].symbol ) );
			}
			for ( wave = 0; wave < convertedCount; wave++ )
			{
				Log_Msg( LOG_SOUND, "\t\t\"wave\"\t\"%s\"\n", GetWaveName( p->GetConvertedNames()[ wave ].symbol ) );
			}

			Log_Msg( LOG_SOUND, "\t}\n" );
		}
	}

	Log_Msg( LOG_SOUND, "}\n" );
}
