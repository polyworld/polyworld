#define POSITION_WRITER_CP

#include "PositionWriter.h"

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "PositionPath.h"
#include "PositionRecord.h"

using namespace std;

//---------------------------------------------------------------------------
// Misc Macros
//---------------------------------------------------------------------------
#define CHECK_RECORD() if(!record) return
#define PF_SET(AGENT, PF) (AGENT)->external.position_recorder = PF
#define PF(AGENT) ((PositionFile *)(AGENT)->external.position_recorder)

#define FOR_EACH(VAR, STMT)						\
  agent* VAR;								\
									\
  objectxsortedlist::gXSortedObjects.reset();				\
  while(objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE,		\
						   (gobject**)&(VAR)))	\
    {									\
      STMT;								\
    }									\

#define	DIR_MODE ( S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH )

//---------------------------------------------------------------------------
// Trivial Utility Functions
//---------------------------------------------------------------------------

static void __mkdir(const char *path)
{
	int rc = ::mkdir(path,
			 DIR_MODE);
	if(rc != 0)
	{
		assert(errno == EEXIST);
	}
}


//===========================================================================
// PositionWriter
//===========================================================================

//---------------------------------------------------------------------------
// PositionWriter::PositionWriter
//---------------------------------------------------------------------------
PositionWriter::PositionWriter(bool *_record)
: record(*_record)
{
}


//---------------------------------------------------------------------------
// PositionWriter::~PositionWriter
//---------------------------------------------------------------------------
PositionWriter::~PositionWriter()
{
}

//---------------------------------------------------------------------------
// PositionWriter::birth
//---------------------------------------------------------------------------
void PositionWriter::birth(long step,
			   agent *c)
{
	CHECK_RECORD();

	PF_SET(c, new PositionFile(step,
				   c));
}

//---------------------------------------------------------------------------
// PositionWriter::death
//---------------------------------------------------------------------------
void PositionWriter::death(long,
			   agent *c)
{
	CHECK_RECORD();

	PositionFile *pf = PF(c);

	delete pf;

	PF_SET(c, NULL);
}

//---------------------------------------------------------------------------
// PositionWriter::step
//---------------------------------------------------------------------------
void PositionWriter::step(long step)
{
	CHECK_RECORD();

	if(step == 1)
	{
		string run = PATH__DIR_RUN"/";
		
		__mkdir((run + PATH__DIR_MOTION).c_str());
		__mkdir((run + PATH__DIR_POSITION).c_str());
	}

	FOR_EACH(c, PF(c)->step(step));
}

#undef PATH


//===========================================================================
// PositionFile
//===========================================================================

#define PATH(COMPLETE) PositionPath::position_file(PositionPath::position_dir().c_str(), \
						   c->Number(), 		 \
						   COMPLETE).c_str()

//---------------------------------------------------------------------------
// PositionFile::PositionFile
//---------------------------------------------------------------------------
PositionFile::PositionFile(long step_birth,
			   agent *c)
{
	this->step_birth = step_birth;
	this->c = c;
	this->file = NULL;
}

//---------------------------------------------------------------------------
// PositionFile::~PositionFile
//---------------------------------------------------------------------------
PositionFile::~PositionFile()
{
	if(file)
	{
		::fclose(file);
		file = NULL;

		int rc = ::rename(PATH(false),
				  PATH(true));
		if(rc != 0)
		{
			error(0, "Failed renaming ", PATH(false));
		}
	}
}

//---------------------------------------------------------------------------
// PositionFile::step
//---------------------------------------------------------------------------
void PositionFile::step(long step)
{
	if(file == NULL)
	{
		file = ::fopen(PATH(false), "a");
		if(!file)
		{
			error(3, "Failed opening/creating ", PATH(false));
		}

		if(ftell(file) == 0)
		{
			PositionHeader header;
			header.version = POSITION_HEADER_VERSION;
			header.sizeof_header = sizeof(PositionHeader);
			header.sizeof_record = sizeof(PositionRecord);

			size_t rc = ::fwrite(&header,
					     sizeof(header),
					     1,
					     file);
			assert(rc == 1);
		}
	}

	PositionRecord record = {step,
				 c->X(),
				 c->Y(),
				 c->Z()};

	size_t rc = ::fwrite(&record,
			     sizeof(record),
			     1,
			     file);
	assert(rc == 1);

	::fflush(file);
}

// eof
