#pragma once

#include <vector>
#include <map>

namespace genome { class Gene; class GenomeSchema; }

class SheetsCrossover
{
 public:
	class Level
	{
	public:
		void defineBoundaryPoints( genome::Gene *gene );
		void defineRange( genome::Gene *start, genome::Gene *end );

		const char *name;
		float probability;

	private:
		friend class SheetsCrossover;
		Level( SheetsCrossover *crossover, const char *name );
		void complete( genome::GenomeSchema *schema );
		int getEnd();
		bool next( int start, int *result );

		struct PointDefinition
		{
			enum Type { BoundaryPoints, Range } type;
			union
			{
				struct 
				{
					genome::Gene *gene;
				} boundaryPoints;
				struct
				{
					genome::Gene *start;
					genome::Gene *end;
				} range;
			};
		};

		struct Point
		{
			int start;
			int end;

			bool operator<( const Point &other ) const;
		};

		std::vector<PointDefinition> _pointDefs;
		std::vector<Point> _points;
	};

 private:
	friend class Level;
	std::vector<Level *> _levels;

 public:
	Level PhysicalLevel;
	Level GeneLevel;
	Level SheetLevel;
	Level NeuronAttrLevel;
	Level ReceptiveFieldLevel;

	SheetsCrossover();
	void complete( genome::GenomeSchema *schema );

	class Segment
	{
	public:
		Segment();

		Level *level;
		int start;
		int end;
	};
	bool nextSegment( Segment &segment );

 private:

	void nextPoint( Segment &segment );

	int _physicalEnd;
	int _genomeEnd;
};
