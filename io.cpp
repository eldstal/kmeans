#include "io.h"

#include "sim_interface.h"

#include <iostream>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

using std::cerr;
using std::endl;

size_t load_dump_legacy(FILE* f, APX_ARRAY** dump) {
	if (!f) return 0;


	size_t total_arrays = 0;
	APX_ARRAY* DUMP = NULL;

	while (true) {
		uint32_t len;

		//cerr << "Array header at 0x" << std::hex << ftell(f) << std::dec << endl;
		size_t another = fread(&len, sizeof(uint32_t), 1, f);
		if (another == 0) break;

		size_t a = total_arrays;
		//cerr << "Array " << a << " : " << len << " elements." << endl;

		total_arrays++;

		DUMP = (APX_ARRAY*) realloc(DUMP, total_arrays * sizeof(APX_ARRAY));



		//DUMP[a].buf = (float*) malloc(len * sizeof(float));

		DUMP[a].ndims = 1;
		DUMP[a].dims[0] = len;

		DUMP[a].size = len * sizeof(float);
		DUMP[a].buf = (float*) sim_alloc_approx_buffer(DUMP[a].size, DUMP[a].ndims, DUMP[a].dims, DUMP[a].min, DUMP[a].max);

		if (!DUMP[a].buf) {
			fclose(f);
			return 0;
		}

		size_t read = 0;
		while (read < len) {
			float* next_buf = DUMP[a].buf + read;
			read += fread(next_buf, sizeof(float), len - read, f);
			if (feof(f)) break;
		}

		if (feof(f)) break;
	}
	fclose(f);

	*dump = DUMP;

	//cerr << "Loaded " << total_arrays << " arrays." << endl;
	return total_arrays;
}

size_t load_dump_v1(FILE* f, APX_ARRAY** dump) {
	if (!f) return 0;

	uint32_t magic;
	fread(&magic, sizeof(uint32_t), 1, f);

	if(magic != 0xDEADBEE1) return 0;

	size_t total_arrays = 0;
	APX_ARRAY* DUMP = NULL;

	while (true) {
		uint32_t len;

		//cerr << "Array header at 0x" << std::hex << ftell(f) << std::dec << endl;
		size_t another = fread(&len, sizeof(uint32_t), 1, f);
		if (another == 0) break;

		size_t a = total_arrays;
		//cerr << "Array " << a << " : " << len << " elements." << endl;

		total_arrays++;
		DUMP = (APX_ARRAY*) realloc(DUMP, total_arrays * sizeof(APX_ARRAY));

		fread(&(DUMP[a].ndims), sizeof(uint32_t), 1, f);

		//DUMP[a].buf = (float*) malloc(len * sizeof(float));
		//DUMP[a].size = len * sizeof(float);

		fread(DUMP[a].dims, sizeof(uint32_t), DUMP[a].ndims, f);

		DUMP[a].min = -500;
		DUMP[a].max = 500;

		DUMP[a].size = len * sizeof(float);
		DUMP[a].buf = (float*) sim_alloc_approx_buffer(DUMP[a].size, DUMP[a].ndims, DUMP[a].dims, DUMP[a].min, DUMP[a].max);

		if (!DUMP[a].buf) {
			fclose(f);
			return 0;
		}

		size_t read = 0;
		while (read < len) {
			float* next_buf = DUMP[a].buf + read;
			//read += fread(next_buf, sizeof(float), len - read, f);
			read += fread(next_buf, sizeof(float), 1, f);
			if (feof(f)) break;
		}

		if (feof(f)) break;
	}
	fclose(f);

	*dump = DUMP;

	//cerr << "Loaded " << total_arrays << " arrays." << endl;
	return total_arrays;
}


size_t load_dump(const char* filename, APX_ARRAY** dump) {
	FILE* f = fopen(filename, "r");
	if (!f) return 0;

	uint32_t magic;
	fread(&magic, sizeof(uint32_t), 1, f);
	rewind(f);


	if (magic == 0xDEADBEE1) {
		return load_dump_v1(f, dump);
	} else {
		cerr << "Loading legacy-format dump" << endl;
		return load_dump_legacy(f, dump);
	}


}

