#include "MovieController.h"

#include <assert.h>
#include <stdio.h>

#include "SceneRenderer.h"
#include "PwMovieUtils.h"


SceneMovieController::SceneMovieController( SceneRenderer *_renderer,
											const MovieSettings &_settings)
	: renderer( _renderer )
	, settings( _settings )
	, recorder( NULL )
	, connectedToRenderer( false )
	, timestep( 0 )
{
	assert( settings.shouldRecord() );

	FILE *f = fopen( settings.getMoviePath(), "wb" );
	if( !f )
	{
		perror( settings.getMoviePath() );
		exit( 1 );
	}

	writer = new PwMovieWriter( f );
}

SceneMovieController::~SceneMovieController()
{
	delete recorder;
	delete writer;
}

void SceneMovieController::step( long timestep )
{
	this->timestep = timestep;

	if( settings.shouldRecord(timestep) )
	{
		if( !connectedToRenderer )
		{
			connect( renderer, SIGNAL(renderComplete()),
					 this, SLOT(renderComplete()) );
			connectedToRenderer = true;
		}
	}
	else
	{
		if( connectedToRenderer )
		{
			disconnect( renderer, SIGNAL(renderComplete()),
						this, SLOT(renderComplete()) );
			connectedToRenderer = false;
		}
	}
}

void SceneMovieController::renderComplete()
{
	if( recorder == NULL )
	{
		recorder = renderer->createMovieRecorder( writer );
	}

	recorder->recordFrame( (uint32_t)timestep );
}
