//  STRING.H
//    Class definition for STRING.C

#ifndef _string_h
#define _string_h

#include <iostream>
#include <fstream>
using std::istream;
using std::ostream;
using std::cout;
using std::cin;
using std::cerr;


class string {
  private:
    char  BOUNDS_ERR;			// Used for out of bounds error
    int   m_size;			// Maximum length of string
    int   size;				// Length of string
    char *str;				// Character array for string

  public:
    string();				// Constructor
    string(				// Constructor (copy constructor)
      const string& s			//  String object to construct from
    );
    string(				// Constructor (copy constructor)
      const char *s			//  Character array to construct from
    );
    string(				// Constructor (copy constructor)
      const char c			//  Character to construct from
    );
   ~string();				// Destructor

    int      is_int();			// String is integer test
    int      is_float();		// String is float test
    int      len() const;		// Length of string
    int      strstr(			// Index of first occurance of substr
               const string& sub	//  Substring to search for
             );
    string   substr(			// Return substring of string
               int a,			//  Beginning of substring
               int b			//  End of substring
             );
    int      token(			// Tokenize string
               string buf[],		//  Array to store tokens
               int n,			//  Size of array
               char *sep = " \t",	//  Separator character(s)
               char *quote = "\"",	//  Quote character(s)
               int eat = 1		//  Eat consecutive separators flag
             );

    int      operator==(		// Equality operator
               const string& s		//  String to compare with
             );
    void     operator+=(		// Concatenate operator
               const string& s		//  String to concatenate
             );
    int      operator!=(		// Inequality operator
               const string& s		//  String to compare with
             );
    int      operator<=(		// Less than or equal operator
               const string& s		//  String to compare with
             );
    int      operator>=(		// Greater than or equal operator
               const string& s		//  String to compare with
             );
    int      operator<(			// Less than operator
               const string& s		//  String to compare with
             );
    int      operator>(			// Greater than operator
               const string& s		//  String to compare with
             );
    char&    operator[](		// Index operator
               int i			//  Index to access
             );
    string&  operator=(			// Assignment operator
               const string& s		//  String to compare with
             );
    string   operator+(			// Append string operator
               const string& s		//  String to compare with
             );
    string   operator+(			// Append character operator
               const char& c		//  String to compare with
             );

    int      operator==( const char *s ) { return operator==((string) s ); }
    void     operator+=( const char *s ) { return operator+=((string) s ); }
    int      operator!=( const char *s ) { return operator!=((string) s ); }
    int      operator<=( const char *s ) { return operator<=((string) s ); }
    int      operator>=( const char *s ) { return operator>=((string) s ); }
    int      operator>( const char *s ) { return operator>((string) s ); }
    int      operator<( const char *s ) { return operator<((string) s ); }
    string&  operator=( const char *s ) { return operator=((string) s ); }
    string   operator+( const char *s ) { return operator+((string) s ); }

    operator char*() const;		// Cast to character string
    operator unsigned char*() const;	// Cast to character string
    operator float() const;		// Cast to float
    operator int() const;		// Cast to integer

    friend ostream&  operator<<(	// Redirect string
               ostream& stream,		//  Output stream to redirect to
               const string& s		//  String to redirect
             );
    friend istream&  operator>>(	// Redirect into string
               istream& stream,		//  Input stream to redirect into
               string& s		//  String to redirect
             );
};

#endif
