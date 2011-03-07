#pragma once

#include <assert.h>

#include <algorithm>
#include <list>

#include "datalib.h"
#include "misc.h"

namespace condprop
{
	//===========================================================================
	//===========================================================================
	//===========================================================================
	//===
	//=== Top-Level Classes and Templates
	//===
	//===========================================================================
	//===========================================================================
	//===========================================================================

	//===========================================================================
	// Condition
	//===========================================================================
	template<typename T>
		class Condition
	{
	public:
		Condition( T endValue )
		{
			this->endValue = endValue;
		}

		virtual ~Condition() {}

		virtual bool isSatisfied( long step ) = 0;
		virtual void setActive( long step, bool active ) = 0;
		virtual long getEnd( long step ) = 0;

		T getEndValue() { return endValue; }

	private:
		T endValue;
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
		// add()
		//---------------------------------------------------------------------------
		void add( Condition<T> *condition )
		{
			conditions.push_back( condition );
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
		virtual void update( long step ) = 0;
	};

	//===========================================================================
	// Property
	//===========================================================================
	template<typename T>
		class Property : public BaseProperty
	{
	public:
		typedef T InterpolateFunction(T value, T endValue, long time);

		//---------------------------------------------------------------------------
		// ctor()
		//---------------------------------------------------------------------------
		Property( T *value,
				  InterpolateFunction *interpolateFunction,
				  ConditionList<T> *conditionList,
				  Logger<T> *logger = NULL )
		{
			this->value = value;
			this->interpolateFunction = interpolateFunction;
			this->conditionList = conditionList;
			this->logger = logger;
		}

		//---------------------------------------------------------------------------
		// dtor()
		//---------------------------------------------------------------------------
		virtual ~Property()
		{
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
		}

		//---------------------------------------------------------------------------
		// update()
		//---------------------------------------------------------------------------
		virtual void update( long step )
		{
			Condition<T> *condition = conditionList->findActive( step );

			if( condition )
			{
				long time = max( 1l, condition->getEnd(step) - step  );

				T newValue = interpolateFunction( *value, condition->getEndValue(), time );
				if( logger )
				{
					if( (step == 1) || (newValue != *value) )
					{
						logger->log( step, newValue );
					}
				}
				*value = newValue;
			}
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
		T *value;
		InterpolateFunction *interpolateFunction;
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
			itfor( Properties, properties, it )
			{
				(*it)->update( step );
			}
		}

	private:
		Properties properties;
	};


	//===========================================================================
	//===========================================================================
	//===========================================================================
	//===
	//=== Function Implementations
	//===
	//===========================================================================
	//===========================================================================
	//===========================================================================

	//===========================================================================
	// InterpolateFunction Implementations
	//===========================================================================
	float InterpolateFunction_float( float value, float endValue, long time );


	//===========================================================================
	//===========================================================================
	//===========================================================================
	//===
	//=== Condition Implementations
	//===
	//===========================================================================
	//===========================================================================
	//===========================================================================

	//===========================================================================
	// TimeCondition
	//===========================================================================
	template<typename T>
		class TimeCondition : public Condition<T>
	{
	public:
		TimeCondition( long step, T endValue )
			: Condition<T>( endValue )
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
						TValue endValue )
		: Condition<TValue>( endValue )
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
	//===========================================================================
	//===========================================================================
	//===
	//=== Logger Implementations
	//===
	//===========================================================================
	//===========================================================================
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
}
