#ifndef CONFIG_H
#define CONFIG_H

// Debug flags
#define DEBUG 0
#define START_ROW 0
#define END_ROW 5

// Verification flags
#define DATA_VERIFICATION 0
#define CHANNEL_RETRIEVAL_VERIFICATION 0
#define DOWNSAMPING_VERIFICATION 0
#define PROCESSING_RESULT_CHECK 1

// Constants
#define MAX_CHANNEL 2000
#define MATLAB_DIRECTORY "MATLAB Model/"
#define TARGET_FREQUENCY 32		 // Signal frequency in Hz
#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 1000	 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output_512.csv"
#define BUFFER_SIZE 1000 // Must be a multiple of 2
#define PRECISION 1e-9	 // Precision for floating point comparisons

#endif // CONFIG_H