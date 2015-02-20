#include "MovieController.h"

#include <assert.h>
#include <stdio.h>

#include "MovieRecorder.h"
#include "PwMovieUtils.h"
#include "SceneRenderer.h"


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
            renderComplete_handle =
                renderer->renderComplete += [=]() {this->renderComplete();};
			connectedToRenderer = true;
		}
	}
	else
	{
		if( connectedToRenderer )
		{
            renderer->renderComplete -= renderComplete_handle;
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
