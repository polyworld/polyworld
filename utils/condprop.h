#pragma once

#include <assert.h>

#include <algorithm>
#include <iostream>
#include <list>

#include "datalib.h"
#include "misc.h"

#pragma push_macro("ERR")
#undef ERR
#define ERR(MSG) {std::cerr << __FILE__ << ":" << __LINE__ << ": " << MSG << std::endl; exit( 1 );}

//#define TRC(STMT) STMT
#define TRC(STMT)

namespace condprop
{
	enum CouplingRange
	{
		COUPLING_RANGE__NONE,
		COUPLING_RANGE__BEGIN,
		COUPLING_RANGE__END
	};

	//===========================================================================
	// Condition
	//===========================================================================
	template<typename T>
		class Condition
	{
	public:
	    Condition( T _endValue, CouplingRange _couplingRange )
			: endValue( _endValue )
			, couplingRange( _couplingRange )
		{
		}

		virtual ~Condition() {}

		virtual bool isSatisfied( long step ) = 0;
		virtual void setActive( long step, bool active ) = 0;
		virtual long getEnd( long step ) = 0;

		T getEndValue() { return endValue; }
		CouplingRange getCouplingRange() { return couplingRange; }

	private:
		T endValue;
		CouplingRange couplingRange;
	};

	//===========================================================================
	// TimeCondition
	//===========================================================================
	template<typename T>
		class TimeCondition : public Condition<T>
	{
	public:
	TimeCondition( long step, T endValue, CouplingRange couplingRange )
			: Condition<T>( endValue, couplingRange )
		{
			end = step;
		}
		virtual ~TimeCondition() {}

		virtual bool isSatisfied( long step )
		{
			return (step < end)
				|| ( (end == 0) && (step <= 1) );
		}

		virtual void setActive( long step, bool active )
		{
			// no-op
		}

		virtual long getEnd( long step )
		{
			return end;
		}

	private:
		long end;
	};

	//===========================================================================
	// ThresholdCondition
	//===========================================================================
	template<typename TValue, typename TStat>
		class ThresholdCondition : public Condition<TValue>
	{
	public:
		enum Op
		{
			EQ, GT, LT
		};

	ThresholdCondition( const TStat *stat,
						Op op,
						TStat threshold,
						long duration,
						TValue endValue,
						CouplingRange couplingRange )
		: Condition<TValue>( endValue, couplingRange )
		{
			parms.stat = stat;
			parms.op = op;
			parms.threshold = threshold;
			parms.duration = duration;

			state.end = -1;
		}

		virtual ~ThresholdCondition() {}

		virtual bool isSatisfied( long step )
		{
			TStat stat = *(parms.stat);
			TStat threshold = parms.threshold;

			switch( parms.op )
			{
			case EQ:
				return stat == threshold;
			case GT:
				return stat > threshold;
			case LT:
				return stat < threshold;
			default:
				assert( false );
			}
		}

		virtual void setActive( long step, bool active )
		{
			if( active )
			{
				state.end = step + parms.duration;
			}
			else
			{
				state.end = -1;
			}
		}

		virtual long getEnd( long step )
		{
			assert( state.end > -1 );

			return state.end;
		}

	private:
		struct Parameters
		{
			const TStat *stat;
			Op op;
			TStat threshold;
			long duration;
		} parms;

		struct State
		{
			long end;
		} state;
	};

	//===========================================================================
	// ConditionList
	//===========================================================================
	template<typename T>
		class ConditionList
	{
	private:
		typedef typename std::list<Condition<T> *> Conditions;

	public:
		//---------------------------------------------------------------------------
		// ctor()
		//---------------------------------------------------------------------------
		ConditionList()
		{
			activeCondition = NULL;
		}

		//---------------------------------------------------------------------------
		// dtor()
		//---------------------------------------------------------------------------
		~ConditionList()
		{
			itfor( typename Conditions, conditions, it )
			{
				delete *it;
			}
		}

