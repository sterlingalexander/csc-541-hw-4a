#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#include "filereader.h"
#include "str.h"
#include <iostream>

using std::cin;
using std::cout;

int ROOT = 0;											// BTree root node global
const int NODE_SIZE = sizeof(int) + sizeof(long) * 2;	// node size, system independent

struct qobj {			// queue object for printing
	int key;			// record key
	long lp;			// left pointer (offset)
	long rp;			// right pointer (offset)
	long offset;		// offset of record
	qobj *prev;			// prev pointer
	qobj *next;			// next pointer
};

long find(filereader &index, int record);							// searches for target key
int add(filereader &index, int to_insert);							// adds key to index
void writeKey(filereader &index, int key);							// writes key to index
void readNode(filereader &index, int &key, long &lp, long &rp);		// reads node from index
void readNode(filereader &index, qobj *read);						// reads node from index
long size(filereader &index);										// gets file size
void print(filereader &index);										// prints current tree
void split(char lineinput[], string &command, int &key);			// splits input commands
void printQueue(qobj *head, qobj *tail, int count);					// prints current queue
void addQueue(qobj *insert, qobj *head, qobj *tail);				// adds nodes to queue
void refillQueue(filereader &index, qobj *head, qobj *tail);		// adds next tree level to queue

void main(int argc, char* argv[])  {

	string fname = argv[1];					// Get passed in args
	filereader index;						// Create filereader object
	char lineinput[32] = {};					// Console input string
	string command = "";					// String object to parse from command line
	int key = 0;							// Key to be added to index
	string token[2] = {};					// array of strings to hold tokens

	index.open(fname, 'w');					// Open file to truncate
	index.close();							// Close opened file
	index.open(fname, 'x');					// Open file in RW mode
	LARGE_INTEGER freq;						// Var for clock frequency
	QueryPerformanceFrequency(&freq);		// get clock frequency to convert HPT to seconds
	LARGE_INTEGER tstart, tfinish, tdiff;	// Start time, Finish time, difference in time
	double ttotal = 0;						// Total recorded time
	int totalfinds = 0;						// Total number of times the find() routine is called
	double telapsed = 0;					// Accumulator for elapsed time

	while ( 1 )  {
		cin.getline(lineinput, 25);			// get a line of input from cin
		split(lineinput, command, key);		// split out the command and key if any
		if ( command == "add" )  {			// if it was an add command
			add(index, key);				// call add method with the key
		}
		else if ( command == "find" )  {	// if it was a find command
			totalfinds++;					// increment total finds
			QueryPerformanceCounter(&tstart);						// get start time
			find(index, key);										// perform the find
			QueryPerformanceCounter(&tfinish);						// get the end time
			tdiff.QuadPart = tfinish.QuadPart - tstart.QuadPart;	// get the time difference
			telapsed = tdiff.QuadPart / (double) freq.QuadPart;		// convert to actual time
			ttotal += telapsed;										// accumulate total time in find method
		}
		else if ( command == "print" )  {	// if it was a print commmand
			print(index);					// call print method
		}
		else if ( command == "end" )  {						// if it was an end command
			cout << '\n';									// format output
			printf("Sum: %.6f\n", ttotal);					// print total time for finds
			printf("Avg: %.6f\n", ttotal / totalfinds);		// print average time for finds
			exit(0);
		}
		else  {								// generic error handler for malformed input
			cout << "An error occurred, THIS SHOULD NEVER HAPPEN WITH WELL FORMED INPUT.\n\nTERMINATING\n\n";
			exit(1);
		}
	}
}

