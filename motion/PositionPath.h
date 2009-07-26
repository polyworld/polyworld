#ifndef POSITION_PATH_H
#define POSITION_PATH_H

#include <string>

//---------------------------------------------------------------------------
// Path Definitions
//---------------------------------------------------------------------------
#define PATH__DIR_RUN		"run"
#define PATH__DIR_MOTION	"motion"
#define PATH__DIR_POSITION	PATH__DIR_MOTION"/position"
#define PATH__PREFIX_INCOMPLETE "incomplete_"
#define PATH__PREFIX_COMPLETE   ""
#define PATH__BASENAME_POSITION "position_"
#define PATH__EXT               ".bin"

//===========================================================================
// PositionPath
//===========================================================================
class PositionPath
{
 public:
	static std::string position_dir(const char *path_run_dir = PATH__DIR_RUN);
	static std::string position_file(const char *path_position_dir,
					 long agent_number,
					 bool complete);
};

#endif // POSITION_PATH_H
