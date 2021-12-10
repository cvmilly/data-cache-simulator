/**************************************************************
 *        Name:  Vivian Martinez                              *
 *       Class:  CDA3101                                      *
 *  Assignment:  Implementing a Data Cache Simulator          *
 *     Compile:  "gcc -g -o data_cache data.cpp"              *
 *                                                            *
 **************************************************************/


#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <math.h>
#include <vector>

using namespace std;

#define MAXNUMSETS               8192
#define MAXSETSIZE                  8
#define MINLINESIZE                 8
#define NUMBER_OF_BITS             32


enum state_rw { NONE = 0, READ = 1, WRITE = 2 };
enum state_df { EMPTY = 3, MISS = 4, HIT = 5 };


struct config
{
  int  set_cnt;
  int  set_size;
  int  line_size;
  bool valid;
};

// struct representing a line
struct line
{
  bool         dirtyBit;
  bool         validBit;
  unsigned int tag;
  int          cntLRU;
};

struct statistics
{
  int    hit;
  int    miss;
  int    access;
  double hitRatio;
  double missRatio;
};

struct reference
{
  int           numRef;
  enum state_rw accesstype;
  unsigned int  address;
  unsigned int  tag;
  unsigned int  index;
  unsigned int  offset;
  enum state_df df;
  int           memRef;
};


//
// functions declarations
//
void dumpCacheState(vector< vector<struct line> > *pCache);
void printCacheConfig(struct config *cfg);
void printResults(struct reference *ref);
void printStats(struct statistics *stats); 

bool isPowerOfTwo(int num);
void initCache(struct config *cfg, vector< vector<struct line> > *pCache);
void clearReference(struct reference *pRef);
struct config readConfig(string filename);
bool validateReferenceInput(int refNum, int line_size, int ref_size, int address, char access_type);
int findLRU(const vector< vector<line> > cache, int index, int set_size);
void process_read(vector< vector<line> > &cache, struct reference *ref, struct statistics *stats, int set_size);
void process_write(vector< vector<line> > &cache, struct reference *ref, struct statistics *stats, int set_size);





//
// main
//
int main(int argc, char *argv[])
{
  // to store config values
  struct config cfg;

  // read and validate configuration values
  cfg = readConfig("trace.config");
  if (!cfg.valid)
    {
      exit(1);
    }

  unsigned short set_offset = (unsigned short)log2(cfg.line_size);
  unsigned short set_index = (unsigned short)log2(cfg.set_cnt);

  // Initialize the vector of vectors representing the cache
  vector< vector<struct line> > dataCache;
  initCache(&cfg, &dataCache);
    
  struct reference ref { 0, NONE, 0, 0, 0, 0, EMPTY, 0 };
  printResults(&ref);
    
  // variable for summary printout
  struct statistics stats = { 0, 0, 0, 0.0, 0.0 };

  // to check EOF condition
  char sentinel = 'a';

  int  lineNum = 0;

  // take each line and get references
  while (sentinel != EOF)
    {
      // variables to get data from .dat file
      char         access_type;     // R or W for read or write
      int          ref_size;        // bytes trying to be accessed
      unsigned int address;         // address in hex

      // keep track of line of input
      lineNum++;

      scanf(" %c:%d:%x", &access_type, &ref_size, &address);

      // extract any white space and check for EOF
      cin >> ws;
      sentinel = cin.peek();

      ref.numRef++;
      if (!validateReferenceInput(ref.numRef, cfg.line_size, ref_size, address, access_type))
        {
	  ref.numRef--;
	  continue;
        }

      // populate ref;
      ref.accesstype = (access_type == 'R')? READ : WRITE;
      ref.address = address;

      // calculate offset, index, and tag values
      int displacement;

      // calculate ref.offset
      displacement = NUMBER_OF_BITS - set_offset;
      ref.offset = (ref.address << displacement) >> displacement;

      // calculate ref.index
      displacement = NUMBER_OF_BITS - set_index - set_offset;
      ref.index = (ref.address << displacement) >> (displacement + set_offset);

      // calculate ref.tag
      displacement = set_index + set_offset;
      ref.tag = ref.address >> displacement;

      //
      // checks whether it is a read or write and checks cache for hit or miss
      // and updates cache accordingly
      if (ref.accesstype == READ)
        {
	  process_read(dataCache, &ref, &stats, cfg.set_size);
        }
      else
        {
	  process_write(dataCache, &ref, &stats, cfg.set_size);
        }

      // print ref number, read/write, address, tag, index, offset, hit/miss, and memory reference
      printResults(&ref);

      // start next reference with initial values
      clearReference(&ref);

    }  // end of while loop

  // print summary information
  printStats(&stats);

  return 0;
}