int add(filereader &index, int to_insert)  {
	
	long eof_offset = size(index);				// get the offset for EOF

	if ( eof_offset == 0 )  {					// if the file is empty
		writeKey(index, to_insert);				// write the key and return success value
		return 1;
	}

	int key = 0;								// initialize key
	long lp = 0;								// initialize lp
	long rp = 0;								// initialize rp
	index.seek(ROOT, BEGIN);					// seek to beginning of file
	readNode(index, key, lp, rp);				// read first record

	while ( 1 ) {
		if ( to_insert < key )  {				// if the new key is less than the key to insert
			if ( lp < 0 ) {						// if there is not another node at the lp
				index.seek(-8, CUR);			// seek to lp value
				index.write_raw( (char*) &eof_offset, sizeof(long) );	// write the offset of the new key to be inserted
				writeKey(index, to_insert);								// insert the new key
				return 1;												// return success
			}
			else {								// otherwise
				index.seek(lp, BEGIN);			// seek to the node at the lp
				readNode(index, key, lp, rp);	// read the node and repeat
			}
		}
		else if ( to_insert > key )  {			// if the new key is greater than the key to insert
			if ( rp < 0 ) {						// if there is not another node at the rp
				index.seek(-4, CUR);			// seek to the rp value
				index.write_raw( (char*) &eof_offset, sizeof(long) );	// write the offset of the new key to be inserted
				writeKey(index, to_insert);								// write the new key
				return 1;												// return success
			}
			else {								// otherwise
				index.seek(rp, BEGIN);			// seek to the node at the rp
				readNode(index, key, lp, rp);	// read the node and repeat
			}
		}
	}
}

long find(filereader &index, int target)  {

	long offset = 0;					// initialize variables
	int key = 0;
	long lp = 0;
	long rp = 0;

	index.seek(ROOT, BEGIN);			// seek to root node of file

	while ( 1 ) {
		readNode(index, key, lp, rp);	// read current node
		index.seek(-NODE_SIZE, CUR);	// move back to current node position
		if ( target < key )  {			// if the target is less than current node key
			if ( lp > 0) {				// and there is a valid lp
			index.seek(lp, BEGIN);		// seek to the lp and repeat
			}
			else  {						// otherwise the target does not exist, print and return failure offset
				cout << "Record " << target << " does not exist.\n";
				return -1;
			}
		}
		else if ( target > key )  {		// if the target is greater than current node key
			if ( rp > 0 ) {				// and there is a valid rp
			index.seek(rp, BEGIN);		// seek to the rp and repeat
			}
			else  {						// otherwise the target does not exist, print and return failure offset
				cout << "Record " << target << " does not exist.\n";
				return -1;
			}
		}
		else  {							// this is the equals case, print and return offset
			cout << "Record " << key << " exists.\n";
			return index.offset();
		}
	}
}

void readNode(filereader &index, int &key, long &lp, long &rp)  {
	index.read_raw( (char*) &key, sizeof(int) );	// read key
	index.read_raw( (char*) &lp, sizeof(long) );	// read lp
	index.read_raw( (char*) &rp, sizeof(long) );	// read rp
}

void readNode(filereader &index, qobj *read)  {
	int offset = index.offset();					// record offset
	int key = 0;									// initialize key
	long lp = 0;									// initialize lp
	long rp = 0;									// initialize rp
	index.read_raw( (char*) &key, sizeof(int) );	// read key
	index.read_raw( (char*) &lp, sizeof(long) );	// read lp
	index.read_raw( (char*) &rp, sizeof(long) );	// read rp

	read->key = key;  read->lp = lp; read->rp = rp; read->offset = offset;	// set values of node to read values
}

void writeKey(filereader &index, int key)  {

	long minusone = -1;										// var to hold long -1 value
	index.seek(0, END);										// seek to file end
	index.write_raw( (char*) &key, sizeof(int));			// write key
	index.write_raw( (char*) &minusone, sizeof(long) );		// write null offset
	index.write_raw( (char*) &minusone, sizeof(long) );		// write null offset
}

long size(filereader &index)  {

	index.seek(0, END);			// seek to the end of the file
	return index.offset();		// get the offset (file size)
}

