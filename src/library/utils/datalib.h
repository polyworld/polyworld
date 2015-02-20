#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>
#if __cplusplus >= 201103L
	#include <functional>
#else
	#include <tr1/functional>
#endif


#include "misc.h"
#include "Variant.h"

// ================================================================================
// ===
// === NAMESPACE datalib
// ===
// ================================================================================
namespace datalib
{
	// ------------------------------------------------------------
	// --- ENUM Type
	// ---
	// --- Column datatype
	// ------------------------------------------------------------
	enum Type
	{
		INVALID,
		INT,
		FLOAT,
		STRING,
		BOOL
	};

	// ------------------------------------------------------------
	// --- CLASS __Table
	// ---
	// --- For internal use
	// ------------------------------------------------------------
	class __Table
	{
	public:
		__Table()
		{
			offset = rowlen = nrows = 0;
		}
		__Table( const char *name )
		{
			this->name = name;
			offset = rowlen = nrows = 0;
		}
		std::string name;
		size_t offset;
		size_t data;
		size_t rowlen;
		size_t nrows;
	};

	typedef std::vector<__Table> __TableVector;
	typedef std::map<std::string, __Table> __TableMap;

	// ------------------------------------------------------------
	// --- CLASS __Column
	// ---
	// --- For internal use
	// ------------------------------------------------------------
	class __Column
	{
	public:
		__Column();
		__Column( const char *name,
				  datalib::Type type,
				  const char *format,
				  bool pad );

		std::string name;
		datalib::Type type;
		Variant rowdata;

		const char *tname;
		const char *format;
	};

	typedef std::vector<__Column> __ColVector;
	typedef std::map<std::string, __Column *> __ColMap;
};

// ================================================================================
// ===
// === CLASS DataLibWriter
// ===
// ================================================================================
class DataLibWriter
{
 public:
	DataLibWriter( const char *path,
				   bool randomAccess = false,
				   bool singleSchema = true );
	~DataLibWriter();

	void beginTable( const char *name,
					 std::vector<std::string> &colnames,
					 std::vector<datalib::Type> &coltypes );
	void beginTable( const char *name,
					 const char *colnames[],
					 const datalib::Type coltypes[],
					 const char *colformats[] = NULL );
	void addRow( Variant col0, ... );
	void addRow( Variant *cols );
	void endTable();
	void flush();

 private:
	void fileHeader();
	void fileFooter();
	void tableHeader();
	void tableFooter();
	void colMetaData();

 private:
	FILE *f;
	bool randomAccess;
	bool singleSchema;
	datalib::__TableVector tables;
	datalib::__Table *table;
	datalib::__ColVector cols;
};


// ================================================================================
// ===
// === CLASS DataLibReader
// ===
// ================================================================================
class DataLibReader
{
 public:
	DataLibReader( const char *path );
	~DataLibReader();

	bool seekTable( const char *name );
	void rewindTable();
	size_t nrows();
	void seekRow( int index );
	bool nextRow();
	const Variant &col( const char *name );


 private:
	void parseHeader();
	void parseDigest();
	void parseTableHeader();
	void parseLine( const char *line,
#if __cplusplus >= 201103L
					std::function<void (const char *start,
#else
					std::tr1::function<void (const char *start,
#endif
											 const char *end)> callback);

 private:
	FILE *f;
	int row;
	datalib::__TableMap tables;
	datalib::__Table *table;
	datalib::__ColVector cols;
	datalib::__ColMap colmap;
	std::string path;
};
