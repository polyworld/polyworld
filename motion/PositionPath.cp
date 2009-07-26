#define POSITION_PATH_CP

#include "PositionPath.h"

#include <assert.h>
#include <errno.h>
#include <glob.h>
#include <unistd.h>
#include <sys/stat.h>

#include <algorithm>

using namespace std;


//===========================================================================
// PositionPath
//===========================================================================

//---------------------------------------------------------------------------
// PositionPath::position_dir
//---------------------------------------------------------------------------
string PositionPath::position_dir(const char *path_run_dir)
{
  char buf[1024];

  sprintf(buf,
	  "%s/%s",
	  path_run_dir, PATH__DIR_POSITION);

  return buf;
}

//---------------------------------------------------------------------------
// PositionPath::position_file
//---------------------------------------------------------------------------
string PositionPath::position_file(const char *path_position_dir,
				   long agent_number,
				   bool complete)
{
  char buf[1024];

  sprintf(buf,
	  "%s/%s%s%ld%s",
	  path_position_dir,
	  complete ? PATH__PREFIX_COMPLETE : PATH__PREFIX_INCOMPLETE,
	  PATH__BASENAME_POSITION,
	  agent_number,
	  PATH__EXT);

  return buf;
}

// eof
