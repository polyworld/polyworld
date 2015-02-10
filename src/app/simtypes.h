#pragma once

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include <vector>

#include <QtGlobal>

#include "Energy.h"
#include "LifeSpan.h"
#include "simconst.h"


// Forward declarations
namespace genome { class Genome; }
class agent;
class gobject;


namespace sim
{
	typedef int EventType;

	// Each value must be a single unique bit since we use them with bitwise OR.
	// Note that we can go all the way to 128 bits on gcc with __int128.
	static const EventType Event_None = 0;
	static const EventType Event_SimInited = (1 << 0);
	static const EventType Event_AgentBirth = (1 << 1);
	static const EventType Event_BrainGrown = (1 << 2);
	static const EventType Event_AgentGrown = (1 << 3);
	static const EventType Event_BrainUpdated = (1 << 4);
	static const EventType Event_BodyUpdated = (1 << 5);
	static const EventType Event_ContactBegin = (1 << 6);
	static const EventType Event_ContactEnd = (1 << 7);
	static const EventType Event_Collision = (1 << 8);
	static const EventType Event_Carry = (1 << 9);
	static const EventType Event_Energy = (1 << 10);
	static const EventType Event_AgentDeath = (1 << 11);
	static const EventType Event_BrainAnalysisBegin = (1 << 12);
	static const EventType Event_BrainAnalysisEnd = (1 << 13);
	static const EventType Event_StepEnd = (1 << 14);
	static const EventType Event_EpochEnd = (1 << 15);

	//===========================================================================
	// SimInitedEvent
	//===========================================================================
	struct SimInitedEvent
	{
		inline EventType getType() const { return Event_SimInited; }
	};

	//===========================================================================
	// AgentBirthEvent
	//===========================================================================
	struct AgentBirthEvent
	{
		inline EventType getType() const { return Event_AgentBirth; }

		AgentBirthEvent( agent *_a,
						 LifeSpan::BirthReason _reason,
						 agent *_parent1,
						 agent *_parent2 )
		: a(_a)
		, reason(_reason)
		, parent1(_parent1)
		, parent2(_parent2)
		{}

		agent *a;
		LifeSpan::BirthReason reason;
		agent *parent1;
		agent *parent2;
	};

	//===========================================================================
	// BrainGrownEvent
	//===========================================================================
	struct BrainGrownEvent
	{
		inline EventType getType() const { return Event_BrainGrown; }

		BrainGrownEvent( agent *_a )
		: a(_a)
		{}

		agent *a;
	};

	//===========================================================================
	// AgentGrownEvent
	//===========================================================================
	struct AgentGrownEvent
	{
		inline EventType getType() const { return Event_AgentGrown; }

		AgentGrownEvent( agent *_a )
		: a(_a)
		{}

		agent *a;
	};

	//===========================================================================
	// AgentBodyUpdatedEvent
	//===========================================================================
	struct AgentBodyUpdatedEvent
	{
		inline EventType getType() const { return Event_BodyUpdated; }

		AgentBodyUpdatedEvent( agent *_a ) : a(_a) {}
		
		agent *a;
	};

	//===========================================================================
	// BrainUpdatedEvent
	//===========================================================================
	struct BrainUpdatedEvent
	{
		inline EventType getType() const { return Event_BrainUpdated; }

		BrainUpdatedEvent( agent *_a ) : a(_a) {}
		
		agent *a;
	};

	//===========================================================================
	// AgentContactBeginEvent
	//===========================================================================
	struct AgentContactBeginEvent
	{
		inline EventType getType() const { return Event_ContactBegin; }

		AgentContactBeginEvent( agent *_c, agent *_d );

		struct AgentInfo
		{
			void init( agent *a );

			agent *a;
			long number;
			int mate;
			int fight;
			int give;
		} c, d;

		void mate( agent *a, int status );
		void fight( agent *a, int status );
		void give( agent *a, int status );

		AgentInfo *get( agent *a );
	};

	//===========================================================================
	// AgentContactEndEvent
	//===========================================================================
	struct AgentContactEndEvent
	{
		inline EventType getType() const { return Event_ContactEnd; }

		AgentContactEndEvent( const AgentContactBeginEvent &e );

		struct AgentInfo
		{
			void init( const AgentContactBeginEvent::AgentInfo &begin );

			long number;
			int mate;
			int fight;
			int give;
		} c, d;
	};

	//===========================================================================
	// CollisionEvent
	//===========================================================================
	struct CollisionEvent
	{
		inline EventType getType() const { return Event_Collision; }

		CollisionEvent( agent *_a, ObjectType _ot ) : a(_a), ot(_ot) {}
		
		agent *a;
		ObjectType ot;
	};

	//===========================================================================
	// CarryEvent
	//===========================================================================
	struct CarryEvent
	{
		inline EventType getType() const { return Event_Carry; }

		enum Action { Pickup = 0, DropRecent, DropObject };

		CarryEvent( agent *_a, Action _action, gobject *_obj )
		: a(_a), action(_action), obj(_obj) {}

		agent *a;
		Action action;
		gobject *obj;
	};

	//===========================================================================
	// EnergyEvent
	//===========================================================================
	struct EnergyEvent
	{
		inline EventType getType() const { return Event_Energy; }

		enum Action { Give = 0, Fight, Eat };