		//---------------------------------------------------------------------------
		// size()
		//---------------------------------------------------------------------------
		size_t size()
		{
			return conditions.size();
		}

		//---------------------------------------------------------------------------
		// add()
		//---------------------------------------------------------------------------
		void add( Condition<T> *condition )
		{
			conditions.push_back( condition );
		}

		//---------------------------------------------------------------------------
		// cancelActive()
		//---------------------------------------------------------------------------
		void cancelActive( long step )
		{
			if( activeCondition )
			{
				activeCondition->setActive( step, false );
				activeCondition = NULL;
			}
		}

		//---------------------------------------------------------------------------
		// findActive()
		//---------------------------------------------------------------------------
		Condition<T> *findActive( long step )
		{
			Condition<T> *nextCondition = NULL;

			itfor( typename Conditions, conditions, it )
			{
				Condition<T> *condition = *it;

				if( condition->isSatisfied( step ) )
				{
					nextCondition = condition;
					break;
				}
			}

			if( nextCondition != activeCondition )
			{
				if( activeCondition )
					activeCondition->setActive( step, false );

				activeCondition = nextCondition;

				if( activeCondition )
					activeCondition->setActive( step, true );
			}

			return activeCondition;
		}

		//---------------------------------------------------------------------------
		// findBalanceRange()
		// 
		// name: used for error message to stderr.
		//---------------------------------------------------------------------------
		std::pair<T,T> *findBalanceRange( const std::string &name )
		{
			Condition<T> *conditionBegin = NULL;
			Condition<T> *conditionEnd = NULL;

			itfor( typename Conditions, conditions, it )
			{
				Condition<T> *condition = *it;

				switch( condition->getCouplingRange() )
				{
				case COUPLING_RANGE__NONE:
					// no-op
					break;
				case COUPLING_RANGE__BEGIN:
					conditionBegin = condition;
					break;
				case COUPLING_RANGE__END:
					conditionEnd = condition;
					break;
				default:
					assert( false );
				}
			}

			if( (conditionBegin == NULL) != (conditionEnd == NULL) )
			{
				ERR( "Property '" << name << "' must define both Begin and End for CouplingRange." );
			}

			TRC(std::cout << "Found range for " << name << " = " << (conditionBegin != NULL) << std::endl);

			if( conditionBegin )
			{
				std::pair<T,T> *result = new std::pair<T,T>( conditionBegin->getEndValue(),
															 conditionEnd->getEndValue() );
				if( result->first == result->second )
				{
					ERR( "Property '" << name << "' has same Begin and End values for CouplingRange." );
				}
				
				return result;
			}
			else
			{
				return NULL;
			}
		}

		//---------------------------------------------------------------------------
		// findValueRange()
		// 
		// name: used for error message to stderr.
		//---------------------------------------------------------------------------

	private:
		//---------------------------------------------------------------------------
		// FIELDS
		//---------------------------------------------------------------------------
		Condition<T> *activeCondition;
		Conditions conditions;
	};

	//===========================================================================
	// Logger
	//===========================================================================
	template<typename T>
		class Logger
	{
	public:
		virtual ~Logger() {}

		virtual void init() = 0;
		virtual void log( long step, T value ) = 0;
	};

	//===========================================================================
	// BaseProperty
	//===========================================================================
	class BaseProperty
	{
	public:
		virtual ~BaseProperty() {}

		virtual void init() = 0;
		virtual bool isLogging() = 0;
		virtual float getNormalizedDistance() = 0;
		virtual void update( long step, float maxNormalizedDistance ) = 0;
	};

	//===========================================================================
	// Property
	//===========================================================================
	template<typename T>
		class Property : public BaseProperty
	{
	public:
		typedef T InterpolateFunction(T value, T endValue, float ratio);
		typedef float DistanceFunction(T a, T b);

