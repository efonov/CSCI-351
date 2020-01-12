//Elliot Young (elliotyoung)
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

//print help info
void printHelpInfo(void){
	printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

typedef struct cacheLine{
	int valid;
	unsigned long int tag;
	int address;
	int time;
}cacheLine;




typedef struct cacheAccess{
	char operation;
	unsigned long int address;
	unsigned int size;
	int time;
}cacheAccess;

typedef struct results{
	int hits;
	int misses;
	int evictions;
}results;

typedef struct cache{
	cacheLine **cache;
	int sets;
	int linesPerSet;
	int blockSize;
	int setIndexBits;
	int blockOffset;
	results results;
}cache;

void printCache(cache cache){
	int i, j;
	for(i = 0; i < cache.sets; i++){
		printf("set %d:", i);
		for(j = 0; j < cache.linesPerSet; j++){
			printf(" %lx ", cache.cache[i][j].tag);
		}
		printf("\n");
	}
}

cache doCache(cache cache, cacheAccess access){
	//isolate the set index by masking it with set setIndexBits
	unsigned int set = (access.address >> cache.blockOffset) & ~(~0 << cache.setIndexBits);
	//isolate the tag (t = 64 - b - s)
	unsigned long int tag = access.address >> (cache.blockOffset + cache.setIndexBits);
	
	//printf("Raw Address: %lx (Set Index: %d, Tag: %lx)\n", access.address, set, tag);

	int i;
	int l = cache.linesPerSet;

	if(access.operation == 'M') cache.results.hits ++;

	//check for a hit in the set
	for(i = 0; i < l; i++){
		if(cache.cache[set][i].valid == 1 && cache.cache[set][i].tag == tag){
			cache.cache[set][i].time = access.time;
			cache.results.hits ++;
			return cache;
		}
		continue;
	}

	//no hit:

	//check if the set is full, if there is an empty spot update the vacant line and count a miss
	for(i = 0; i < l; i++){
		if(cache.cache[set][i].valid == 0){
			cache.cache[set][i].tag = tag;
			cache.cache[set][i].valid = 1;
			cache.cache[set][i].time = access.time;
			cache.results.misses ++;
			return cache;
		}
		continue;
	}

	//set is full:

	//evict the oldest line and replace it with a new valid tag and access time (LRU logic)
	for(i = 0; i < l; i++){
			int oldest = 0;
			for(i = 0; i < cache.linesPerSet; i++){
				if(cache.cache[set][i].time < cache.cache[set][oldest].time){
					oldest = i;
				}
			}
			cache.cache[set][oldest].time = access.time;
			cache.cache[set][oldest].tag = tag;
			cache.results.misses++;
			cache.results.evictions ++;
			return cache;
	}
	printCache(cache);
	return cache;
}


int main(int argc, char **argv){

	//local variables:
	results results = {.hits = 0, .misses = 0, .evictions = 0};
	char options;
	char *tracePath;
	int helpFlag = 0, setIndexBits = 0, linesPerSet = 0, blockOffset = 0, trace = 0;

	//parse options
	while((options = getopt(argc, argv, "hvs:E:b:t:")) != -1){
		switch(options){

			case 'h' :
				helpFlag = 1;
				break;

			case 's' :
				setIndexBits = atoi(optarg);
				//printf("Sets: %d\n", (int)pow(setIndexBits, 2));
				break;

			case 'E' :
				linesPerSet = atoi(optarg);
				//printf("Lines/Set: %d\n", linesPerSet);
				break;

			case 'b' :
				blockOffset = atoi(optarg);
				//printf("Block Offset: %d\n", blockOffset);
				break;

			case 't' :
				trace = 1;
				tracePath = optarg;
				break;

			default:
				helpFlag = 1;
				break;

		}
	}
	if (helpFlag == 1){
        printHelpInfo();
        exit(0);
    }

    if(setIndexBits == 0 || blockOffset == 0 || linesPerSet == 0){

    	printf("enter valid arguements!");
    	exit(0);
    }

	//build the cache
    int sets = (int)pow(2, setIndexBits);
	int sizeOfCache = sets * linesPerSet * 64;
	int blockSize = (int)pow(2, blockOffset);
	cacheLine **tempCache;

	tempCache = (cacheLine **)calloc(1, sizeof(cacheLine *) * sets);

	for(int i = 0; i < sets; i++){
		tempCache[i] = (cacheLine *)calloc(1, (sizeof(cacheLine) * linesPerSet));
	}

	if(tempCache == NULL){
		printf("malloc of size %d failed!\n", sizeOfCache);
		exit(0);
	}

	//fill the array with empty/invalid cache lines
	else{
		for(int set = 0; set < sets; set++){
			for(int line = 0; line < linesPerSet; line++){
				cacheLine current = {.valid = 0, .tag = 0xffffffff, .address = 0, .time = 0}; 
				tempCache[set][line] = current;
			}
		}
	}
	cache finalCache = {.cache = tempCache, .sets = sets, .linesPerSet = linesPerSet, .blockSize = blockSize, setIndexBits = setIndexBits, .blockOffset = blockOffset, .results = results};
	//printCache(finalCache);

	//read the trace file
	if(trace == 1){
		cacheAccess access;
		char memAccess[80];
		char *currentMemAccess = NULL;
		FILE *traceFile = fopen(tracePath, "r");
		int time = 0;
		//printf("Trace: %s \n", tracePath);

		while(fgets(memAccess, 80, traceFile) != NULL){
			currentMemAccess = memAccess;
			if (*currentMemAccess++ == 'I')
				continue;
			sscanf(currentMemAccess, "%c %lx, %u", &access.operation, &access.address, &access.size);
			access.time = time++;
			finalCache = doCache(finalCache, access);
			// printCache(finalCache);
			// printf("\n");
		}
	}

	for(int i = 0; i < sets; i++){
		free(tempCache[i]);
		tempCache[i] = NULL;
	}
	free(finalCache.cache);
	finalCache.cache = NULL;

	//print the answer
	printSummary(finalCache.results.hits, finalCache.results.misses, finalCache.results.evictions);
	return 0;
}