//    
// return true if number given is a power of two
//
bool isPowerOfTwo(int num)
{
  return (num != 0) && ((num & (num - 1)) == 0);
}


//
// Initialize the vector of vectors representing the cache
//
void initCache(struct config *cfg, vector< vector<struct line> > *pCache)
{
  pCache->resize(cfg->set_cnt);
  for (unsigned int w = 0; w < pCache->size(); w++)
    {
      pCache->at(w).resize(cfg->set_size);
      for (int z = 0; z < cfg->set_size; z++)
        {
	  pCache->at(w).at(z).validBit = false;
	  pCache->at(w).at(z).tag = 0;
	  pCache->at(w).at(z).dirtyBit = false;
	  pCache->at(w).at(z).cntLRU = 0;
        }
    }
}


//
// clear all the values of the ref, for a new cycle
//
void clearReference(struct reference *pRef)
{
  // refNum cannot be modified here
  pRef->accesstype = NONE;
  pRef->address = 0;
  pRef->tag = 0;
  pRef->index = 0;
  pRef->offset = 0;
  pRef->df = EMPTY;
  pRef->memRef = 0;
}


//
// get values from config file plus validation
//
struct config readConfig(string filename)
{
  string token;
  ifstream ifs;

  struct config cfg = { 0, 0, 0, true };

  ifs.open(filename.c_str());
  if (!ifs)
    {
      cerr << "could not open trace.config  file in current directory" << endl;
      cfg.valid = false;
    }

  if (cfg.valid)
    {
      cout << "Cache Configuration\n\n";
      getline(ifs, token, ':');
      ifs >> cfg.set_cnt;
      ifs >> ws;
      getline(ifs, token, ':');
      ifs >> cfg.set_size;
      ifs >> ws;
      getline(ifs, token, ':');
      ifs >> cfg.line_size;

      ifs.close();
    }

  if (cfg.valid && cfg.set_cnt > MAXNUMSETS)
    {
      cerr << "error - number of sets requested exceeds MAXNUMSETS" << endl;
      cfg.valid = false;
    }

  // set size should not exceed MAXSETSIZE
  if (cfg.valid && cfg.set_size > MAXSETSIZE)
    {
      cerr << "error - set size requested exceeds MAXSETSIZE" << endl;
      cfg.valid = false;
    }

  if (cfg.valid)
    {
      cout << "   " << cfg.set_cnt << " " << cfg.set_size << "-way set associative entries\n";
      cout << "   of line size " << cfg.line_size << " bytes\n";
    }

  if (cfg.valid)
    {
      if (!isPowerOfTwo(cfg.set_cnt))
        {
	  cerr << "numsets not a power of two" << endl;
	  cfg.valid = false;
        }
    }

  // line size validation
  if (cfg.valid)
    {
      if (cfg.line_size < MINLINESIZE)
        {
	  cerr << "linesize too small" << endl;
	  cfg.valid = false;
        }
      if (cfg.valid && !isPowerOfTwo(cfg.line_size))
        {
	  cerr << "linesize not a power of two" << endl;
	  cfg.valid = false;
        }
    }
  if (cfg.valid)
    {
      cout << endl << endl;
    }
  return cfg;
}


