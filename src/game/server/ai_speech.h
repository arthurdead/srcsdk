//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SPEECH_H
#define AI_SPEECH_H
#pragma once

#include "utlmap.h"

#include "soundflags.h"
#include "ai_responsesystem.h"
#include "utldict.h"
#include "util.h"
#include "ai_speechconcept.h"
#include "ai_speechqueue.h"

class KeyValues;
class CBaseFlex;
class CAI_BaseNPC;
namespace ResponseRules
{
	class CriteriaSet;
}

using ResponseRules::ResponseType_t;
using ResponseRules::AI_ResponseFollowup;

//-----------------------------------------------------------------------------
// Purpose: Used to share a global resource or prevent a system stepping on
//			own toes.
//-----------------------------------------------------------------------------

class CAI_TimedSemaphore
{
public:
	CAI_TimedSemaphore()
	 :	m_ReleaseTime( 0 )
	{
		m_hCurrentTalker = NULL;
	}
	
	void Acquire( float time, CBaseEntity *pTalker )		{ m_ReleaseTime = gpGlobals->curtime + time; m_hCurrentTalker = pTalker; }
	void Release()					{ m_ReleaseTime = 0; m_hCurrentTalker = NULL; }
	
	// Current owner of the semaphore is always allowed to talk
	bool IsAvailable( CBaseEntity *pTalker ) const		{ return ((gpGlobals->curtime > m_ReleaseTime) || (m_hCurrentTalker.Get() == pTalker)); }
	float GetReleaseTime() const 	{ return m_ReleaseTime; }

	CBaseEntity *GetOwner()	{ return m_hCurrentTalker.Get(); }

private:
	float		m_ReleaseTime;
	EHANDLE		m_hCurrentTalker;
};

//-----------------------------------------------------------------------------

extern CAI_TimedSemaphore g_AIFriendliesTalkSemaphore;
extern CAI_TimedSemaphore g_AIFoesTalkSemaphore;

#define GetSpeechSemaphore( pNpc ) (((pNpc)->IsPlayerAlly()) ? &g_AIFriendliesTalkSemaphore : &g_AIFoesTalkSemaphore )
//-----------------------------------------------------------------------------
// Basic speech system types
//-----------------------------------------------------------------------------

//-------------------------------------
// Constants


const float AIS_DEF_MIN_DELAY 	= 2.8; // Minimum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_DEF_MAX_DELAY 	= 3.2; // Maximum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_NO_DELAY  		= 0;
const soundlevel_t AIS_DEF_SNDLVL 	 	= SNDLVL_TALKING;
#define AI_NULL_CONCEPT NULL

#define AI_NULL_SENTENCE NULL

// Sentence prefix constants
#define AI_SP_SPECIFIC_SENTENCE	'!'
#define AI_SP_WAVFILE			'^'
#define AI_SP_SCENE_GROUP		'='
#define AI_SP_SPECIFIC_SCENE	'?'

#define AI_SPECIFIC_SENTENCE(str_constant)	"!" str_constant
#define AI_WAVFILE(str_constant)			"^" str_constant
// @Note (toml 09-12-02): as scene groups are not currently implemented, the string is a semi-colon delimited list
#define AI_SCENE_GROUP(str_constant)		"=" str_constant
#define AI_SPECIFIC_SCENE(str_constant)		"?" str_constant

// Designer overriding modifiers
#define AI_SPECIFIC_SCENE_MODIFIER "scene:"

//-------------------------------------

//-------------------------------------
// An id that represents the core meaning of a spoken phrase, 
// eventually to be mapped to a sentence group or scene

#if AI_CONCEPTS_ARE_STRINGS
typedef const char *AIConcept_t;
inline bool CompareConcepts( AIConcept_t c1, AIConcept_t c2 ) 
{
	return ( (void *)c1 == (void *)c2 || ( c1 && c2 && Q_stricmp( c1, c2 ) == 0 ) );
}
#else
typedef CAI_Concept AIConcept_t;
inline bool CompareConcepts( AIConcept_t c1, AIConcept_t c2 ) 
{
	return c1.m_iConcept == c2.m_iConcept;
}
#endif