void print(filereader &index)  {

	qobj *head = new qobj;		// create head node
	qobj *tail = new qobj;		// create tail node
	qobj *curr = new qobj;		// create current node
	qobj *left = NULL;			// create pointer for left offset
	qobj *right = NULL;			// create pointer for right offset
	head->prev = NULL;			// head anchor previous is null
	head->next = tail;			// head anchor next is tail
	tail->next = NULL;			// tail anchor next is null
	tail->prev = head;			// tail anchor prev is head
	index.seek(ROOT, BEGIN);	// seek to the tree root
	int count = 0;				// var for tree level

	readNode(index, curr);		// prime the queue
	addQueue(curr, head, tail);	// add root node to queue
	cout << "\n";				// formatting output

	while ( head->next != tail )  {			// while the queue is not empty
		count ++;							// set the new level
		printQueue(head, tail, count);		// print the queue
		refillQueue(index, head, tail);		// refill the queue with the next level
	}
}

void refillQueue(filereader &index, qobj *head, qobj *tail)  {

	qobj *mark = tail->prev;				// marker for "end of tree level" node
	qobj *curr = head->next;				// current pointer
	qobj *left = new qobj;					// new qobj for left child node
	qobj *right = new qobj;					// new qobj for right child node
	qobj *delptr = NULL;					// pointer for objects to delete

	do  {		// while our current pointer is not beyond our marker for this level
		if (curr->lp >= 0)  {
			index.seek(curr->lp, BEGIN);	// seek to left object position
			readNode(index, left);			// read information for left object
			addQueue(left,head, tail);
		}
		if (curr->rp >= 0)  {
			index.seek(curr->rp, BEGIN);	// seek to right object position
			readNode(index, right);			// read information for right object
			addQueue(right, head, tail);	// add right object to queue
		}
//		printQueue(head, tail);
		delptr = curr;					// get a pointer to object to delete
		head->next = curr->next;		// reset head to new next node
		curr = curr->next;				// move current node pointer
		if (delptr != mark)
			delete delptr;				// delete removed object
		left = new qobj;				// craete new objects to insert
		right = new qobj;				// create new objects to insert
	} while ( curr != mark->next );
}

void printQueue(qobj *head, qobj *tail, int count)  {

	qobj *curr = head->next;								// current pointer in queue

	cout << count << ": ";									// formatting output
	while ( curr != tail )  {								// while we are not at the queue end
		cout << curr->key << "/" << curr->offset << " ";	// print queue values
		curr = curr->next;									// move to next node
	}
	cout << '\n';											// formatting output
}

void addQueue(qobj *insert, qobj *head, qobj *tail)  {

	qobj *prev = tail->prev;		// temp pointer to prev node
	insert->next = tail;			// set inserted nodes next pointer
	insert->prev = tail->prev;		// set inserted nodes prev pointer
	tail->prev = insert;			// set tail anchor prev pointer
	if (head->next == tail)			// if the queue was empty
		head->next = insert;		// set the head appropriately
	else
		prev->next = insert;		// otherwise, set next pointer of prev node to new node
}

void split(char lineinput[], string &command, int &key)  {

	int delim = 0;			// index of delimiter
	string tmp = "";		// temp string
	command = "";			// clear command string
	long size;				// size of string, counts while iterating

	for (size = 0; size < lineinput[size] != '\0'; size++)  {	// iterate string
		if ( lineinput[size] == ' ')  {				// find delimiters
			delim = size;							// set delimiter position
		}
	}

	if (delim == 0 )  {			// we have a command with no space, no key to add
		for (int j = 0; lineinput[j] != '\0'; j++)  {
			command += lineinput[j];		// copy command to command variable
		}
		key = 0;							// set key to nonsense value
		return;
	}

	for ( int j = 0; j < delim; j++ )  {			// otherwise, parse command
		command += lineinput[j];
	}
	for ( int j = delim + 1; j < size; j++ )  {		// them parse key
		tmp += lineinput[j];
	}
	key = atoi(tmp);								// convert key from string to int
}
