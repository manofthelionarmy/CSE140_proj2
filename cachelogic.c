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


//typedef unsigned int label; 
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
	return uint_log2(set_count);
}

/*Calculates the bit length of the offset*/
int getOffsetSize(){
	return uint_log2(block_size);
}
unsigned int getByte_size(unsigned int b){
	if(b == 4){
		return 2; 
	}
	if(b == 8){
		return 3; 
	}
	if(b == 16){
		return 4; 
	}
	if(b == 32){
		return 5; 
	}
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

	t.indexAddress = createMask(offsetBits, (indexBits + offsetBits) -1);
	t.indexAddress = t.indexAddress & addr; 
	t.indexAddress = t.indexAddress >> (indexBits + offsetBits) -1;

	t.offsetAddress = createMask(0, offsetBits - 1);
	t.offsetAddress = t.offsetAddress & addr; 

	return t; 
}

void handleRead(address addr, word* data, TIO t){

	unsigned int lru_index = 0; 
	unsigned int lru_value = 0; 
	address oldAddress = 0; 

	TransferUnit byte_size = 0; 

  	byte_size = getByte_size(block_size);

	CacheAction label = MISS; 
	for(int i = 0; i < assoc; ++i){
		if(t.tagAddress == cache[t.indexAddress].block[i].tag && cache[t.indexAddress].block[i].valid == 1){
			label = HIT; 
			cache[t.indexAddress].block[i].lru.value = 0; 
			cache[t.indexAddress].block[i].valid = 1; 
			//Read from cache to memory!! (4 bytes = 1 word)
			memcpy(data, (cache[t.indexAddress].block[i].data + t.offsetAddress), 4);
			return; 
		}
	}

	if(label == MISS){

		//Finds the LRU block
		if(policy == LRU){
			for(int i = 0; i < assoc; ++i){
				if(lru_value < cache[t.indexAddress].block[i].lru.value){
					lru_index = i; 
					lru_value = cache[t.indexAddress].block[i].lru.value; 
				}
				
			}
		}
		if(policy == RANDOM){
			//Selects the block we wish to replace randomly
			lru_index = randomint(assoc);
		}
		if(cache[t.indexAddress].block[lru_index].dirty == DIRTY){
			unsigned int indexLength = getIndexSize();
			unsigned int offsetLength = getOffsetSize();

			oldAddress = cache[t.indexAddress].block[lru_index].tag << (indexLength + offsetLength) + (t.indexAddress << offsetLength);
			accessDRAM(oldAddress, (cache[t.indexAddress].block[lru_index].data), byte_size, WRITE);
		}

		accessDRAM(addr, (cache[t.indexAddress].block[lru_index].data), byte_size, READ);
		cache[t.indexAddress].block[lru_index].lru.value = 0; 
		cache[t.indexAddress].block[lru_index].valid = 1; 
		cache[t.indexAddress].block[lru_index].dirty = VIRGIN;
		cache[t.indexAddress].block[lru_index].tag = t.tagAddress; 

		memcpy(data, (cache[t.indexAddress].block[lru_index].data + t.offsetAddress), 4);
	}

}
void handleWriteBack(address addr, word* data, TIO t){

	CacheAction label = MISS; 
	unsigned int lru_index = 0; 
	unsigned int lru_value = 0; 
	unsigned int indexLength = 0; 
	unsigned int offsetLength = 0; 
	TransferUnit byte_size= 0; 
	address oldAddress = 0; 
	byte_size = getByte_size(block_size);
	for(int i = 0; i < assoc; ++i){
		if(t.tagAddress == cache[t.indexAddress].block[i].tag && cache[t.indexAddress].block[i].valid == 1){
			memcpy((cache[t.indexAddress].block[i].data + t.offsetAddress), data, 4);
			cache[t.indexAddress].block[i].dirty = DIRTY; 
			cache[t.indexAddress].block[i].lru.value = 0; 
			cache[t.indexAddress].block[i].valid = 1; 
			label = HIT; 
		}
	}

	if(label == MISS){
		if(policy  == LRU){
			for(int i = 0; i < assoc; i++){
				if(lru_value < cache[t.indexAddress].block[i].lru.value){
					lru_index = i; 
					lru_value = cache[t.indexAddress].block[i].lru.value; 
				}
			}
		}
		if(policy == RANDOM){
			lru_index = randomint(assoc);
		}
		if(cache[t.indexAddress].block[lru_index].dirty == DIRTY){
			indexLength = getIndexSize();
			offsetLength = getOffsetSize();
			oldAddress = cache[t.indexAddress].block[lru_index].tag << (indexLength + offsetLength) + (t.indexAddress << offsetLength); 
			accessDRAM(oldAddress, (cache[t.indexAddress].block[lru_index].data), byte_size, WRITE);

		}

		cache[t.indexAddress].block[lru_index].lru.value = 0; 
		cache[t.indexAddress].block[lru_index].valid = 1; 
		cache[t.indexAddress].block[lru_index].dirty = DIRTY; 
		cache[t.indexAddress].block[lru_index].tag = t.tagAddress; 

		accessDRAM(addr, (cache[t.indexAddress].block[lru_index].data), byte_size, READ);
		memcpy((cache[t.indexAddress].block[lru_index].data + t.offsetAddress), data, 4);
	}

}
void handleWriteThrough(address addr, word* data, TIO t){
	CacheAction label = MISS; 
	unsigned int lru_index = 0; 
	unsigned int lru_value = 0; 
	unsigned int indexLength = 0; 
	unsigned int offsetLength = 0; 
	TransferUnit byte_size= 0; 
	address oldAddress = 0; 

	for(int i = 0; i < assoc; ++i){
		if(t.tagAddress == cache[t.indexAddress].block[i].tag && cache[t.indexAddress].block[i].valid == 1){
			memcpy((cache[t.indexAddress].block[i].data + t.offsetAddress), data, 4);
			cache[t.indexAddress].block[i].dirty = VIRGIN; 
			cache[t.indexAddress].block[i].lru.value = 0; 
			cache[t.indexAddress].block[i].valid = 1; 
			label = HIT; 
			accessDRAM(addr, (cache[t.indexAddress].block[i].data), byte_size, WRITE);
		}
	}
	if(label == MISS){
		if(policy  == LRU){
			for(int i = 0; i < assoc; i++){
				if(lru_value < cache[t.indexAddress].block[i].lru.value){
					lru_index = i; 
					lru_value = cache[t.indexAddress].block[i].lru.value; 
				}
			}
		}
		if(policy == RANDOM){
			lru_index = randomint(assoc);
		}

		accessDRAM(addr, (cache[t.indexAddress].block[lru_index].data), byte_size, READ);
		cache[t.indexAddress].block[lru_index].lru.value = 0; 
		cache[t.indexAddress].block[lru_index].valid = 1; 
		cache[t.indexAddress].block[lru_index].dirty = VIRGIN; 
		cache[t.indexAddress].block[lru_index].tag = t.tagAddress; 

		memcpy((cache[t.indexAddress].block[lru_index].data + t.offsetAddress), data, 4);


	}
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
  
  TIO t = getTIO(addr);
  //Increments the LRU values. 
  if(policy == LRU){
  	for(int i = 0; i < assoc; ++i){
  		cache[t.indexAddress].block[i].lru.value++;
  	}
  }
  if(we == READ){
  	handleRead(addr, data, t);
  	return; 
  }
  if(memory_sync_policy == WRITE_BACK){
  	handleWriteBack(addr, data, t);
  	return; 
  }
  if(memory_sync_policy == WRITE_THROUGH){
  	handleWriteThrough(addr, data, t);
  	return; 
  }

 

}
