#ifndef SIM_H
#define SIM_H

#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>

#define PAGE_SIZE 4096


#define MAX_APX_BUFFERS 1000
#define MAX_APX_DIMS 10

typedef struct {
	float* buf;
	uint32_t size;
	uint32_t ndims;
	uint32_t dims[MAX_APX_DIMS];
	float min;
	float max;
} APX_ARRAY;

//extern APX_ARRAY ALLOCATED_APX_ARRAYS[MAX_APX_BUFFERS];
extern size_t N_APX_BUFFERS;


void __attribute__ ((noinline)) pin_start_roi();

void __attribute__ ((noinline)) pin_end_roi();

bool __attribute__ ((noinline)) sim_auto_reorder(APX_ARRAY* arr, uint32_t* axis_order);
void __attribute__ ((noinline)) sim_mark_remap(APX_ARRAY* array, const uint32_t* axis_order);

// Set ndims to 0 and dims to NULL for default 1D array.
void* sim_alloc_approx_buffer(size_t s, uint32_t ndims, const uint32_t* dims, float min, float max);
void* sim_alloc_buffer(size_t s, uint32_t ndims, const uint32_t* dims, float min, float max, bool approx);
void __attribute__ ((noinline)) sim_mark_compressed(APX_ARRAY* array);


/*
 *
 * Utility functions (old sim_util.h)
 *
 */

// Set by sim_parse_cmdline
extern const char* SIM_INPUT_FILE;
extern const char* SIM_OUTPUT_FILE;

// Set by sim_parse_cmdline
// Increment SIM_CODE_LEVEL after allocating a block of
// approximate buffers. Use sim_approx_here() to determine if the current apx_level
// includes approximation of the block of buffers you're about to allocate
extern uint64_t SIM_APX_LEVEL;
extern uint64_t SIM_CODE_LEVEL;

void sim_parse_cmdline(int argc, const char* argv[]);

void sim_dump_buffer(const void* buf, size_t size, uint32_t ndims, const uint32_t* dims);

void sim_dump_clear(const char* filename);

void sim_dump_buffer_f(const char* filename, void* buf, size_t size, uint32_t ndims, const uint32_t* dims);

// Dump every buffer that has been marked approximate.
void sim_dump_all(const char* filename);


// At point code_level in the program, should approximation be applied?
bool sim_approx_at_l(uint64_t code_level, uint64_t apx_level);
bool sim_approx_at(uint64_t code_level);
bool sim_approx_here();

#endif
