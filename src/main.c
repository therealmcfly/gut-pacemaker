#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "data_init.h"

int main(int argc, char *argv[])
{
	size_t sample_data_length;

	// INITIALIZE SAMPLE DATA
	// Sample data loading, channel selection, and downsampling is all handled within the get_sample_data function
	// The function will return a pointer to sample data on success, NULL on error
	float *sample_data = get_sample_data(argc, argv, &sample_data_length);

	if (sample_data == NULL)
	{
		printf("Error occured while initializing sample data.\n");
		printf("Exiting program...\n");
		return 1;
	}
#ifdef DEBUG
	printf("Sample data[%d]: %f\n", 0, sample_data[0]);
	printf("Sample data[%d]: %f\n", 1, sample_data[1]);
	printf("Sample data[%d]: %f\n", 2, sample_data[2]);
	printf("Data length: %lld\n", sample_data_length);
#endif

	// LOW PASS FILTERING

	// HIGH PASS FILTERING

	// ARTIFACT REMOVAL
	return 0;
}