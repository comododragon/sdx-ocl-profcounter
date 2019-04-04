#define PREAMBLE(levels, levelsSz, levelsC, levelsCSz, edgeOffsets, edgeOffsetsSz, edgeList, edgeListSz, numVertices) {\
	int _i;\
	unsigned int _vars = 5;\
	char *_fileNames[] = {\
		"inputLevels",\
		"outputLevels",\
		"inputEdgeOffsets",\
		"inputEdgeList",\
		"inputNumVertices"\
	};\
	void *_varsPointers[] = {\
		levels,\
		levelsC,\
		edgeOffsets,\
		edgeList,\
		&numVertices\
	};\
	unsigned int _varsSizes[] = {\
		levelsSz,\
		levelsCSz,\
		edgeOffsetsSz,\
		edgeListSz,\
		1\
	};\
	unsigned int _varsTypeSizes[] = {\
		sizeof(unsigned int),\
		sizeof(unsigned int),\
		sizeof(unsigned int),\
		sizeof(unsigned int),\
		sizeof(unsigned int)\
	};\
\
	for(_i = 0; _i < _vars; _i++) {\
		FILE *ipf = fopen(_fileNames[_i], "rb");\
		fread(_varsPointers[_i], _varsTypeSizes[_i], _varsSizes[_i], ipf);\
		fclose(ipf);\
	}\
}
