#pragma once

#include <string>

#include "Signal.h"


class MovieSettings
{
 public:
	MovieSettings( bool _record,
				   std::string _moviePath,
				   int _sampleFrequency,
				   int _sampleDuration )
		: record( _record )
		, moviePath( _moviePath )
		, sampleFrequency( _sampleFrequency )
		, sampleDuration( _sampleDuration )
	{
#if __BIG_ENDIAN__
		if( record )
		{
			static bool warn = true;
			if( warn )
			{
				fprintf( stderr, "Sorry, disabling movie recording, because they cannot currently be recorded on big endian machines\n" );
				warn = false;
			}
			record = false;
		}
#endif
	}

	const char *getMoviePath() const
	{
		return moviePath.c_str();
	}

	bool shouldRecord() const
	{
		return record;
	}

	bool shouldRecord( long timestep ) const
	{
		return record && ((timestep-1) % sampleFrequency < sampleDuration);
	}

 private:
	bool record;
	std::string moviePath;
	int sampleFrequency;
	int sampleDuration;
};


class SceneMovieController
{
 public:
	SceneMovieController( class SceneRenderer *_renderer, const MovieSettings &settings );
	virtual ~SceneMovieController();

	void step( long timestep );

 private:
	void renderComplete();
    util::Signal<>::SlotHandle renderComplete_handle;

	class SceneRenderer *renderer;
	MovieSettings settings;
	class PwMovieWriter *writer;
	class PwMovieQGLPixelBufferRecorder *recorder;
	bool connectedToRenderer;
	long timestep;
};
