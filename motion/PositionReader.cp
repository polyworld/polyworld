#include "PositionReader.h"

#include <assert.h>
#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <algorithm>

#include "PositionPath.h"
#include "PositionRecord.h"

using namespace std;

//---------------------------------------------------------------------------
// Misc Macros
//---------------------------------------------------------------------------

#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

#if 0
//---------------------------------------------------------------------------
// Trivial Utility Functions
//---------------------------------------------------------------------------

static FILE *__fopen(const char *path,
		     const char *mode)
{
	FILE *file = ::fopen(path, mode);
	if(file == NULL)
	{
		fprintf(stderr, "Failed opening %s\n", path);
		assert(false);
	}

	return file;
}
#endif

//===========================================================================
// PopulationHistory
//===========================================================================

//---------------------------------------------------------------------------
// PopulationHistory::~PopulationHistory
//---------------------------------------------------------------------------
PopulationHistory::~PopulationHistory()
{
	for(AgentNumberMap::iterator
	      it = numberLookup.begin(),
	      it_end = numberLookup.end();
	    it != it_end;
	    it++)
	{
		delete it->second;
	}
}


//===========================================================================
// PositionReader
//===========================================================================

//---------------------------------------------------------------------------
// PositionReader::PositionReader
//---------------------------------------------------------------------------
PositionReader::PositionReader(const char *path_position_dir)
{
	this->path_position_dir = path_position_dir;

	initPopulationHistory();
}

//---------------------------------------------------------------------------
// PositionReader::~PositionReader
//---------------------------------------------------------------------------
PositionReader::~PositionReader()
{
}

//---------------------------------------------------------------------------
// PositionReader::getPopulationHistory()
//---------------------------------------------------------------------------
const PopulationHistory *PositionReader::getPopulationHistory()
{
	return &population_history;
}

//---------------------------------------------------------------------------
// PositionReader::getPositions
//---------------------------------------------------------------------------
void PositionReader::getPositions(PositionQuery *query)
{
	PopulationHistoryEntry *entry = population_history.numberLookup[query->agent_number];
	assert(entry);

	long epoch_begin = query->steps_epoch.begin;
	long epoch_end = query->steps_epoch.end;
	int nsteps = query->steps_epoch.duration();


	if(query->nrecords < nsteps)
	{
		if(query->records) delete query->records;
		query->records = new PositionRecord[nsteps];
		query->nrecords = nsteps;
	}

	FILE *file = open(entry->agent_number);

	long step_begin = max(entry->step_begin,
			      epoch_begin);
	long step_end = min(entry->step_end,
			    epoch_end);

	long nseek =
	  sizeof(PositionHeader)
	  + ((step_begin - entry->step_begin) * sizeof(PositionRecord));

	int rc = ::fseek(file,
			 nseek,
			 SEEK_CUR);
	assert(rc == 0);

	PositionRecord *record_read = query->records + (step_begin - epoch_begin);
	size_t nrecords_read = step_end - step_begin + 1;

	size_t nread = ::fread(record_read,
			       sizeof(PositionRecord),
			       nrecords_read,
			       file);
	assert(nread == nrecords_read);

	query->steps_agent.begin = step_begin;
	query->steps_agent.end = step_end;

	::fclose(file);
}

//---------------------------------------------------------------------------
// PositionReader::initPopulationHistory
//---------------------------------------------------------------------------
void PositionReader::initPopulationHistory()
{
	population_history.step_end = 0;

	char pattern[1024];
	sprintf(pattern,
		"%s/*%s*%s",
		path_position_dir,
		PATH__BASENAME_POSITION,
		PATH__EXT);

	glob_t _glob;
	int rc = glob(pattern,
		      0,
		      NULL,
		      &_glob);
	if(rc != 0)
	{
		fprintf(stderr, "Invalid position directory\n");
		exit(1);
	}

	for(char **paths = _glob.gl_pathv;
	    *paths != NULL;
	    paths++)
	{
		const char *path = *paths;
		int num = 0;
		int place = 1;

		for(const char *wrk = path + strlen(path) - strlen(PATH__EXT) - 1;
		    *wrk >= '0' && *wrk <= '9';
		    wrk--, place *= 10)
		{
			num += (*wrk - '0') * place;
		}  

		// now read the position file to get first and last step
		FILE *file = open(num);
		
		// --- verify proper file format ---
		PositionHeader header;
		size_t rc = ::fread(&header,
				    sizeof(header),
				    1,
				    file);
		assert(rc == 1);

		assert(header.version == POSITION_HEADER_VERSION);
		assert(header.sizeof_header = sizeof(header));
		assert(header.sizeof_record = sizeof(PositionRecord));

		// --- read first step ---
		PositionRecord record;
		rc = ::fread(&record,
			     sizeof(record),
			     1,
			     file);

		long step_begin = record.step;

		// --- read last step ---
		int rc_seek = ::fseek(file,
				      -sizeof(record),
				      SEEK_END);
		assert(rc_seek == 0);

		rc = ::fread(&record,
			     sizeof(record),
			     1,
			     file);
		assert(rc == 1);

		long step_end = record.step;
		
		//printf("%ld [%ld,%ld]\n", num, step_begin, step_end);

		::fclose(file);

		PopulationHistoryEntry *entry = new PopulationHistoryEntry();
		entry->agent_number = num;
		entry->step_begin = step_begin;
		entry->step_end = step_end;

		population_history.numberLookup[num] = entry;
		population_history.step_end = max(population_history.step_end,
						  step_end);
	}
}

//---------------------------------------------------------------------------
// PositionReader::open
//---------------------------------------------------------------------------
FILE *PositionReader::open(long agent_number)
{
#define PATH(COMPLETE) PositionPath::position_file(path_position_dir,	\
						   agent_number,	\
						   COMPLETE).c_str()

	FILE *file = ::fopen(PATH(false),
			     "r");
	if(file == NULL)
	{
		file = ::fopen(PATH(true),
			       "r");
		if(file == NULL)
		{
			fprintf(stderr,
				"Failed opening position file for id %ld in dir %s\n",
				agent_number,
				path_position_dir);
			exit(1);
		}
	}

	return file;
}

/*
//---------------------------------------------------------------------------
// PositionReader::getEpochAttributes
//---------------------------------------------------------------------------
void PositionReader::getEpochAttributes(PositionEpochAttributes *epoch_attrs)
{
	epoch_attrs->step_begin = epoch_header.step_begin;
	epoch_attrs->step_end = epoch_header.step_end;
	epoch_attrs->agent_numbers.clear();

	char pattern[1024];
	sprintf(pattern,
		"%s/%s%s*%s",
		path_epoch_dir,
		PATH__PREFIX_EPOCH,
		PATH__BASENAME_POSITION,
		PATH__EXT);

	glob_t _glob;
	int rc = glob(pattern,
		      0,
		      NULL,
		      &_glob);
	assert(rc == 0);

	for(char **paths = _glob.gl_pathv;
	    *paths != NULL;
	    paths++)
	{
		const char *path = *paths;
		int num = 0;
		int place = 1;

		for(const char *wrk = path + strlen(path) - strlen(PATH__EXT) - 1;
		    *wrk >= '0' && *wrk <= '9';
		    wrk--, place *= 10)
		{
			num += (*wrk - '0') * place;
		}  

		epoch_attrs->agent_numbers.push_back(num);
	}

	globfree(&_glob);

	::sort(epoch_attrs->agent_numbers.begin(),
	       epoch_attrs->agent_numbers.end());
}
*/


// eof
