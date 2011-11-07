//  FILEREADER.H
//    Class definition for FILEREADER.C

#ifndef _filereader_h
#define _filereader_h


#include <fstream>
#include "str.h"
using std::istream;
using std::ostream;

typedef enum {				// Enumerate valid seek modes
  BEGIN,				// From beginning of file
  CUR,					// From current offset
  END					// From end of file
} seek_mode;


class filereader {
  private:
    typedef enum {			// Enumerate valid filereader modes
      APPEND,				// Append to existing file
      READ,				// Read from file
      WRITE,				// Write to (create) file
      READ_WRITE,			// Read and write file
      UNDEF				// Undefined mode
    } fmode;

    istream      *inp_fp;		// Input file stream
    ostream      *out_fp;		// Output file stream
    fmode         mode;			// Current filereader mode

  public:
    filereader();			// Constructor
   ~filereader();			// Destructor

    void  clear();			// Clear error state
    void  close();			// Close input/output stream
    int   eof();			// Report end-of-file state
    int   getline(			// Read next line
            string& buf,		//  Storage for line
            int empty = 1		//  Allow empty line flag
          );
    int   next(				// Read and tokenize line
            string buf[],		//  Array to hold tokens
            int size,			//  Size of token array
            char *sep = " \t",		//  Token seperator(s), default=" \t"
            char *quo = "\"",		//  Quote separator(s), default="\""
            int eat = 1			//  Eat consecutive separators flag
          );
    long  offset();			// Return file offset
    int   open(				// Open from cin, or to cout
            char m			//  Mode (r=cin, w=cout)
          );
    int   open(				// Open file
            string fname,		//  Name of file to open
            char m			//  Mode (r=read, w=write, ..
					//  ..a=append, x=read/write)
          );
    int   read_raw(			// Read raw bytes from input stream
            char buf[],			//  Input buffer
            int len			//  Length of buffer
          );
    void  seek(				// Seek file pointer within file
            long off,			//  Offset into file
            seek_mode pos		//  Position to seek from
          );
    void  write(			// Write to output stream
            string line			//  Line to write
          );
    void  write_raw(			// Write raw bytes to output stream
            char buf[],			//  Buffer to write
            int len			//  Length of buffer
          );
};

#endif
