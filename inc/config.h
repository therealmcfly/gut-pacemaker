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
#define TARGET_FREQUENCY 32		 // Signal frequency in Hz
#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 500	 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output_512.csv"
// #define DEFAULT_FILE "pig41exp2_30.csv"
#define BUFFER_SIZE 1001 // Must be a multiple of 2

#define BUFFER_SIZE_HALF (BUFFER_SIZE / 2) // Half of the buffer size

#define PRECISION 1e-6 // Precision for floating point comparisons
#define ED_PRECISION 6 // Precision for edge detection

#define ACTIVATIONS_ARRAY_SIZE 1000
#define BUFFER_ACTIVATION_ARRAY_SIZE 20
#define BUFFER_OFFSET BUFFER_SIZE_HALF // Overlap count for ring buffer
// #define BUFFER_OFFSET 900 // Overlap count for ring buffer

// PREPROCESSING CONSTANTS
// Low-pass filter
#define LPF_PADDING_SIZE 60
#define LPF_PADDED_BUFFER_SIZE ((BUFFER_SIZE) + (2 * LPF_PADDING_SIZE))
// High-pass filter
#define HPF_FILTER_ORDER 50 // coefficient is declared in highpass_filter function
#define HPF_PADDING_SIZE 50
#define HPF_AD_SIGNAL_LEN (BUFFER_SIZE + HPF_FILTER_ORDER)
#define HPF_PADDED_SIGNAL_SIZE ((BUFFER_SIZE) + (2 * HPF_PADDING_SIZE))
#define HPF_CONV_PADDED_SIGNAL_SIZE (HPF_PADDED_SIGNAL_SIZE + HPF_FILTER_ORDER)
// Artifact detection and removal
#define ARTIFACT_DETECT_WINDOW_WIDTH 101 // Window size for artifact detection
#define ARTIFACT_SIZE 60								 // Expected artifact size in samples
#define ARTIFACT_DETECT_THRESHOLD 500		 // Threshold for detecting artifacts
// NEO Transform
#define NEO_MAF_ED_SIGNAL_SIZE (BUFFER_SIZE + HPF_FILTER_ORDER - 1)
// Edge detection
#define ED_SCALAR_VALUE 59
// Activation detection
#define CLOSE_PROX_ACT_REMOVAL_THRESHOLD 500

// Thread logs
#define PT_TITLE "[Process Thread] "
#define RT_TITLE "[Receive Thread] "

#endif // CONFIG_H