#pragma once

struct Variant
{
	enum Type
	{
		INVALID,
		INT,
		FLOAT,
		BOOL,
		STRING
	};

	Variant();
	Variant( int ival );
	Variant( long ival );
	Variant( float fval );
	Variant( double fval );
	Variant( bool bval );
	Variant( const char *sval );
	Variant( const Variant &variant );

	Variant &operator = ( const Variant &variant );

	~Variant();

	operator int () const;
	operator long () const;
	operator float () const;
	operator double () const;
	operator bool () const;
	operator const char *() const;

	Type type;
	union {
		int ival;
		float fval;
		bool bval;
		const char *sval;
	};
};
