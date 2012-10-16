#pragma once

#include <assert.h>

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "FiringRateModel.h"
#include "SpikingModel.h"

namespace sheets
{
	//===========================================================================
	// Orientation
	//===========================================================================
	enum Orientation
	{
		PlaneXY = 0, PlaneXZ, PlaneZY
	};

	//===========================================================================
	// Vector2
	//===========================================================================
	template<typename T>
		struct Vector2
		{
			Vector2() { set(0,0); }
			Vector2( T a ) { set(a,a); }
			Vector2( T a, T b ) { set(a,b); }

			T a, b;

			void set( T a_, T b_ ) { a=a_; b=b_; }
			void operator+=( const Vector2<T> &other ) { a += other.a; b += other.b; }
			Vector2<T> operator+( const Vector2<T> &other ) const { return Vector2<T>( a + other.a, b + other.b ); }
			Vector2<T> operator-( const Vector2<T> &other ) const { return Vector2<T>( a - other.a, b - other.b ); }
			void scale( Vector2<T> &scale ) { a *= scale.a; b *= scale.b; }
		};

	template<typename T>
		Vector2<T> operator/( const Vector2<T> &vec, float scalar ) { return Vector2<T>( vec.a / scalar, vec.b / scalar ); }

	template<typename T>
	std::ostream &operator<<( std::ostream &out, const Vector2<T> &v ) {return out << "{" << v.a << "," << v.b << "}";}

	typedef Vector2<float> Vector2f;
	typedef Vector2<int> Vector2i;

	//===========================================================================
	// Vector3
	//===========================================================================
	template<typename T>
	struct Vector3
	{
		Vector3() { set(0,0,0); }
		Vector3( T a, T b, T c ) { set(a, b, c); }

		float x, y, z;

		void set( float x_, float y_, float z_ ) { x=x_; y=y_; z=z_; }
		void set( Orientation orientation, const Vector2<T> &plane, T planePos )
		{
			switch( orientation )
			{
			case PlaneXY: x = plane.a; y = plane.b; z = planePos; break;
			case PlaneXZ: x = plane.a; y = planePos; z = plane.b; break;
			case PlaneZY: x = planePos; z = plane.a; y = plane.b; break;
			default: assert( false );
			}
		}

		void scale( const Vector3<T> &scale ) { x *= scale.x; y *= scale.y; z *= scale.z; }
		float distance( Vector3<T> &other )
		{
			return sqrt( (x - other.x) * (x - other.x)
						 + (y - other.y) * (y - other.y)
						 + (z - other.z) * (z - other.z) );
		}
	};

	template<typename T>
	std::ostream &operator<<( std::ostream &out, const Vector3<T> &v ) {return out << "{" << v.x << "," << v.y << "," << v.z << "}";}

	typedef Vector3<float> Vector3f;

	//===========================================================================
	// Synapse
	//===========================================================================
	class Synapse
	{
	public:
		class Neuron *from;
		class Neuron *to;
		struct Attributes
		{
			float weight;
			float lrate;
		} attrs;
	};

	//===========================================================================
	// Neuron
	//===========================================================================
	struct NeuronKeyCompare
	{
		bool operator()( class Neuron *, class Neuron * );
	};
	typedef std::map<class Neuron *, Synapse *, NeuronKeyCompare> SynapseMap;

	class Neuron
	{
	public:
		virtual ~Neuron();

		class Sheet *sheet;
		int nonCulledId;
		int id;
		Vector2i sheetIndex;
		Vector2f sheetPosition;
		Vector3f absPosition;
		SynapseMap synapsesOut;
		SynapseMap synapsesIn;
		struct Attributes
		{
			enum Type { E, I, EI } type;
			union
			{
				FiringRateModel__NeuronAttrs firingRate;
				FiringRateModel__NeuronAttrs tau;
				SpikingModel__NeuronAttrs spiking;
			} neuronModel;
		} attrs;
		struct
		{
			bool touchedFromInput;
			bool touchedFromOutput;
		} cullState;
	};

	typedef std::vector<Neuron *> NeuronVector;

	//===========================================================================
	// NeuronSubset
	//===========================================================================
	struct NeuronSubset
	{
		Vector2i _begin;
		Vector2i _end;