//-------------------------------------
// Specifies and stores the base timing and attentuation values for concepts
//
namespace ResponseRules
{
	class CRR_Response;
}

//-----------------------------------------------------------------------------
// CAI_Expresser
//
// Purpose: Provides the functionality of going from abstract concept ("hello")
//			to specific sentence/scene/wave
//

//-------------------------------------
// Sink supports behavior control and receives notifications of internal events

class CAI_ExpresserSink
{
public:
	virtual void OnSpokeConcept( AIConcept_t ai_concept, AI_Response *response )	{};
	virtual void OnStartSpeaking()						{}
	virtual bool UseSemaphore()							{ return true; }
	// Works around issues with CAI_ExpresserHost<> class hierarchy
	virtual CAI_Expresser *GetSinkExpresser() { return NULL; }
	virtual bool IsAllowedToSpeakFollowup( AIConcept_t conc, CBaseEntity *pIssuer, bool bSpecific ) { return true; }
	virtual bool Speak( AIConcept_t conc, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL ) { return false; }
};

struct ConceptHistory_t
{
	ConceptHistory_t(float timeSpoken = -1 )
	 : timeSpoken( timeSpoken ), m_response( )
	{
	}

	ConceptHistory_t( const ConceptHistory_t& src );
	ConceptHistory_t& operator = ( const ConceptHistory_t& src );

	~ConceptHistory_t();

	float		timeSpoken;
	AI_Response m_response;
};
//-------------------------------------

class CAI_Expresser : public ResponseRules::IResponseFilter
{
public:
	CAI_Expresser( CBaseFlex *pOuter = NULL );
	~CAI_Expresser();

	// --------------------------------
	
	bool Connect( CAI_ExpresserSink *pSink )		{ m_pSink = pSink; return true; }
	bool Disconnect( CAI_ExpresserSink *pSink )	{ m_pSink = NULL; return true;}

	void TestAllResponses();

	// --------------------------------
	