//
// validate all reference input values
//
bool validateReferenceInput(int refNum, int line_size, int ref_size, int address, char access_type)
{
  bool valid = true;

  if (access_type != 'R' && access_type != 'W')
    {
      cout << "reference " << refNum << " has unknown access type " << access_type << endl;
      valid = false;
    }

  // reference size is 1, 2, 4 or 8
  if (valid && !(ref_size == 1 || ref_size == 2 || ref_size == 4 || ref_size == 8) 
      && (ref_size > line_size))
    {
      // print to stderr if the size is illegal
      cerr << "line " << refNum << " has illegal size " << ref_size << endl;
      valid = false;
    }

  // check alignment: if address is a multiple of reference size
  if (valid  && address % ref_size != 0)
    {
      // print to stderr if the reference is misaligned
      cerr << "line " << refNum << " has misaligned reference at address ";
      cerr << std::hex << address << " for size " << std::dec << ref_size << endl;
      valid = false;
    }
  return valid;
}


//
// use LRU algorithm
//
int findLRU(const vector< vector<line> > cache, int index, int set_size)
{
  // finds place where the LRU block is using a counter
  int lowNum = cache.at(index).at(0).cntLRU;
  int spot = 0;

  for (int k = 0; k < set_size; k++)
    {
      if (cache.at(index).at(k).cntLRU < lowNum)
        {
	  lowNum = cache.at(index).at(k).cntLRU;
	  spot = k;
        }
    }

  return spot;
};


//
// process reads
//
void process_read(vector< vector<line> > &cache, struct reference *ref, struct statistics *stats, int set_size)
{
  int k;
  int new_ndx;
  bool tracker = false;
  bool blockAvailable;

  // check if data is already in the cache
  for (k = 0; k < set_size && tracker == false; k++)
    {
      if (cache.at(ref->index).at(k).tag == ref->tag &&
	  cache.at(ref->index).at(k).validBit == true)
        {
	  ref->df = HIT;
	  cache.at(ref->index).at(k).cntLRU += 1;
	  tracker = true;
	  stats->hit++;
        }
    }
  if (!tracker)
    {
      // found miss
      ref->df = MISS;
      stats->miss++;
      blockAvailable = false;

      // for loop to see if there is an empty block to insert data
      for (k = 0; k < set_size && blockAvailable == false; k++)
        {
	  if (cache.at(ref->index).at(k).validBit == false)
            {
	      ref->memRef = 1;// takes 1 memory reference
	      cache.at(ref->index).at(k).validBit = true;
	      cache.at(ref->index).at(k).tag = ref->tag;
	      cache.at(ref->index).at(k).cntLRU = 0;
	      blockAvailable = true;
            }
        }

      // if there is no empty spot the LRU slot is replaced
      if (!blockAvailable)
        {
	  // calls function to find place LRU
	  new_ndx = findLRU(cache, ref->index, set_size);
	  ref->memRef = (cache.at(ref->index).at(new_ndx).dirtyBit == true) ? 2 : 1;
	  cache.at(ref->index).at(new_ndx).validBit = true;
	  cache.at(ref->index).at(new_ndx).tag = ref->tag;

	  // increase LRU counter
	  cache.at(ref->index).at(new_ndx).cntLRU += 1;
	  cache.at(ref->index).at(new_ndx).dirtyBit = false;
        }
    }
}


