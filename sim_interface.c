#define _POSIX_C_SOURCE (200112L)

#include "sim_interface.h"

#include <assert.h>
#include <stdlib.h>

/*
 * Simulator interface
 */

APX_ARRAY ALLOCATED_APX_ARRAYS[MAX_APX_BUFFERS];
size_t N_APX_BUFFERS = 0;

void __attribute__ ((noinline)) sim_mark_compressed(APX_ARRAY* arr){
	printf("\n\nBENCHMARK: Mapped approx region 0x%.16lX of size %u  [ %f, %f ]\n\n", (uint64_t) arr->buf, arr->size, arr->min, arr->max);

	if (N_APX_BUFFERS < MAX_APX_BUFFERS) {
		ALLOCATED_APX_ARRAYS[N_APX_BUFFERS] = *arr;
		N_APX_BUFFERS++;
	}

	return;
}

void __attribute__ ((noinline)) sim_mark_remap(APX_ARRAY* arr, const uint32_t* axis_order){

	printf("\n\nBENCHMARK: Mapped region for remapping 0x%.16lX of size %u  [ %f, %f ]\n\n", (uint64_t) arr->buf, arr->size, arr->min, arr->max);

	printf("Automatic reordering: [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", arr->dims[d]);
	}
	printf(" ] -- [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", axis_order[d]);
	}
	printf(" ] --> [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", arr->dims[axis_order[d]]);
	}
	printf(" ]\n");

	return;
}

void* sim_alloc_buffer(size_t s, uint32_t ndims, const uint32_t* dims, float min, float max, bool approx) {
	if (approx) {
		return sim_alloc_approx_buffer(s, ndims, dims, min, max);
	} else {
		void* ret;
		assert(!posix_memalign((void**) &(ret), PAGE_SIZE, s));
		return ret;
	}
}
void* sim_alloc_approx_buffer(size_t s, uint32_t ndims, const uint32_t* dims, float min, float max) {
  size_t pages = (s + PAGE_SIZE - 1) / PAGE_SIZE;
  s = pages * PAGE_SIZE;

	float* ret = NULL;
	assert(!posix_memalign((void**) &(ret), PAGE_SIZE, s));

	// By default, 1D array
	uint32_t len = s / sizeof(float);
	if (ndims == 0) {
		ndims = 1;
		dims = &len;
	}

	APX_ARRAY arr;
	arr.buf = ret;
	arr.size = s;
	arr.ndims = ndims;
	for (size_t d=0; d<ndims; ++d) arr.dims[d] = dims[d];
	arr.min = min;
	arr.max = max;

	sim_mark_compressed(&arr);

	uint32_t axis_order[MAX_APX_DIMS];
	if (sim_auto_reorder(&arr, axis_order)) {
		sim_mark_remap(&arr, axis_order);
	}

	return ret;
}

void __attribute__ ((noinline)) pin_start_roi(){
	printf("\n\nBENCHMARK: pin_start_roi\n\n");
	return;
}

void __attribute__ ((noinline)) pin_end_roi(){
	printf("\n\nBENCHMARK: pin_end_roi\n\n");
	return;
}







/*
 *
 * Utility functions (old sim_util.h)
 *
 */


const char* SIM_INPUT_FILE;
const char* SIM_OUTPUT_FILE;
uint64_t SIM_APX_LEVEL;
uint64_t SIM_CODE_LEVEL;

void sim_parse_cmdline(int argc, const char* argv[]) {
	SIM_INPUT_FILE = "input.data";
	SIM_OUTPUT_FILE = "output.data";
	SIM_CODE_LEVEL = 0;
	SIM_APX_LEVEL = 0xFFFFFFFFFFFFFFFF;

	if (argc >= 2) SIM_INPUT_FILE = argv[1];
	if (argc >= 3) SIM_OUTPUT_FILE = argv[2];
	if (argc >= 4) SIM_APX_LEVEL = atol(argv[3]);
}

void sim_dump_buffer(const void* buf, size_t size, uint32_t ndims, const uint32_t* dims) {

	const char* filename = getenv("SIM_DUMP_FILE");
	if (filename == NULL) return;


	uint32_t len = size / sizeof(float);
	// Default to 1D array
	if (ndims == 0) {
		ndims = 1;
		dims = &len;
	}



	// CAll the function multiple times to append more arrays
	static int opened = 0;
	static FILE* f = NULL;

	if (!opened) {
		sim_dump_clear(filename);
		f = fopen(filename, "a");
		opened = 1;
	}

	if (!f) return;

	fwrite(&len, sizeof(uint32_t), 1, f);
	fwrite(&ndims, sizeof(uint32_t), 1, f);
	fwrite(dims, sizeof(uint32_t), ndims, f);


	//fwrite(buf, sizeof(float), len, f);

	// We must write one value at a time for the memory reorganization to work.
	for (size_t i=0; i<len; ++i) {
		float val = ((float*) buf)[i];
		fwrite(&val, sizeof(float), 1, f);
	}

}