	bool Speak( AIConcept_t &ai_concept, const char *modifiers, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	bool Speak( AIConcept_t &ai_concept, AI_CriteriaSet *criteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	bool Speak( AIConcept_t &ai_concept )
	{ return Speak( ai_concept, static_cast<AI_CriteriaSet *>(NULL), NULL, 0, NULL ); }

	// These two methods allow looking up a response and dispatching it to be two different steps
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept, const char *modifiers );
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept, AI_CriteriaSet *modifiers );
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept )
	{ return FindResponse(outResponse, ai_concept, static_cast<AI_CriteriaSet *>(NULL)); }

	bool SpeakDispatchResponse( AIConcept_t &ai_concept, AI_Response *response, IRecipientFilter *filter )
	{ return SpeakDispatchResponse(ai_concept, response, NULL, filter); }

	virtual bool SpeakDispatchResponse( AIConcept_t &ai_concept, AI_Response *response, AI_CriteriaSet *criteria, IRecipientFilter *filter );

	bool SpeakDispatchResponse( AIConcept_t &ai_concept, AI_Response *response )
	{ return SpeakDispatchResponse(ai_concept, response, static_cast<IRecipientFilter *>(NULL)); }

	bool SpeakDispatchResponse( AIConcept_t &ai_concept, AI_Response *response, AI_CriteriaSet *criteria )
	{ return SpeakDispatchResponse(ai_concept, response, criteria, NULL); }

	// Given modifiers (which are colon-delimited strings), fill out a criteria set including this 
	// character's contexts and the ones in the modifier. This lets us hang on to them after a call
	// to SpeakFindResponse.
	void GatherCriteria( AI_CriteriaSet *outputCritera, const AIConcept_t &ai_concept, const char *modifiers );
	float GetResponseDuration( AI_Response *response );

	void SetUsingProspectiveResponses( bool bToggle );
	void MarkResponseAsUsed( AI_Response *response );

	virtual void OnSpeechFinished() {};

	// This function can be overriden by games to suppress speech altogether during glue screens, etc
	static bool IsSpeechGloballySuppressed();

	virtual int SpeakRawSentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	
	bool SemaphoreIsAvailable( CBaseEntity *pTalker );
	float GetSemaphoreAvailableTime( CBaseEntity *pTalker );

	// --------------------------------
	
	virtual bool IsSpeaking();
	bool CanSpeak();
	bool CanSpeakAfterMyself();
	float GetTimeSpeechComplete() const 	{ return m_flStopTalkTime; }
	float GetTimeSpeechCompleteWithoutDelay() const	{ return m_flStopTalkTimeWithoutDelay; }
	void  BlockSpeechUntil( float time );

	// --------------------------------
	
	bool CanSpeakConcept( AIConcept_t ai_concept );
	bool SpokeConcept( AIConcept_t ai_concept );
	float GetTimeSpokeConcept( AIConcept_t ai_concept ); // returns -1 if never
	void SetSpokeConcept( AIConcept_t ai_concept, AI_Response *response, bool bCallback = true );
	void ClearSpokeConcept( AIConcept_t ai_concept );

	AIConcept_t GetLastSpokeConcept( AIConcept_t excludeConcept = NULL );
	
	// --------------------------------
	
	void SetVoicePitch( int voicePitch )	{ m_voicePitch = voicePitch; }
	int GetVoicePitch() const;

	void NoteSpeaking( float duration, float delay = 0 );

	// Force the NPC to release the semaphore & clear next speech time
	void ForceNotSpeaking( void );

	// helper used in dealing with RESPONSE_ENTITYIO
	// response is the output of AI_Response::GetName
	// note: the response string will get stomped on (by strtok)
	// returns false on failure (eg, couldn't match parse contents)
	static bool FireEntIOFromResponse( char *response, CBaseEntity *pInitiator ); 

	void AllowMultipleScenes();
	void DisallowMultipleScenes();

	CAI_TimedSemaphore *GetMySpeechSemaphore( CBaseEntity *pNpc );

protected:

	bool SpeakRawScene( const char *pszScene, float delay, AI_Response *response, IRecipientFilter *filter = NULL );
	// This will create a fake .vcd/CChoreoScene to wrap the sound to be played
	bool SpeakAutoGeneratedScene( char const *soundname, float delay, AI_Response *response = NULL, IRecipientFilter *filter = NULL );

	void DumpHistories();

	void SpeechMsg( CBaseEntity *pFlex, PRINTF_FORMAT_STRING const char *pszFormat, ... );

	// --------------------------------
	
	CAI_ExpresserSink *GetSink() { return m_pSink; }

private:
	// --------------------------------

	virtual bool IsValidResponse( ResponseType_t type, const char *pszValue );

	// --------------------------------
	
	CAI_ExpresserSink *m_pSink;
	
	// --------------------------------
	//
	// Speech concept data structures
	//

	CUtlDict< ConceptHistory_t, int > m_ConceptHistories;
	
	// --------------------------------
	//
	// Speaking states
	//

	float				m_flStopTalkTime;				// when in the future that I'll be done saying this sentence.
	float				m_flStopTalkTimeWithoutDelay;	// same as the above, but minus the delay before other people can speak
	float				m_flBlockedTalkTime;
	int					m_voicePitch;					// pitch of voice for this head
	float				m_flLastTimeAcceptedSpeak;		// because speech may not be blocked until NoteSpeaking called by scene ent, this handles in-think blocking
	
	// --------------------------------
	//
public:
	virtual void SetOuter( CBaseFlex *pOuter );

	CBaseFlex *		GetOuter() 			{ return m_pOuter.Get(); }
	const CBaseFlex *	GetOuter() const 	{ return m_pOuter.Get(); }

