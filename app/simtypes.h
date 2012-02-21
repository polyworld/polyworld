#pragma once

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include <vector>

// Forward declarations
namespace genome { class Genome; }


namespace sim
{
	typedef std::vector<char *> StatusText;

	struct Position
	{
		float x;
		float y;
		float z;
	};

	struct FitStruct
	{
		ulong	agentID;
		float	fitness;
		float   complexity;
		genome::Genome *genes;
	};
	typedef struct FitStruct FitStruct;

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
