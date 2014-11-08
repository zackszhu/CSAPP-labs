// 5130379013 Zhu Chaojie

#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#define WRONG_PARA 1
#define MISS_PARA 2

typedef struct {
	long valid; // 0 represent in-valid, otherwise the sequence
	long tag;
} Line;

typedef struct {
	Line *line;
	long lastLine;
} Set;

typedef Set* Cache;

typedef int bool;

#define true 1
#define false 0

long hit = 0;
long miss = 0;
long eviction = 0;

long setBit = 0;
long blockBit = 0;
long setNum = 0;
long lineNum = 0;
long blockSize = 0;
char* traceName = 0;
bool v = 0;
Cache cache;

long getSet(long address) {
	return (address >> blockBit) & ((1 << setBit) - 1);
}

long getTag(long address) {
	return (address >> (setBit + blockBit)) & ((1 << (32 - (setBit + blockBit))) - 1);
}

void setValid(Line* _lines, long _line) {
	long i = 0;
	for (i = 0; i < lineNum; i++) {
		if (_lines[i].valid >= 1)
			_lines[i].valid++;
	}
	_lines[_line].valid = 1;
}

bool updateLast(Set* _set) {
	long i, maxValid = 0;;
	for (i = 0; i < lineNum; i++) {
		if (_set->line[i].valid == 0) {
			_set->lastLine = i;
			return true;
		}
		else if (_set->line[i].valid > maxValid) {
			maxValid = _set->line[i].valid;
			_set->lastLine = i;
		} 
	}
	return false;
}

void addHit() {
	++hit;
	printf(" hit");
}

void addMiss() {
	++miss;
	printf(" miss");
}

void addEvic() {
	++eviction;
	printf(" eviction");
}

void work(Set* _set, char op, long newTag) {
	if (op == 'I')
		return;
	long i;
	for (i = 0; i < lineNum; i++) {
		if (_set->line[i].valid > 0 && _set->line[i].tag == newTag) {
			addHit();			
			if (op == 'M') {
				addHit();
			}
			setValid(_set->line, i);
			printf("\n");
			return;
		}
	}
	addMiss();
	if (!updateLast(_set)) {
		addEvic();
	}
	setValid(_set->line, _set->lastLine);
	_set->line[_set->lastLine].tag = newTag;
	if (op == 'M'){
		addHit();
	}
	printf("\n");
}

void parse() {
	FILE *trace;
	trace = fopen(traceName, "r");
	if (!trace) {
		printf("No such trace!\n");
		return;
	}
	char op;
	long address, size;
	while (fscanf(trace, " %c %lx, %lx", &op, &address, &size) > 0) {
		if (v)
			printf("%c %lx, %lx", op, address, size);
		work(&cache[getSet(address)], op, getTag(address));
	}
	fclose(trace);
}

void initCache() {
	cache = (Cache)malloc(setNum * sizeof(Set));
	long i = 0;
	for (i = 0; i < setNum; i++) {
		cache[i].line = (Line*)malloc(lineNum * sizeof(Line));
		cache[i].lastLine = 0;
		long j;
		for (j = 0; j < lineNum; j++) {
			cache[i].line[j].valid = 0;
			cache[i].line[j].tag = 0;
		}
	}
}

int main(int argc, char* argv[])
{
	char op;
	opterr = 0;
	while ((op = getopt(argc, argv, "vs:E:b:t:")) != -1) {
		switch (op) {
			case 'v':	v = true;
						break;
			case 's':	setBit = atoi(optarg);
						break;
			case 'E':	lineNum = atoi(optarg);
						break;
			case 'b':	blockBit = atoi(optarg);
						break;
			case 't':	printf("b\n");
						traceName = optarg;
						printf("c\n");
						break;
			default:	printf("Wrong Parametre!\n");
						break;
		}
	}
	setNum = 1 << setBit;
	blockSize = 1 << blockBit;
	if (setNum == 0 || lineNum == 0 || blockSize == 0 || traceName == 0) {
		printf("Missing parametre!\n");
		return 1;
	}
	initCache();
	parse();
    printSummary(hit, miss, eviction);
    return 0;
}