//
// process writes
//
void process_write(vector< vector<line> > &cache, struct reference *ref, struct statistics *stats, int set_size)
{
  int  k;
  int  new_ndx;
  bool track = false;
  bool blockAvailable = false;

  // for loop checks if data is already in cache
  for (k = 0; k < set_size; k++)
    {
      if (cache.at(ref->index).at(k).tag == ref->tag &&
	  cache.at(ref->index).at(k).validBit == true)
        {
	  ref->df = HIT;
	  stats->hit++;
	  cache.at(ref->index).at(k).cntLRU += 1;
	  cache.at(ref->index).at(k).dirtyBit = true;
	  track = true;
	  ref->memRef = 0;
        }
    }

  // if data is not already there then it is a miss
  if (!track)
    {
      ref->df = MISS;
      stats->miss++;
      blockAvailable = false;

      // for loop to check if there is empty block in index
      for (k = 0; k < set_size && blockAvailable == false; k++)
        {
	  if (cache.at(ref->index).at(k).validBit == false)
            {
	      ref->memRef = 1;
	      cache.at(ref->index).at(k).validBit = true;
	      cache.at(ref->index).at(k).tag = ref->tag;
	      cache.at(ref->index).at(k).dirtyBit = true;
	      cache.at(ref->index).at(k).cntLRU = 0;
	      blockAvailable = true;
            }
        }
      // if block for if slot needs to be replaced
      if (blockAvailable == false)
        {
	  new_ndx = findLRU(cache, ref->index, set_size);
	  ref->memRef = (cache.at(ref->index).at(new_ndx).dirtyBit == true) ? 2 : 1;
	  cache.at(ref->index).at(new_ndx).validBit = true;
	  cache.at(ref->index).at(new_ndx).tag = ref->tag;
	  cache.at(ref->index).at(new_ndx).cntLRU += 1;
        }
    }
}


//
// print results - references for each cycle
//
void printResults(struct reference *ref)
{
  string str;
  if (ref->numRef == 0)
    {
      cout << "Results for Each Reference\n\n";
      cout << "Ref  Access Address    Tag   Index Offset Result Memrefs\n";
      cout << "---- ------ -------- ------- ----- ------ ------ -------\n";
    }
  else
    {
      cout << setw(4) << right << ref->numRef << ' ';
      str = (ref->accesstype == READ) ? "read" : "write";
      cout << setw(6) << right << str << ' ';
      cout << setw(8) << right << std::hex << ref->address << ' ';
      cout << setw(7) << right << std::hex << ref->tag << ' ';
      cout << setw(5) << right << ref->index << ' ';
      cout << setw(6) << right << ref->offset << ' ';
      str = (ref->df == HIT) ? "hit" : "miss";
      cout << setw(6) << right << str << ' ';
      cout << setw(7) << right << ref->memRef << endl;
    }
};


//
// print cache config from config file
//
void printCacheConfig(struct config *cfg)
{
  cout << "Cache Configuration\n\n";
  cout << "   " << cfg->set_cnt << ' ' << cfg->set_size;
  cout << "-way set associative entries\n";
  cout << "   of line size " << cfg->line_size << " bytes\n\n\n";
};


//
// for debugging only
//
void dumpCacheState(vector< vector<struct line> > *pCache)
{
  cout << endl << endl;
  cout << "validBit       tag  dirtyBit  LRUCount\n";
  cout << "--------  --------  --------  --------\n";
  for (auto x = pCache->begin(); x != pCache->end(); x++)
    {
      for (auto y = x->begin(); y != x->end(); y++)
        {
	  cout << setw(8) << right << y->validBit << "  ";
	  cout << setw(8) << std::hex << right << y->tag << "  ";
	  cout << setw(8) << right << y->dirtyBit << "  ";
	  cout << setw(8) << right << y->cntLRU << endl;
        }
    }
  cout << endl;
};


//
// Print out summary statistics with the # hits, # misses, # of total
// accesses, hit ratio, miss ratio
//
void printStats(struct statistics *stats)
{
  cout << "\n\nSimulation Summary Statistics\n";
  cout << "-----------------------------\n";
  cout << "Total hits       : " << stats->hit << endl;
  cout << "Total misses     : " << stats->miss << endl;
  cout << "Total accesses   : " << stats->access << endl;

  // calculate the total access, hit ratio, and miss ratio
  stats->access = stats->hit + stats->miss;
  if (stats->access != 0.0)
    {
      stats->hitRatio = (stats->hit * 1.0) / stats->access;
      stats->missRatio = (stats->miss * 1.0) / stats->access;

      cout << "Hit ratio        : " << std::fixed << setprecision(6) << stats->hitRatio << endl;
      cout << "Miss ratio       : " << setprecision(6) << stats->missRatio << endl;
      cout << endl;
    }
};
