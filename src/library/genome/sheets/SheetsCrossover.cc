#include "SheetsCrossover.h"

#include <string.h>

#include <algorithm>

#include "Gene.h"
#include "GenomeSchema.h"

using namespace genome;
using namespace std;

SheetsCrossover::Level::Level( SheetsCrossover *crossover, const char *name )
{
	this->name = name;
	probability = 0;

	if( this != &crossover->PhysicalLevel )
	{
		crossover->_levels.push_back( const_cast<Level *>(this) );
	}
}

void SheetsCrossover::Level::defineBoundaryPoints( Gene *gene )
{
	PointDefinition def;
	def.type = PointDefinition::BoundaryPoints;
	def.boundaryPoints.gene = gene;

	_pointDefs.push_back( def );
}

void SheetsCrossover::Level::defineRange( Gene *start, Gene *end )
{
	PointDefinition def;
	def.type = PointDefinition::Range;
	def.range.start = start;
	def.range.end = end;

	_pointDefs.push_back( def );
}

void SheetsCrossover::Level::complete( 	genome::GenomeSchema *schema )
{
	for( PointDefinition &def : _pointDefs )
	{
		switch( def.type )
		{
		case PointDefinition::BoundaryPoints:
			{
				Gene *gene = def.boundaryPoints.gene;
				Point start = { gene->getOffset(), gene->getOffset() };
				_points.push_back( start );

				Point end = { gene->getOffset() + gene->getMutableSize() - 1,
							  gene->getOffset() + gene->getMutableSize() - 1 };
				_points.push_back( end );
			}
			break;
		case PointDefinition::Range:
			{
				Point range = { def.range.start == NULL
									? 0
									: def.range.start->getOffset(),
								def.range.end == NULL
									? schema->getMutableSize() - 1
									: def.range.end->getOffset() + def.range.end->getMutableSize() - 1 };
				_points.push_back( range );
			}
			break;
		default:
			assert( false );
		}
	}

	sort( _points.begin(), _points.end() );

	for( int i = 1; i < (int)_points.size(); i++ )
		assert( _points[i - 1].end <= _points[i].start );
}

int SheetsCrossover::Level::getEnd()
{
	return _points.back().end;
}

bool SheetsCrossover::Level::Point::operator<( const Point &other ) const
{
	return end < other.start;
}

bool SheetsCrossover::Level::next( int start, int *result )
{
	Point search = { start + 1, start + 1 };
	vector<Point>::iterator it = lower_bound( _points.begin(), _points.end(), search );

	if( it == _points.end() )
		return false;

	if( it->start == it->end )
	{
		*result = it->start;
	}
	else
	{
		assert( (start >= it->start) && (start <= it->end) );

		start = max( start, it->start );
		*result = start + round( randpw() * (it->end - start) );

		assert( *result <= it->end );
	}

	assert( *result >= start );

	return true;
}

SheetsCrossover::SheetsCrossover()
: PhysicalLevel( this, "Physical" )
, GeneLevel( this, "Gene" )
, SheetLevel( this, "Sheet" )
, NeuronAttrLevel( this, "NeuronAttr" )
, ReceptiveFieldLevel( this, "ReceptiveField" )
{
}

void SheetsCrossover::complete( GenomeSchema *schema )
{
	PhysicalLevel.complete( schema );

	for( Level *level : _levels )
		level->complete( schema );

	{
		// sort levels by ascending probability
		sort( _levels.begin(), _levels.end(),
			  [] ( Level *a, Level *b ) { return a->probability < b->probability; } );

		float psum = 0.0;

		for( Level *level : _levels )
		{
			psum += level->probability;
			level->probability = psum;
		}
			
		if( fabs(psum - 1.0) > 1e-6 )
		{
			cerr << "Sheets crossover point probabilities must sum to 1.0" << endl;
			exit( 1 );
		}
		
	}

	_physicalEnd = PhysicalLevel.getEnd();
	_genomeEnd = schema->getMutableSize() - 1;
}

SheetsCrossover::Segment::Segment()
{
	start = end = -1;
	level = NULL;
}

bool SheetsCrossover::nextSegment( Segment &segment )
{
	if( segment.end == _genomeEnd )
		return false;

	segment.start = segment.end + 1;
	nextPoint( segment );

	assert( segment.start >= 0 );
	assert( segment.end <= _genomeEnd );

	return true;
}

void SheetsCrossover::nextPoint( Segment &segment )
{
	segment.end = _genomeEnd;

	if( segment.start == 0 )
	{
		PhysicalLevel.next( segment.start, &segment.end );
		segment.level = &PhysicalLevel;
	}
	else if( segment.start <= _physicalEnd ) 
	{
		segment.end = _physicalEnd;
		segment.level = &PhysicalLevel;
	}
	else
	{
		segment.level = NULL;

		float selector = randpw();

		for( Level *level : _levels )
		{
			if( (selector <= level->probability)
				&& level->next(segment.start, &segment.end) )
			{
				segment.level = level;
				break;
			}
		}
	}
}
