//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SIMTIMER_H
#define SIMTIMER_H

#pragma once

#include "datamap.h"
#include "sharedInterface.h"
#ifdef GAME_DLL
#include "util.h"
#else
#include "cdll_client_int.h"
#endif

#define ST_EPS 0.001

#define DEFINE_SIMTIMER( type, name ) 	DEFINE_EMBEDDED( type, name )

//-----------------------------------------------------------------------------

class CSimpleSimTimer
{
public:
	CSimpleSimTimer()
	 : m_next( -1 )
	{ 
	}

	void Force()
	{
		m_next = -1;
	}

	bool Expired() const
	{
		return ( gpGlobals->curtime - m_next > -ST_EPS );
	}

	float Delay( float delayTime )
	{
		return (m_next += delayTime);
	}
	
	float GetNext() const
	{
		return m_next;
	}

	void Set( float interval )
	{
		m_next = gpGlobals->curtime + interval;
	}

	void Set( float minInterval, float maxInterval )
	{ 
		if ( maxInterval > 0.0 )
			m_next = gpGlobals->curtime + random_valve->RandomFloat( minInterval, maxInterval );
		else
			m_next = gpGlobals->curtime + minInterval;
	}

	float GetRemaining() const
	{
		float result = m_next - gpGlobals->curtime;
		if (result < 0 )
			return 0;
		return result;
	}
	
protected:
	float m_next;
};

//-----------------------------------------------------------------------------

class CSimTimer : public CSimpleSimTimer
{
public:
	CSimTimer()	
	{ 
		this->Set( 0.0, true );
	}

	CSimTimer( float interval )	
	{ 
		this->Set( interval, true );
	}

	CSimTimer( float interval, bool startExpired )	
	{ 
		this->Set( interval, startExpired );
	}

	void Set( float interval )
	{
		this->Set( interval, true );
	}

	void Set( float minInterval, float maxInterval )
	{
		if ( maxInterval > 0.0 )
			m_interval = random_valve->RandomFloat( minInterval, maxInterval );
		else
			m_interval = minInterval;
		m_next = gpGlobals->curtime + m_interval;
	}
	
	void Set( float interval, bool startExpired )
	{ 
		m_interval = interval;
		m_next = (startExpired) ? -1.0 : gpGlobals->curtime + m_interval;
	}

	void Reset()
	{
		this->Reset( -1.0 );
	}

	void Reset( float interval )
	{
		if ( interval == -1.0 )
		{
			m_next = gpGlobals->curtime + m_interval;
		}
		else
		{
			m_next = gpGlobals->curtime + interval;
		}
	}

