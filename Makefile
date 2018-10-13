APP=kmeans

all: kmeans inputs/heightmap.dump inputs/heightmap_small.dump

kmeans: kmeans.cpp io.cpp size.h sim_interface.h sim_interface.c
	g++ -O3 kmeans.cpp io.cpp sim_interface.c -o $@

%.dump: %.mat
	# Auto-generate a float-format dump file that is C-friendly from
	# a MAT matrix.
	../../analyzer/fiddle --input $< --output $@ load:0 push save

clean:
	rm kmeans

run: $(APP)
	#bash -c "time ../mpki ./$(APP) 3000 output.data 2 0 input/100_100_130_ldc.of"
	bash -c "time ../mpki ./$(APP) input.data output.data"

dump: $(APP)
	SIM_DUMP_FILE=../../analyzer/dumps/$(APP).dump ./$(APP) input.data output.data