		EnergyEvent( agent *_a,
					 gobject *_obj,
					 float _neuralActivation,
					 const Energy &_energy,
					 Action _action )
		: a(_a), obj(_obj), neuralActivation(_neuralActivation), energy(_energy), action(_action) {}

		agent *a;
		gobject *obj;
		float neuralActivation;
		const Energy &energy;
		Action action;
	};

	//===========================================================================
	// AgentDeathEvent
	//===========================================================================
	struct AgentDeathEvent
	{
		inline EventType getType() const { return Event_AgentDeath; }

		AgentDeathEvent( agent *_a,
						 LifeSpan::DeathReason _reason )
		: a(_a)
		, reason(_reason)
		{}

		agent *a;
		LifeSpan::DeathReason reason;
	};

	//===========================================================================
	// BrainAnalysisBeginEvent
	//===========================================================================
	struct BrainAnalysisBeginEvent
	{
		inline EventType getType() const { return Event_BrainAnalysisBegin; }

		BrainAnalysisBeginEvent( agent *_a )
		: a(_a)
		{}

		agent *a;
	};

	//===========================================================================
	// BrainAnalysisEndEvent
	//===========================================================================
	struct BrainAnalysisEndEvent
	{
		inline EventType getType() const { return Event_BrainAnalysisEnd; }

		BrainAnalysisEndEvent( agent *_a )
		: a(_a)
		{}

		agent *a;
	};

	//===========================================================================
	// StepEndEvent
	//===========================================================================
	struct StepEndEvent
	{
		inline EventType getType() const { return Event_StepEnd; }
	};

	//===========================================================================
	// EpochEndEvent
	//===========================================================================
	struct EpochEndEvent
	{
		inline EventType getType() const { return Event_EpochEnd; }

		EpochEndEvent( long _epoch ) : epoch(_epoch) {}

		long epoch;
	};

	//===========================================================================
	// StatusText
	//===========================================================================
	typedef std::vector<char *> StatusText;

	//===========================================================================
	// Position
	//===========================================================================
	struct Position
	{
		float x;
		float y;
		float z;
	};

	//===========================================================================
	// Stat
	//===========================================================================
	class Stat
	{
	public:
		Stat()					{ reset(); }
		~Stat()					{}
	
		float	mean()			{ if( !count ) return( 0.0 ); return( sum / count ); }
		float	stddev()		{ if( !count ) return( 0.0 ); register double m = sum / count;  return( sqrt( sum2 / count  -  m * m ) ); }
		float	min()			{ if( !count ) return( 0.0 ); return( mn ); }
		float	max()			{ if( !count ) return( 0.0 ); return( mx ); }
		void	add( float v )	{ sum += v; sum2 += v*v; count++; mn = v < mn ? v : mn; mx = v > mx ? v : mx; }
		void	reset()			{ mn = FLT_MAX; mx = FLT_MIN; sum = sum2 = count = 0; }
		unsigned long samples() { return( count ); }

	private:
		float	mn;		// minimum
		float	mx;		// maximum
		double	sum;	// sum
		double	sum2;	// sum of squares
		unsigned long	count;	// count
	};

	//===========================================================================
	// StatRecent
	//===========================================================================
	class StatRecent
	{
	public:
		StatRecent( unsigned int width = 1000 )	{ w = width; history = (float*) malloc( w * sizeof(*history) ); Q_CHECK_PTR( history); reset(); }
	~StatRecent()							{ if( history ) free( history ); }
	
	float	mean()			{ if( !count ) return( 0.0 ); return( sum / count ); }
	float	stddev()		{ if( !count ) return( 0.0 ); register double m = sum / count;  return( sqrt( sum2 / count  -  m * m ) ); }
	float	min()			{ if( !count ) return( 0.0 ); if( needMin ) recomputeMin(); return( mn ); }
	float	max()			{ if( !count ) return( 0.0 ); if( needMax ) recomputeMax(); return( mx ); }
	void	add( float v )	{ if( count < w ) { sum += v; sum2 += v*v; mn = v < mn ? v : mn; mx = v > mx ? v : mx; history[index++] = v; count++; } else { if( index >= w ) index = 0; sum += v - history[index]; sum2 += v*v - history[index]*history[index]; if( v >= mx ) mx = v; else if( history[index] == mx ) needMax = true; if( v <= mn ) mn = v; else if( history[index] == mn ) needMin = true; history[index++] = v; } }
	void	reset()			{ mn = FLT_MAX; mx = FLT_MIN; sum = sum2 = count = index = 0; needMin = needMax = false; }
	unsigned long samples() { return( count ); }

private:
	float	mn;		// minimum
	float	mx;		// maximum
	double	sum;	// sum
	double	sum2;	// sum of squares
	unsigned long	count;	// count
	unsigned int	w;		// width of data window considered current (controls roll-off)
	float*	history;	// w recent samples
	unsigned int	index;	// point in history[] at which next sample is to be inserted
	bool	needMin;
	bool	needMax;
	
	void	recomputeMin()	{ mn = FLT_MAX; for( unsigned int i = 0; i < w; i++ ) mn = history[i] < mn ? history[i] : mn; needMin = false; }
	void	recomputeMax()	{ mx = FLT_MIN; for( unsigned int i = 0; i < w; i++ ) mx = history[i] > mx ? history[i] : mx; needMax = false; }
};
}