void sim_dump_clear(const char* filename) {
	FILE* f = fopen(filename, "w");
	uint32_t magic = 0xDEADBEE1;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	assert(fclose(f) == 0);
}

void sim_dump_buffer_f(const char* filename, void* buf, size_t size, uint32_t ndims, const uint32_t* dims) {

	uint32_t len = size / sizeof(float);
	// Default to 1D array
	if (ndims == 0) {
		ndims = 1;
		dims = &len;
	}


	if (filename == NULL) {
		sim_dump_buffer(buf, size, ndims, dims);
		return;
	}


	// CAll the function multiple times to append more arrays
	FILE* f = fopen(filename, "a");
	if (!f) return;


	fwrite(&len, sizeof(uint32_t), 1, f);

	fwrite(&ndims, sizeof(uint32_t), 1, f);
	fwrite(dims, sizeof(uint32_t), ndims, f);

	//for (int d=0; d<20; ++d) printf("DUMP_F: %f\n", ((float*) buf)[d]);

	// We must write one value at a time for the memory reorganization to work.
	for (size_t i=0; i<len; ++i) {
		float val = ((float*) buf)[i];
		fwrite(&val, sizeof(float), 1, f);
	}

	fclose(f);
}

// Dump every buffer that has been marked approximate.
void sim_dump_all(const char* filename) {
	if (filename != NULL) {
		sim_dump_clear(filename);
	}

	for (size_t i=0; i< N_APX_BUFFERS; ++i) {
		sim_dump_buffer_f(filename,
		                  ALLOCATED_APX_ARRAYS[i].buf,
											ALLOCATED_APX_ARRAYS[i].size,
											ALLOCATED_APX_ARRAYS[i].ndims,
											ALLOCATED_APX_ARRAYS[i].dims);
	}

}


// At point code_level in the program, should approximation be applied?
bool sim_approx_at_l(uint64_t code_level, uint64_t apx_level) {
	return apx_level & (1 << code_level);
}

bool sim_approx_at(uint64_t code_level) {
	return sim_approx_at_l(code_level, SIM_APX_LEVEL);
}

bool sim_approx_here() {
	return sim_approx_at_l(SIM_CODE_LEVEL, SIM_APX_LEVEL);
}




/*
 * Attempt to automatically select a better axis ordering for a multi-dimensional array
 */

static bool _is_good_axis(size_t v) {
	if (v < 16) return false;

	// Check if it's an even power of 2
	while (v != 0) {
		if (v & 0x01) return v == 1;
		v = v >> 1;
	}

	return false;
}

bool __attribute__ ((noinline)) sim_auto_reorder(APX_ARRAY* arr, uint32_t* axis_order) {
	// No point in linear arrays.
	if (arr->ndims < 2) return false;

	// Start with the current order of the axes, as chosen by the application
	for (size_t d=0; d<arr->ndims; ++d) axis_order[d] = d;

	// Starting from the front, swap every non-power-of-two axis with a better one
	bool did_anything = false;
	size_t candidate = 0;
	for (size_t d = 0; d<arr->ndims; ++d) {
		size_t ax = axis_order[d];
		//printf("Swap o[%lu] ? ax = %lu [%lu]\n", d, ax, arr->dims[ax]);

		// Don't swap axes we're already happy with
		if (_is_good_axis(arr->dims[ax])) continue;

		// Don't swap with axes we've already improved.
		if (candidate < d) candidate = d+1;

		// Find a good candidate axis to swap with
		for (; candidate<arr->ndims; ++candidate) {
			size_t cax = axis_order[candidate];
			if (_is_good_axis(arr->dims[cax])) break;
		}

		// We've run out of axes to swap with.
		if (candidate >= arr->ndims) break;

		// Swap them so that the nice power-of-two axis comes before the non-nice one
		axis_order[d] = axis_order[candidate];
		axis_order[candidate] = ax;
		did_anything = true;
	}

	printf("Automatic reordering: [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", arr->dims[d]);
	}
	printf(" ] -- [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", axis_order[d]);
	}
	printf(" ] --> [ ");
	for (size_t d=0; d<arr->ndims; ++d) {
		if (d != 0) printf(", ");
		printf("%u", arr->dims[axis_order[d]]);
	}
	printf(" ]\n");

	return did_anything;
}

