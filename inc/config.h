#ifndef CONFIG_H
#define CONFIG_H

// Debug flags
#define DEBUG 0
#define START_ROW 0
#define END_ROW 5

// Verification flags
// #define CHANNEL_RETRIEVAL_VERIFICATION
// #define DOWNSAMPLING_VERIFICATION
// #define LOW_PASS_FILTER_VERIFICATION
// #define HIGH_PASS_FILTER_VERIFICATION
// #define ARTIFACT_REMOVAL_VERIFICATION
// #define NEO_TRANSFORM_VERIFICATION
// #define MOVING_AVERAGE_FILTER_VERIFICATION
// #define EDGE_DETECTION_VERIFICATION
#define PRE_ACTIVATION_DETECTION_VERIFICATION
#define ACTIVATION_DETECTION_VERIFICATION

#define MIRROR_MATLAB_LOGIC 0

// Constants
#define MAX_CHANNEL 2000
#define MATLAB_DIRECTORY "MATLAB Model/"
#define TARGET_SAMPLING_FREQUENCY 32 // Signal frequency in Hz
#define DATA_DIRECTORY "data/"			 // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 500				 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output_512.csv"
// #define DEFAULT_FILE "pig41exp2_30.csv"

// CHANNEL CONSTANTS
#define NUM_CHANNELS 1 // Number of channels to process

// BUFFER CONSTANTS
#define SIGNAL_PROCESSING_BUFFER_SIZE 1001											// Buffer size for signal processing
#define SP_BUFFER_SIZE_HALF (SIGNAL_PROCESSING_BUFFER_SIZE / 2) // Half of the buffer size
#define ACTIVATION_DETECTION_BUFFER_SIZE 50											// Buffer size for activation detection
#define AD_BUFFER_OFFSET 1																			// Overlap count for ring buffer
// #define AD_BUFFER_OFFSET SP_BUFFER_SIZE_HALF // Overlap count for ring buffer

// Precision constants
#define PRECISION 1e-6 // Precision for floating point comparisons
#define ED_PRECISION 6 // Precision for edge detection

// Signal processing constants
// Low-pass filter
#define LPF_PADDING_SIZE 60
#define LPF_PADDED_BUFFER_SIZE ((SIGNAL_PROCESSING_BUFFER_SIZE) + (2 * LPF_PADDING_SIZE))
// High-pass filter
#define HPF_FILTER_ORDER 50 // coefficient is declared in highpass_filter function
#define HPF_PADDING_SIZE 50
#define HPF_AD_SIGNAL_LEN (SIGNAL_PROCESSING_BUFFER_SIZE + HPF_FILTER_ORDER)
#define HPF_PADDED_SIGNAL_SIZE ((SIGNAL_PROCESSING_BUFFER_SIZE) + (2 * HPF_PADDING_SIZE))
#define HPF_CONV_PADDED_SIGNAL_SIZE (HPF_PADDED_SIGNAL_SIZE + HPF_FILTER_ORDER)
// Artifact detection and removal
#define ARTIFACT_DETECT_WINDOW_WIDTH 101 // Window size for artifact detection
#define ARTIFACT_SIZE 60								 // Expected artifact size in samples
#define ARTIFACT_DETECT_THRESHOLD 500		 // Threshold for detecting artifacts
// NEO Transform
#define NEO_MAF_ED_SIGNAL_SIZE (SIGNAL_PROCESSING_BUFFER_SIZE + HPF_FILTER_ORDER - 1)
// Edge detection
#define ED_SCALAR_VALUE 59
// Activation detection
#define AD_SCALAR_VALUE 5.9
#define CLOSE_PROX_ACT_REMOVAL_THRESHOLD 500 // set 400 for good dataset, 500 for bad dataset
#define ACTIVATIONS_ARRAY_SIZE 1000
#define BUFFER_ACTIVATION_ARRAY_SIZE 20

// pacemaker thread
#define SAMPLING_INTERVAL_MS 10 // Sampling interval in seconds
#define LEARN_TIME_MS 60000			// Learning time in milliseconds
#define GRI_THRESHOLD_MS 5000		// Default GRI value in milliseconds
#define LRI_THRESHOLD_MS 20500	// Default LRI value in milliseconds

// Thread Constants
#define PT_TITLE "[Process Thread] "
#define RT_TITLE "[Receive Thread] "

#endif // CONFIG_H