		//---------------------------------------------------------------------------
		// ctor()
		//---------------------------------------------------------------------------
		Property( std::string name,
				  T *value,
				  InterpolateFunction *interpolateFunction,
				  DistanceFunction *distanceFunction,
				  ConditionList<T> *conditionList,
				  Logger<T> *logger = NULL )
		{
			this->name = name;
			this->value = value;
			this->interpolateFunction = interpolateFunction;
			this->distanceFunction = distanceFunction;
			this->conditionList = conditionList;
			this->logger = logger;

			balanceRange = conditionList->findBalanceRange( name );
		}

		//---------------------------------------------------------------------------
		// dtor()
		//---------------------------------------------------------------------------
		virtual ~Property()
		{
			delete balanceRange;
			delete conditionList;
			if( logger )
				delete logger;
		}

		//---------------------------------------------------------------------------
		// init()
		//---------------------------------------------------------------------------
		virtual void init()
		{
			if( logger )
			{
				logger->init();
			}

			Condition<T> *condition = conditionList->findActive( 0 );
			*value = condition->getEndValue();
		}

		//---------------------------------------------------------------------------
		// getNormalizedDistance()
		//---------------------------------------------------------------------------
		virtual float getNormalizedDistance()
		{
			return getNormalizedDistance( *value );
		}

		//---------------------------------------------------------------------------
		// getNormalizedDistance()
		//---------------------------------------------------------------------------
		float getNormalizedDistance( T value )
		{
			if( balanceRange == NULL )
				return -1;
			else
			{
				float rangeDistance = distanceFunction( balanceRange->first, balanceRange->second );
				float currDistance = distanceFunction( balanceRange->first, value );

				return currDistance / rangeDistance;
			}
		}

		//---------------------------------------------------------------------------
		// update()
		//---------------------------------------------------------------------------
		virtual void update( long step,
							 float maxNormalizedDistance )
		{
			T newValue = *value;

			Condition<T> *condition = conditionList->findActive( step );

			if( condition )
			{
				long timeRemaining = max( 1l, condition->getEnd(step) - step  );
				newValue = interpolateFunction( newValue, condition->getEndValue(), 1.0f / timeRemaining );
			}

			if( balanceRange != NULL )
			{
				float normalizedDistance = getNormalizedDistance( newValue );

				if( normalizedDistance > maxNormalizedDistance )
				{
					// we don't want it to keep interpolating its value over time.
					conditionList->cancelActive( step );

					newValue = interpolateFunction( balanceRange->first,
													balanceRange->second,
													maxNormalizedDistance );

					TRC(std::cout << name << " [BLOCKED] normdist = " << normalizedDistance << std::endl);

				}
				else
				{
					TRC(std::cout << name << " normdist = " << normalizedDistance << std::endl);
				}
			}				

			if( logger )
			{
				if( (step == 1) || (*value != newValue) )
				{
					logger->log( step, newValue );
				}
			}

			*value = newValue;
		}

		//---------------------------------------------------------------------------
		// isLogging()
		//---------------------------------------------------------------------------
		virtual bool isLogging()
		{
			return logger != NULL;
		}

	private:
		//---------------------------------------------------------------------------
		// FIELDS
		//---------------------------------------------------------------------------
		std::string name;
		T *value;
		std::pair<T,T> *balanceRange;
		InterpolateFunction *interpolateFunction;
		DistanceFunction *distanceFunction;
		ConditionList<T> *conditionList;
		Logger<T> *logger;
	};

	//===========================================================================
	// PropertyList
	//===========================================================================
	class PropertyList
	{
	private:
		typedef std::list<BaseProperty *> Properties;

	public:
		PropertyList()
		{
		}
		
		~PropertyList()
		{
			dispose();
		}

		void init()
		{
			itfor( Properties, properties, it )
			{
				(*it)->init();
			}
		}

		bool isLogging()
		{
			itfor( Properties, properties, it )
			{
				if( (*it)->isLogging() )
					return true;
			}
			return false;
		}
		
		void dispose()
		{
			itfor( Properties, properties, it )
			{
				delete *it;
			}

			properties.clear();
		}