private:
	CHandle<CBaseFlex>	m_pOuter;

	bool m_bAllowMultipleScenes;
};

//-----------------------------------------------------------------------------
//
// An NPC base class to assist a branch of the inheritance graph
// in utilizing CAI_Expresser
//

template <class BASE_NPC>
class CAI_ExpresserHost : public BASE_NPC, protected CAI_ExpresserSink
{
public:
	DECLARE_CLASS_NOFRIEND( CAI_ExpresserHost, BASE_NPC );

	CAI_Expresser *GetSinkExpresser() { return this->GetExpresser(); }

	virtual void	NoteSpeaking( float duration, float delay );

	bool 	Speak( AIConcept_t ai_concept, const char *modifiers, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	virtual bool 	Speak( AIConcept_t ai_concept, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	bool 	Speak( AIConcept_t ai_concept )
	{ return Speak(ai_concept, static_cast<AI_CriteriaSet *>(NULL), NULL, 0, NULL); }

	// These two methods allow looking up a response and dispatching it to be two different steps
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept, const char *modifiers );
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept, AI_CriteriaSet *criteria );
	bool FindResponse( AI_Response &outResponse, AIConcept_t &ai_concept );

	bool SpeakDispatchResponse( AIConcept_t ai_concept, AI_Response *response );
	bool SpeakDispatchResponse( AIConcept_t ai_concept, AI_Response *response,  AI_CriteriaSet *criteria );

	virtual void	PostSpeakDispatchResponse( AIConcept_t ai_concept, AI_Response *response ) { return; }

	void GatherCriteria( AI_CriteriaSet *outputCritera, const AIConcept_t &conc, const char *modifiers );
	float 			GetResponseDuration( AI_Response *response );

	float GetTimeSpeechComplete() const 	{ return this->GetExpresser()->GetTimeSpeechComplete(); }

	bool IsSpeaking()				{ return this->GetExpresser()->IsSpeaking(); }
	bool CanSpeak()					{ return this->GetExpresser()->CanSpeak(); }
	bool CanSpeakAfterMyself()		{ return this->GetExpresser()->CanSpeakAfterMyself(); }