		struct Iterator
		{
			NeuronSubset *_subset;
			Vector2i _index;

			Iterator( NeuronSubset *subset, const Vector2i &index );

			Vector2i &operator*();
			Iterator &operator++();
			bool operator!=( const Iterator &other ) const;
		};

		Iterator begin() { return Iterator(this, _begin); }
		Iterator end() { return Iterator(this, Vector2i(_end.a + 1, _begin.b)); }

		size_t size();
	};

	std::ostream &operator<<( std::ostream &out, const NeuronSubset &n );

	//===========================================================================
	// Sheet
	//===========================================================================
	class Sheet
	{
	public:
		enum Type
		{
			Input = 0,
			Output,
			Internal,
			__NTYPES
		};

		static const char *getName( Type type );

	private:
		friend class SheetsModel;

		Sheet( class SheetsModel *model,
			   std::string name,
			   int id,
			   Type type,
			   Orientation orientation,
			   float slot,
			   const Vector2f &center,
			   const Vector2f &size,
			   const Vector2i &neuronCount,
			   std::function<void (Neuron*)> &neuronCreated );


	public:
		virtual ~Sheet();
		
		int getId();
		std::string getName();
		Type getType();
		const Vector2i &getNeuronCount();
		Neuron *getNeuron( int a, int b );
		Neuron *getNeuron( Vector2i index );
		
		enum ReceptiveFieldRole
		{
			Source = 0, Target
		};
		enum ReceptiveFieldNeuronRole
		{
			From, To
		};
		void addReceptiveField( ReceptiveFieldRole role,
								Vector2f currentCenter,
								Vector2f currentSize,
								Vector2f otherCenter,
								Vector2f otherSize,
								Vector2f fieldOffset,
								Vector2f fieldSize,
								Sheet *other,
								std::function<bool (Neuron *, ReceptiveFieldNeuronRole)> neuronPredicate,
								std::function<void (Synapse *)> synapseCreated );

	private:
		NeuronSubset findNeurons( const Vector2f &center,
								  const Vector2f &size );
		NeuronSubset findReceptiveFieldNeurons( const Vector2f &neuronPosition,
												const Vector2f &fieldOffset,
												const Vector2f &fieldSize );

		void trim( float &center, float &size );
		void createNeurons( std::function<void (Neuron*)> &neuronCreated );
		float distance( Neuron *a, Neuron *b );

		Synapse *createSynapse( Neuron *from, Neuron *to );

		class SheetsModel *_sheetsModel;
		std::string _name;
		int _id;
		Type _type;
		Orientation _orientation;
		float _slot;
		Vector2f _center;
		Vector2f _size;
		Vector2i _neuronCount;
		Vector2f _neuronSpacing;
		Vector2f _neuronInsets;
		int _nneurons;
		Neuron *_neurons;
	};

	typedef std::vector<Sheet *> SheetVector;

	//===========================================================================
	// SheetsModel
	//===========================================================================
	class SheetsModel
	{
	public:
		SheetsModel( const Vector3f &size,
					 float synapseProbabilityX );
		virtual ~SheetsModel();

		Vector3f getSize() { return _size; }

		Sheet *createSheet( std::string name,
							int id,
						    Sheet::Type type,
							Orientation orientation,
							float slot,
							const Vector2f &center,
							const Vector2f &size,
							const Vector2i &neuronCount,
							std::function<void (Neuron*)> &neuronCreated );
		Sheet *getSheet( int id );
		const SheetVector &getSheets( Sheet::Type type );

		void cull();

		NeuronVector &getNeurons();

	private:
		friend class Sheet;

		float getProbabilitySynapse( float distance );

	private:
		void touchFromInput( Neuron *neuron );
		void touchFromOutput( Neuron *neuron );
		void addNonCulledNeurons( SheetVector &sheets );

		int _numNonCulledNeurons;
		Vector3f _size;
		float _synapseProbabilityX;
		SheetVector _allSheets;
		SheetVector _inputSheets;
		SheetVector _outputSheets;
		SheetVector _internalSheets;
		NeuronVector _neurons;
	};
}

