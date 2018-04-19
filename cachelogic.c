#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

/*Bit masking function*/
unsigned createMask(unsigned a, unsigned b){
   unsigned r = 0; 
   for(unsigned i = a; i <= b; i++){
      r |= 1 << i; 
   }
   return r; 
}

/*A data structure to hold the values of the tag, index, and offset*/
typedef struct{
 	unsigned tagAddress; 
 	unsigned indexAddress; 
 	unsigned offsetAddress; 
} TIO;

/*Calculates the bit length of the tag*/
int getTagSize(const int* indexBits, const int* offsetBits){
	return 32 - *indexBits - *offsetBits;
}

/*Calculates the index length of the index*/
int getIndexSize(){
	/*Based on the number of sets, returns the number of bits that can represent that value*/
	//Keep in mind of the zero index
	if(set_count == 1){
		//2^0 = 1, 0 bit representation
		//Value 1 is represented by "0" bits (0)
		return 0; 
	}
	if(set_count == 2){
		//2^1 = 2, 1 bit representation
		//Value 2 is represented by "1" bits (0 or 1)
		return 1; 
	}
	if(set_count > 2 && set_count <= 4){
		//2^2 = 4, 2 bit represenation
		//Value 0-3 < 4 and is represented by 2 bits also
		return 2;
	}
	if(set_count > 4 && set_count <= 8){
		//2^3 = 8, 3 bit representation
		//Value 0-7 < 8 and is represented by 3 bits also
		return 3; 
	}
	if(set_count > 8 && set_count <= 16){
		//2^4 = 16, 4 bit representation
		//Values 0-15 < 16 and is represnted by 4 bits also
		return 4; 
	}
	else{
		
		return 0; 
	}
	/* need to verify logic with Daneil*/
}

/*Calculates the bit length of the offset*/
int getOffsetSize(){
	return uint_log2(block_size);
}

/*This funciton is responsible for finding the values of the tag, index, and offset*/
TIO getTIO(address addr){
	TIO t; 

	int tagBits = 0; 
	int indexBits = 0; 
	int offsetBits = 0; 

	indexBits = getIndexSize();
	offsetBits = getOffsetSize();
	tagBits = getTagSize(&indexBits, &offsetBits);

	t.tagAddress = createMask(indexBits + offsetBits, 31);
	t.tagAddress = t.tagAddress & addr; 
	t.tagAddress = t.tagAddress >> (indexBits + offsetBits);

	t.indexAddress = createMask(indexBits - 1, (indexBits + offsetBits) -1);
	t.indexAddress = t.indexAddress & addr; 
	t.indexAddress = t.indexAddress >> (indexBits -1);

	t.offsetAddress = createMask(0, offsetBits - 1);
	t.offsetAddress = t.offsetAddress & addr; 

	return t; 
}

void handleSetAssociativity1(address addr, word* data, WriteEnable we){
	//The point of this is once the set is accessed, find the block containing the same tag
	//Index address refers to which set 
	TIO t = getTIO(addr);

}
void handleSetAssociativity2(address addr, word* data, WriteEnable we){

}
void handleSetAssociativity3(address addr, word* data, WriteEnable we){

}
void handleSetAssociativity4(address addr, word* data, WriteEnable we){

}
void handleSetAssociativity5(address addr, word* data, WriteEnable we){

}
void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */


  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  else if(assoc == 1){
  	handleSetAssociativity1(addr, data, we);
  	return; 
  }
  else if(assoc == 2){
  	handleSetAssociativity2(addr, data, we);
  	return;
  }
  else if(assoc == 3){
  	handleSetAssociativity3(addr, data, we);
  	return; 
  }
  else if(assoc == 4){
  	handleSetAssociativity4(addr, data, we);
  	return; 
  }
  else if(assoc == 5){
  	handleSetAssociativity5(addr, data, we);
  	return; 
  }
  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