	void SetSpokeConcept( AIConcept_t ai_concept, AI_Response *response, bool bCallback = true ) 		{ this->GetExpresser()->SetSpokeConcept( ai_concept, response, bCallback ); }
	float GetTimeSpokeConcept( AIConcept_t ai_concept )												{ return this->GetExpresser()->GetTimeSpokeConcept( ai_concept ); }
	bool SpokeConcept( AIConcept_t ai_concept )														{ return this->GetExpresser()->SpokeConcept( ai_concept ); }

protected:
	int 			PlaySentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& set );

	virtual ResponseRules::IResponseSystem *GetResponseSystem();
	// Override of base entity response input handler
	virtual void	DispatchResponse( const char *conceptName );
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::NoteSpeaking( float duration, float delay )
{ 
	this->GetExpresser()->NoteSpeaking( duration, delay ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::Speak( AIConcept_t ai_concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /*=NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ )
{
	AssertOnce( this->GetExpresser()->GetOuter() == this );
	return this->GetExpresser()->Speak( ai_concept, modifiers, pszOutResponseChosen, bufsize, filter );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::Speak( AIConcept_t conc, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen /*=NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ ) 
{
	AssertOnce( this->GetExpresser()->GetOuter() == this );
	CAI_Expresser * const RESTRICT pExpresser = this->GetExpresser();
	conc.SetSpeaker(this);
	// add in any local criteria to the one passed on the command line.
	pExpresser->GatherCriteria( pCriteria, conc, NULL );
	// call the "I have aleady gathered criteria" version of Expresser::Speak
	return pExpresser->Speak( conc, pCriteria, pszOutResponseChosen, bufsize, filter ); 
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline int CAI_ExpresserHost<BASE_NPC>::PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	return this->GetExpresser()->SpeakRawSentence( pszSentence, delay, volume, soundlevel, pListener );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
extern void CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( CAI_BaseNPC *pSpeaker, AI_CriteriaSet& criteriaSet );

template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	if ( this->MyNPCPointer() )
	{
		CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( this->MyNPCPointer(), criteriaSet );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
extern ResponseRules::IResponseSystem *g_pResponseSystem;
template <class BASE_NPC>
inline ResponseRules::IResponseSystem *CAI_ExpresserHost<BASE_NPC>::GetResponseSystem()
{
	// Expressive NPC's use the general response system
	return g_pResponseSystem;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::GatherCriteria( AI_CriteriaSet *outputCriteria, const AIConcept_t &conc, const char *modifiers )
{
	return this->GetExpresser()->GatherCriteria( outputCriteria, conc, modifiers );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::FindResponse( AI_Response& response, AIConcept_t &ai_concept, const char *modifiers /*= NULL*/ )
{
	return this->GetExpresser()->FindResponse( response, ai_concept, modifiers );
}

template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::FindResponse( AI_Response& response, AIConcept_t &ai_concept, AI_CriteriaSet *modifiers /*= NULL*/ )
{
	return this->GetExpresser()->FindResponse( response, ai_concept, modifiers );
}

template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::FindResponse( AI_Response& response, AIConcept_t &ai_concept )
{
	return this->GetExpresser()->FindResponse( response, ai_concept );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::SpeakDispatchResponse( AIConcept_t ai_concept, AI_Response *response )
{
	if ( this->GetExpresser()->SpeakDispatchResponse( ai_concept, response ) )
	{
		PostSpeakDispatchResponse( ai_concept, response );
		return true;
	}

	return false;
}

template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::SpeakDispatchResponse( AIConcept_t ai_concept, AI_Response *response, AI_CriteriaSet *criteria )
{
	if ( this->GetExpresser()->SpeakDispatchResponse( ai_concept, response, criteria ) )
	{
		PostSpeakDispatchResponse( ai_concept, response );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline float CAI_ExpresserHost<BASE_NPC>::GetResponseDuration( AI_Response *response )
{
	return this->GetExpresser()->GetResponseDuration( response );
}

//-----------------------------------------------------------------------------
// Override of base entity response input handler
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::DispatchResponse( const char *conceptName )
	{
		Speak( (AIConcept_t)conceptName );
	}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

/// A shim under CAI_ExpresserHost you can use when deriving a new expresser
/// host type under CAI_BaseNPC. This does the extra step of declaring an m_pExpresser
/// member and initializing it from CreateComponents(). If your BASE_NPC class isn't
/// actually an NPC, then CreateComponents() never gets called and you won't have 
/// an expresser created.
/// Note: you still need to add m_pExpresser to the Datadesc for your derived type.
/// This is because I couldn't figure out how to make a templatized datadesc declaration
/// that works generically on the template type.
template <class BASE_NPC, class EXPRESSER_TYPE>
class CAI_ExpresserHostWithData : public CAI_ExpresserHost<BASE_NPC>
{
	DECLARE_CLASS_NOFRIEND( CAI_ExpresserHostWithData, CAI_ExpresserHost<BASE_NPC> );

public:
	CAI_ExpresserHostWithData( ) : m_pExpresser(NULL) {};

	virtual CAI_Expresser *GetExpresser() { return m_pExpresser; }
	const CAI_Expresser *GetExpresser() const { return m_pExpresser; }

	virtual bool 			CreateComponents() 
	{ 
		return BaseClass::CreateComponents() &&	( CreateExpresser() != NULL );
	}

protected:
	EXPRESSER_TYPE *CreateExpresser( void )
	{
		AssertMsg1( m_pExpresser == NULL, "Tried to double-initialize expresser in %s\n", this->GetDebugName()  );
		m_pExpresser = new EXPRESSER_TYPE(this);
		if ( !m_pExpresser)
		{
			AssertMsg1( false, "Creating an expresser failed in %s\n", this->GetDebugName() );
			return NULL;
		}

		m_pExpresser->Connect(this);
		return m_pExpresser;
	}

	virtual ~CAI_ExpresserHostWithData( void )
	{
		delete m_pExpresser; 
		m_pExpresser = NULL;
	}

	EXPRESSER_TYPE *m_pExpresser;
};

/// response rules
namespace RR
{
	/// some applycontext clauses have operators preceding them,
	/// like ++1 which means "take the current value and increment it
	/// by one". These classes detect these cases and do the appropriate
	/// thing.
	class CApplyContextOperator
	{
	public:
		inline CApplyContextOperator( int nSkipChars ) : m_nSkipChars(nSkipChars) {};

		/// perform whatever this operator does upon the given context value. 
		/// Default op is simply to copy old to new.
		/// pOldValue should be the currently set value of the context. May be NULL meaning no prior value.
		/// pOperator the value that applycontext says to set
		/// pNewValue a pointer to a buffer where the real new value will be writ.
		/// returns true on success; false on failure (eg, tried to increment a 
		/// non-numeric value). 
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );

		/// This is the function that should be called from outside, 
		/// fed the input string, it'll select the right operator 
		/// to apply.
		static CApplyContextOperator *FindOperator( const char *pContextString );
	
	protected:
		int m_nSkipChars; // how many chars to "skip" in the value string to get past the op specifier to the actual value
						  // eg, "++3" has a m_nSkipChars of 2, because the op string "++" is two characters.
	};

	class CIncrementOperator : public CApplyContextOperator
	{
	public:
		inline CIncrementOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CDecrementOperator : public CApplyContextOperator
	{
	public:
		inline CDecrementOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CMultiplyOperator : public CApplyContextOperator
	{
	public:
		inline CMultiplyOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CDivideOperator : public CApplyContextOperator
	{
	public:
		inline CDivideOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CToggleOperator : public CApplyContextOperator
	{
	public:
		inline CToggleOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	// the singleton operators
	extern CApplyContextOperator sm_OpCopy;
	extern CIncrementOperator sm_OpIncrement;
	extern CDecrementOperator sm_OpDecrement;
	extern CMultiplyOperator sm_OpMultiply;
	extern CDivideOperator sm_OpDivide;
	extern CToggleOperator	  sm_OpToggle;

	// LEGACY - See CApplyContextOperator::FindOperator()
	extern CIncrementOperator sm_OpLegacyIncrement;
	extern CDecrementOperator sm_OpLegacyDecrement;
	extern CMultiplyOperator sm_OpLegacyMultiply;
	extern CDivideOperator sm_OpLegacyDivide;
};

//-----------------------------------------------------------------------------
// A kind of AI Expresser that can dispatch a follow-up speech event when it
// finishes speaking.
//-----------------------------------------------------------------------------
class CAI_ExpresserWithFollowup : public CAI_Expresser
{
public:
	CAI_ExpresserWithFollowup( CBaseFlex *pOuter = NULL ) : CAI_Expresser(pOuter),
		m_pPostponedFollowup(NULL) 
		{};
	virtual bool Speak( AIConcept_t &conc, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	virtual bool SpeakDispatchResponse( AIConcept_t &conc, AI_Response *response,  AI_CriteriaSet *criteria, IRecipientFilter *filter = NULL );
	virtual void SpeakDispatchFollowup( AI_ResponseFollowup &followup );

	virtual void OnSpeechFinished();

	typedef CAI_Expresser BaseClass;
protected:
	static void DispatchFollowupThroughQueue( const AIConcept_t &conc,
		const char *criteriaStr,
		const CResponseQueue::CFollowupTargetSpec_t &target,
		float delay,
		CBaseEntity * RESTRICT pOuter		);

	AI_ResponseFollowup *m_pPostponedFollowup; // TODO: save/restore
	CResponseQueue::CFollowupTargetSpec_t	m_followupTarget;
};

#endif // AI_SPEECH_H