	float GetInterval() const
	{
		return m_interval;
	}
	
private:
	float m_interval;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class CRandSimTimer : public CSimpleSimTimer
{
public:
	CRandSimTimer( )	
	{ 
		this->Set( 0.0, 0.0, true );
	}

	CRandSimTimer( float minInterval )	
	{ 
		this->Set( minInterval, 0.0, true );
	}

	CRandSimTimer( float minInterval, float maxInterval )	
	{ 
		this->Set( minInterval, maxInterval, true );
	}

	CRandSimTimer( float minInterval, float maxInterval, bool startExpired )	
	{ 
		this->Set( minInterval, maxInterval, startExpired );
	}

	void Set( float minInterval )
	{ 
		this->Set( minInterval, 0.0, true );
	}

	void Set( float minInterval, float maxInterval )
	{ 
		this->Set( minInterval, maxInterval, true );
	}
	
	void Set( float minInterval, float maxInterval, bool startExpired )
	{ 
		m_minInterval = minInterval;
		m_maxInterval = maxInterval;
		
		if (startExpired)
		{
			m_next = -1;
		}
		else
		{
			if ( m_maxInterval == 0 )
				m_next = gpGlobals->curtime + m_minInterval;
			else
				m_next = gpGlobals->curtime + random_valve->RandomFloat( m_minInterval, m_maxInterval );
		}
	}

	void Reset()
	{
		if ( m_maxInterval == 0 )
			m_next = gpGlobals->curtime + m_minInterval;
		else
			m_next = gpGlobals->curtime + random_valve->RandomFloat( m_minInterval, m_maxInterval );
	}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CStopwatchBase  : public CSimpleSimTimer
{
public:
	CStopwatchBase()	
	{ 
		m_fIsRunning = false;
	}

	bool IsRunning() const
	{
		return m_fIsRunning;
	}
	
	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
	
protected:
	bool m_fIsRunning;
	
};

//-------------------------------------
class CSimpleStopwatch  : public CStopwatchBase
{
public:
	void Start( float minCountdown, float maxCountdown = 0.0 )
	{ 
		m_fIsRunning = true;
		CSimpleSimTimer::Set( minCountdown, maxCountdown );
	}

	void Stop()
	{
		m_fIsRunning = false;
	}

	bool Expired() const
	{
		return ( m_fIsRunning && CSimpleSimTimer::Expired() );
	}
};
//-------------------------------------

class CStopwatch : public CStopwatchBase
{
public:
	CStopwatch ( )
	{ 
		this->Set( 0.0, 0.0 );
	}

	CStopwatch ( float interval )
	{ 
		this->Set( interval, 0.0 );
	}
	
	void Set( float interval )
	{ 
		this->Set( interval, 0.0 );
	}

	void Set( float minInterval, float maxInterval )
	{ 
		if ( maxInterval > 0.0 )
			m_interval = random_valve->RandomFloat( minInterval, maxInterval );
		else
			m_interval = minInterval;
	}

	void Start( float intervalOverride )
	{ 
		m_fIsRunning = true;
		m_next = gpGlobals->curtime + intervalOverride;
	}

	void Start()
	{
		this->Start( m_interval );
	}
	
	float GetInterval() const
	{
		return m_interval;
	}
	
private:
	float m_interval;
};

//-------------------------------------

class CRandStopwatch : public CStopwatchBase
{
public:
	CRandStopwatch()	
	{ 
		this->Set( 0.0, 0.0 );
	}

	CRandStopwatch( float minInterval, float maxInterval )	
	{ 
		this->Set( minInterval, maxInterval );
	}
	
	void Set( float interval )
	{ 
		this->Set( interval, 0.0 );
	}

	void Set( float minInterval, float maxInterval )
	{ 
		m_minInterval = minInterval;
		m_maxInterval = maxInterval;
	}

	void Start( float minOverride )
	{ 
		this->Start( minOverride, 0.0 );
	}

	void Start( float minOverride, float maxOverride )
	{ 
		m_fIsRunning = true;
		if ( maxOverride == 0 )
			m_next = gpGlobals->curtime + minOverride;
		else
			m_next = gpGlobals->curtime + random_valve->RandomFloat( minOverride, maxOverride );
	}

	void Start()
	{
		this->Start( m_minInterval, m_maxInterval );
	}
	
	float GetInterval() const
	{
		return m_minInterval;
	}

	float GetMinInterval() const
	{
		return m_minInterval;
	}

	float GetMaxInterval() const
	{
		return m_maxInterval;
	}
	
private:
	float m_minInterval;
	float m_maxInterval;
};

//-----------------------------------------------------------------------------

class CThinkOnceSemaphore
{
public:
	CThinkOnceSemaphore()
	 :	m_lastTime( -1 )
	{
	}

	bool EnterThink()
	{
		if ( m_lastTime == gpGlobals->curtime )
			return false;
		m_lastTime = gpGlobals->curtime;
		return true;
	}

	bool DidThink() const
	{
		return ( gpGlobals->curtime == m_lastTime );

	}

	void SetDidThink()
	{
		m_lastTime = gpGlobals->curtime;
	}

private:
	float m_lastTime;
};

//-----------------------------------------------------------------------------

#endif // SIMTIMER_H