		void add( BaseProperty *prop )
		{
			properties.push_back( prop );
		}

		void update( long step )
		{
			const float maxLead = 0.05;

			float minNormalizedDistance = 100; // normalized distance can only go to 1
			itfor( Properties, properties, it )
			{
				BaseProperty *prop = *it;

				float normalizedDistance = prop->getNormalizedDistance();
				if( normalizedDistance >= 0 )
					minNormalizedDistance = std::min( minNormalizedDistance,
													  normalizedDistance );
			}

			float maxNormalizedDistance = minNormalizedDistance == 100 ? 1.0f : minNormalizedDistance + maxLead;

			TRC(std::cout << "minNormDist = " << minNormalizedDistance << ", maxNormDist = " << maxNormalizedDistance << std::endl);

			itfor( Properties, properties, it )
			{
				(*it)->update( step, maxNormalizedDistance );
			}
		}

	private:
		Properties properties;
	};

	//===========================================================================
	//=== LineSegment
	//===========================================================================
	class LineSegment
	{
	public:
		LineSegment( float xa, float za, float xb, float zb )
		{
			this->xa = xa;
			this->za = za;
			this->xb = xb;
			this->zb = zb;
		}

		bool operator !=( const LineSegment &other )
		{
			return !(*this == other);
		}

		bool operator ==( const LineSegment &other )
		{
			return xa == other.xa
				&& za == other.za
				&& xb == other.xb
				&& zb == other.zb;
		}

		float xa;
		float za;
		float xb;
		float zb;
	}; 

	//===========================================================================
	// InterpolateFunction Implementations
	//===========================================================================
	float InterpolateFunction_float( float value, float endValue, float ratio );
	long InterpolateFunction_long( long value, long endValue, float ratio );
	LineSegment InterpolateFunction_LineSegment( LineSegment value, LineSegment endValue, float ratio );

	//===========================================================================
	// DistanceFunction Implementations
	//===========================================================================
	float DistanceFunction_float( float a, float b );
	float DistanceFunction_long( long a, long b );
	float DistanceFunction_LineSegment( LineSegment a, LineSegment b );

	//===========================================================================
	// ScalarLogger
	//===========================================================================
	template<typename T>
		class ScalarLogger : public Logger<T>
	{
	public:
		ScalarLogger( const char *name,
					  datalib::Type type )
		{
			this->name = name;
			this->type = type;
		}
		
		virtual ~ScalarLogger()
		{
			delete writer;
		}

		virtual void init()
		{
			writer = new DataLibWriter( (std::string("run/condprop/") + name + ".txt").c_str() );

			const char *colnames[] =
				{
					"Step",
					"Value",
					NULL
				};
			const datalib::Type coltypes[] =
				{
					datalib::INT,
					type
				};

			writer->beginTable( "Values",
								colnames,
								coltypes );
		}

		virtual void log( long step, T value )
		{
			writer->addRow( step, value );
			writer->flush();
		}

	private:
		std::string name;
		datalib::Type type;
		DataLibWriter *writer;
	};

	//===========================================================================
	// LineSegmentLogger
	//===========================================================================
	class LineSegmentLogger : public Logger<LineSegment>
	{
	public:
		LineSegmentLogger( const char *name )
		{
			this->name = name;
		}
		
		virtual ~LineSegmentLogger()
		{
			delete writer;
		}

		virtual void init()
		{
			writer = new DataLibWriter( (std::string("run/condprop/") + name + ".txt").c_str() );

			const char *colnames[] = {"Time", "X1", "Z1", "X2","Z2", NULL};
			const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};
			writer->beginTable( "Values",
								colnames,
								coltypes );
		}

		virtual void log( long step, LineSegment value )
		{
			writer->addRow( step, value.xa, value.za, value.xb, value.zb );
			writer->flush();
		}

	private:
		std::string name;
		DataLibWriter *writer;
	};

#pragma pop_macro("ERR")
}
