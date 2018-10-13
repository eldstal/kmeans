#include <iostream>
#include <stdint.h>
#include <limits>

#include "sim_interface.h"
#include "io.h"

#include "size.h"

using namespace std;

float _abs(float a) { return a < 0 ? -a : a; }
float _min(float a, float b) { return a < b ? a : b; }

float dist(float a, float c) {
	return _abs(a - c);
}

struct State {
	APX_ARRAY* input;
	size_t LEN;

	uint8_t* CLUSTER;
	float* CENTROID;

	size_t assign();
	void update();
};


size_t State::assign() {
	size_t changes = 0;
	// Assign each element e to the closest centroid
	for (size_t e=0; e<LEN; ++e) {
		float a = input->buf[e];

		float mindist = std::numeric_limits<float>::infinity();
		uint8_t nearest_cluster = 0;
		for (size_t k=0; k<K; ++k) {
			float c = CENTROID[k];
			float d = dist(a, c);

			if (d < mindist) {
				nearest_cluster = k;
				mindist = d;
			}
		}

		if (nearest_cluster != CLUSTER[e]) {
			CLUSTER[e] = nearest_cluster;
			changes++;
		}
	}

	return changes;
}

void State::update() {
	float sum[K];
	size_t count[K];

	for (size_t k=0; k<K; ++k) {
		sum[k] = 0;
		count[k] = 0;
	}

	for (size_t e=0; e<LEN; ++e) {
		uint8_t cluster = CLUSTER[e];
		assert(cluster < K);
		sum[cluster] += input->buf[e];
		count[cluster]++;
	}

	for (size_t k=0; k<K; ++k) {
		count[k] = count[k] > 0 ? count[k] : 1;
		CENTROID[k] = sum[k] / count[k];
	}
}


int main(int argc, const char** argv) {

	sim_parse_cmdline(argc, argv);

	pin_start_roi();

	cerr << "Loading input..." << endl;
	APX_ARRAY* input_arrays;
	size_t n_arrays = load_dump(SIM_INPUT_FILE, &input_arrays);

	if (n_arrays != 1) {
		cerr << "Expected a single input array, found " << n_arrays << endl;
		return 1;
	}

	State state;

	state.input = &input_arrays[0];


	state.LEN = 1;
	for (size_t d=0; d<state.input->ndims; ++d) {
		state.LEN *= state.input->dims[d];
	}


	// For each element, the cluster to which it belongs
	state.CLUSTER = (uint8_t*) calloc(state.LEN, sizeof(uint8_t));

	// For each cluster, the value of its centroid
	state.CENTROID = (float*) calloc(K, sizeof(float));

	// Choose K elements in a predictable way as initial centroids
	cerr << "Choosing initial centroids" << endl;
	size_t step = (state.LEN - 1) / K;
	for (size_t i=0; i<K; ++i) state.CENTROID[i] = state.input->buf[i*step];

	// Starting assignment of each value to a cluster
	cerr << "Initial assignment" << endl;
	state.assign();

	cerr << "Initial assignment" << endl;
	size_t iterations = 0;
	bool done = false;
	while (!done && iterations < MAX_ITERATIONS) {

		state.update();
		size_t changes = state.assign();
		cerr << "Iteration " << iterations << ": " << changes << " updated elements." << endl;
		iterations++;

		done = changes == 0;
	}

	pin_end_roi();

	cerr << "Finished after " << iterations << " iterations." << endl;

	// Reformat the output so that it can be compared easily
	// We're really interested in the clustering, i.e. which elements
	// ended up in the same bucket. To stabilize against inversions of
	// the ORDER of buckets, we can sort them by centroid first.

	// This is an indirection. Position i contains the index of the ith largest centroid
	size_t CENT_ORDER[K];
	for (size_t k=0; k<K; ++k) CENT_ORDER[k] = k;

	// Bubble sort them
	cerr << "Sorting buckets..." << endl;
	while (true) {
		bool done = true;
		for (size_t k=1; k<K; ++k) {
			size_t il = CENT_ORDER[k];
			size_t ih = CENT_ORDER[k-1];
			float h = state.CENTROID[ih];
			float l = state.CENTROID[il];
			if (l > h) {
				CENT_ORDER[k-1] = il;
				CENT_ORDER[k] = ih;
				done = false;
			}
		}
		if (done) break;
	}

	// Invert the indirection array, so that i contains the rank of the ith centroid
	size_t CENT_RANK[K];
	for (size_t k=0; k<K; ++k) {
		for (size_t l=0; l<K; ++l) {
			if (CENT_ORDER[l] == k) CENT_RANK[k] = l;
		}
	}

	cerr << "Centroids: " << endl;
	for (size_t k=0; k<K; ++k) {
		cerr << "  [" << CENT_ORDER[k] << "] " << state.CENTROID[CENT_ORDER[k]] << endl;
	}


	size_t l = state.input->size / sizeof(float);
	float* output = (float*) calloc(l, sizeof(float));

	for (size_t i=0; i<state.LEN; ++i) {
		output[i] = (float) CENT_ORDER[state.CLUSTER[i]];
	}

	cerr << "Writing output..." << endl;
	sim_dump_clear(SIM_OUTPUT_FILE);
	sim_dump_buffer_f(SIM_OUTPUT_FILE, output, l*sizeof(float), state.input->ndims, state.input->dims);
	cerr << "Complete." << endl;
}
