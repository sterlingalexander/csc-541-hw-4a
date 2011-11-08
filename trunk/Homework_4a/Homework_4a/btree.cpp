#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#include "filereader.h"
#include "str.h"
#include <iostream>

using std::cin;
using std::cout;

int ROOT = 0;											// BTree root node global
const int NODE_SIZE = sizeof(int) * 33 + sizeof(long) * 33;	// node size, system independent

struct bt_node {
	int n;				// number of keys in the node
	int key[32];		// array for record keys
	long child[33];		// array for child node offsets
};

struct qobj {			// queue object for printing
	long offset;		// offset of record
	int key[32];		// array for keys
	qobj *prev;			// prev pointer
	qobj *next;			// next pointer
};

void readNode(filereader &index, bt_node &node);
void writeNode(filereader &index, bt_node &node);
long find(filereader &index, int key, bool addcall = false);
void print(filereader &index);
void split(char lineinput[], string &command, int &key);
long add(filereader &index, int key, int parentoffset);
void writedata(filereader &index);
int addKey(int key, bt_node &node);
void initializeNode(filereader &index, int offset);
long splitCurrentNode(filereader &index, bt_node &node, int key, int &median);
bool isleaf(bt_node &node);

void main(int argc, char* argv[])  {

	string fname = argv[1];					// Get passed in args
	filereader index;						// Create filereader object
	char lineinput[32] = {};				// Console input string
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

	initializeNode(index, ROOT);
//	writedata(index);

	while ( 1 )  {
		cin.getline(lineinput, 25);				// get a line of input from cin
		split(lineinput, command, key);			// split out the command and key if any
		if ( command == "add" )  {				// if it was an add command
			index.seek(ROOT, BEGIN);			// ensure index is in proper position
			add(index, key, index.offset());	// call add method with the key
		}
		else if ( command == "find" )  {		// if it was a find command
			totalfinds++;						// increment total finds
			QueryPerformanceCounter(&tstart);						// get start time
			index.seek(ROOT, BEGIN);								// set index to root node
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

long add(filereader &index, int key, int parentoffset)  {

	int offset = index.offset();
	bt_node node;
	readNode(index, node);
	int median = 0;
	
	if ( node.n < 32 && isleaf(node) )  {
		addKey(key, node);
		writeNode(index, node);
	}
	else if ( node.n == 32 )  {
		int newnodeoffset = splitCurrentNode(index, node, key, median);
		int loffset = index.offset();
		index.seek(0, END);
		int eof = index.offset();
		initializeNode(index, eof);
		bt_node pt;
		readNode(index, pt);
		int addpos = addKey(median, pt);
		pt.child[0] = offset;
		pt.child[1] = newnodeoffset;
		writeNode(index, pt);
		ROOT = index.offset();
		index.seek(loffset, BEGIN);	
	}
	else  {								// find offset and recurse add
		index.seek(find(index, key, true), BEGIN);
		add(index, key, offset);
		return offset;
	}
	index.seek(offset, BEGIN);
	return offset;
}

long splitCurrentNode(filereader &index, bt_node &node, int key, int &median)  {

	bt_node newnode;
	int overflow[33] = {};
	int offset = index.offset();
	bool flag = false;
	index.seek(0, END);
	int newnodeoffset = index.offset();

	for (int i = 0; i < 33; i++)  {
		if ( key > node.key[i] )  {
			overflow[i] = node.key[i];
		}
		else if ( flag == false )  {
			overflow[i] = key;
			flag = true;
		}
		else  {
			overflow[i] = node.key[i - 1];
		}
	}
	if ( flag == false )
		overflow[32] = key;

	node.n = 16;
	newnode.n = 16;
	for (int i = 16; i < 33; i++)  {
		newnode.key[i - 16] = node.key[i];
		newnode.child[i - 16] = node.child[i]; 
	}
	newnode.child[17] = -1;
	median = overflow[16];
	writeNode(index, newnode);

	index.seek(offset, BEGIN);
	writeNode(index, node);
	return newnodeoffset;
}

bool isleaf(bt_node &node)  {

	for (int i = 0; i < node.n; i++)  {
		if ( node.child[i] != -1 ) {
			return false;
		}
	}
	return true;
}

int addKey(int key, bt_node &node)  {

	int swap1 = 0;
	int swap2 = 0;
	int swappos = -1;

	if ( node.n == 0 ) {
		node.n++;
		node.key[0] = key;
		return node.n;
	}

	for (int i = 0; i < node.n; i++)  {
		if ( key > node.key[i] )
			;
		else  {
			swappos = i;
			break;
		}
	}
	if ( swappos == -1 )  {
		node.key[node.n] = key;
		node.n++;
		return node.n;
	}
	else  {
		swap1 = node.key[swappos];
		node.key[swappos] = key;
		for (int i = swappos + 1; i < node.n + 1; i++)  {
			swap2 = node.key[i];
			node.key[i] = swap1;
			swap1 = swap2;
		}
		node.n++;
	}
	return node.n;
}

long find(filereader &index, int key, bool addcall)  {		// index must be pointing to root node at first call

	int orig_offset = index.offset();
	bt_node node;
	readNode(index, node);
	int indexfound = 0;

	for ( int i = 0; i < node.n; i++ )  {
		if ( node.key[i] == key )  {
			if ( addcall == false )  {
				cout << "\nRecord " << key << " exists.\n";
			}
			return -2;
		}
		else if ( key > node.key[i] ) {
			indexfound = i + 1;
		}
		else  {
			indexfound = i;
			break;
		}
	}

	if ( node.child[indexfound] == -1 )  {
		if ( addcall == false )  {
			cout << "\nRecord " << key << " does not exist.\n";
		}
		index.seek(orig_offset, BEGIN);
		return -1;
	}
	else  {
		index.seek(node.child[indexfound], BEGIN);
		return find( index, key, addcall);
	}
	index.seek(orig_offset, BEGIN);							// reset file index position
	return node.child[indexfound];
}

void readNode(filereader &index, bt_node &node)  {					// filereader must be in position when call is made
																	// filereader is always reset
	long offset = index.offset();
	index.read_raw( (char*) &node.n, sizeof(int) );
	for (int i = 0; i < 32; i++)  {
		index.read_raw( (char*) &node.key[i], sizeof(int) );
	}
	for (int i = 0; i < 33; i++)  {
		index.read_raw( (char*) &node.child[i], sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

void writeNode(filereader &index, bt_node &node)  {					// filereader must be in position when call is made
																	// filereader is always reset
	long offset = index.offset();
	index.write_raw( (char*) &node.n, sizeof(int) );
	for (int i = 0; i < 32; i++)  {
		index.write_raw( (char*) &node.key[i], sizeof(int) );
	}
	for (int i = 0; i < 33; i++)  {
		index.write_raw( (char*) &node.child[i], sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

void print(filereader &index)  {
	cout << "STUB\n\n";
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

void writedata(filereader &index)  {

	index.seek(0,BEGIN);
	int n = 32;
	int inttmp = 0;
	long lngtmp = -1;
	long p2 = NODE_SIZE;

	index.write_raw( (char*) &n, sizeof(int) );

	for (int i = 0; i < 32; i++)  {
		inttmp = (i + 1) * 100;
		index.write_raw( (char*) &inttmp, sizeof(int) );
	}
	index.write_raw( (char*) &p2, sizeof(long) );
	for (int i = 0; i < 32; i++)  {
		index.write_raw( (char*) &lngtmp, sizeof(long) );
	}
	index.seek(NODE_SIZE, BEGIN);
	index.write_raw( (char*) &n, sizeof(int) );
	for (int i = 0; i < 32; i++)  {
		inttmp = (i + 1) * 1;
		index.write_raw( (char*) &inttmp, sizeof(int) );
	}
	for (int i = 0; i < 33; i++)  {
		index.write_raw( (char*) &lngtmp, sizeof(long) );
	}
	index.seek(0, BEGIN);
}

void initializeNode(filereader &index, int offset)  {				// always returns index at position of beginning of new node

	int zero = 0;
	long minusone = -1;
	index.seek(offset, BEGIN);
	index.write_raw( (char*) &zero, sizeof(int) );

	for ( int i = 0; i < 32; i++ ) {
		index.write_raw( (char*) &zero, sizeof(int) );
	}
	for ( int i = 0; i < 33; i++)  {
		index.write_raw( (char*) &minusone, sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

/*
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
void initializeRoot(filereader &index);


void initializeNode(filereader &index, int offset)  {				// always returns index at position of beginning of new node

	int zero = 0;
	long minusone = -1;
	index.seek(offset, BEGIN);
	index.write_raw( (char*) &zero, sizeof(int) );

	for ( int i = 0; i < 32; i++ ) {
		index.write_raw( (char*) &zero, sizeof(int) );
	}
	for ( int i = 0; i < 33; i++)  {
		index.write_raw( (char*) &minusone, sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

void readNode(filereader &index, bt_node &node)  {					// filereader must be in position when call is made
																	// filereader is always reset
	long offset = index.offset();
	index.read_raw( (char*) &node.n, sizeof(int) );
	for (int i = 0; i < 32; i++)  {
		index.read_raw( (char*) &node.key[i], sizeof(int) );
	}
	for (int i = 0; i < 33; i++)  {
		index.read_raw( (char*) &node.child[i], sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

void writeNode(filereader &index, bt_node &node)  {					// filereader must be in position when call is made
																	// filereader is always reset
	long offset = index.offset();
	index.write_raw( (char*) &node.n, sizeof(int) );
	for (int i = 0; i < 32; i++)  {
		index.write_raw( (char*) &node.key[i], sizeof(int) );
	}
	for (int i = 0; i < 33; i++)  {
		index.write_raw( (char*) &node.child[i], sizeof(long) );
	}
	index.seek(offset, BEGIN);
}

int add(filereader &index, int to_insert)  {
	
	int path[33] = {};
	int pathsize = 0;
	bt_node node;
	index.seek(ROOT, BEGIN);
	readNode(index, node);
	int parentoffset = -1;

	while ( 1 )  {
	
		if ( node.child[0] == -1 )  {				// if the node is a leaf
			insert(index, to_insert, node, parentoffset, path, pathsize);
		}
	
	}
}

void insert(filereader &index, int key, bt_node &node, int parentoffset, int path[], int &pathsize)  {

	int pos = 0;
	int temp = 0;
	int temp2 = 0;

	if (node.n == 0)  {							// this ONLY happens at the initial root insert
		node.key[0] = key;
		node.n++;
	}
	else if (node.n < 32)  {							// if the node is not full
		node.n++;
		for (int i = 0; i < node.n; i++)  {
			if ( key < node.key[i] )  {
				temp = node.key[i];
				node.key[i] = key;
				for (int j = i + 1; j < node.n + 1; j++)  {
					temp2 = node.key[j];
					node.key[j] = temp;
					temp = temp2;
				}
			return;
			}
			else  ;
		}
	}
	else  {
		int overload[33] = {};
		int overloadpl[34] = {};
		int pos = 0;
		int temp = 0;
		int temp2 = 0;
		int tmp = 0;
		int tmp2 = 0;

		while (key < node.key[pos]) {
			overload[pos] = node.key[pos];
			overloadpl[pos] = node.child[pos];
			pos++;
		}
		temp = key;
		tmp = node.child[pos];
		for(int i = pos; i < 33; i++)  {
			temp2 = node.key[i];
			node.key[i] = temp;
			temp = temp2;
			tmp2 = node.child[i];
			node.child[i] = tmp;
			tmp = tmp2;
		}
		long eof_offset = size(index);				// get the offset for EOF
		int median = overload[17];
		bt_node node2;
		node2.n = 16;
		node.n = 16;
		for (int i = 17; i < 33; i++)  {
			node2.key[i - 17] = overload[i];
		}
		node2.child[0] = eof_offset;
		for (int i = 0; i < 17; i++ )  {
			node2.child[i + 1] = overloadpl[i + 17];
		}
		
		initializeNode(index, eof_offset);			// create new node

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


*/