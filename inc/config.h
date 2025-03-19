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
#define LOW_PASS_FILTER_VERIFICATION 0
#define HIGH_PASS_FILTER_VERIFICATION 0
#define ARTIFACT_REMOVAL_VERIFICATION 0
#define NEO_TRANSFORM_VERIFICATION 0
#define MOVING_AVERAGE_FILTER_VERIFICATION 0
#define EDGE_DETECTION_VERIFICATION 1

#define MIRROR_MATLAB_LOGIC 0

// Constants
#define MAX_CHANNEL 2000
#define MATLAB_DIRECTORY "MATLAB Model/"
#define TARGET_FREQUENCY 32		 // Signal frequency in Hz
#define DATA_DIRECTORY "data/" // Set to data directory from where the executable is being run
#define INITIAL_CAPACITY 1000	 // Start memory allocation for rows
#define DEFAULT_FILE "exp_16_output_512.csv"
#define BUFFER_SIZE 1001 // Must be a multiple of 2
#define PRECISION 1e-6	 // Precision for floating point comparisons

// PREPROCESSING CONSTANTS
// Low-pass filter
#define LPF_PADDING_SIZE 60
#define LPF_PADDED_BUFFER_SIZE ((BUFFER_SIZE + 1 /* + 1 to mirror the MATLAB logic, romove if not needed*/) + (2 * LPF_PADDING_SIZE))
// High-pass filter
#define HPF_FILTER_ORDER 50 // coefficient is declared in highpass_filter function
#define HPF_PADDING_SIZE 50
#define HPF_AD_SIGNAL_LEN (BUFFER_SIZE + HPF_FILTER_ORDER)
#define HPF_PADDED_SIGNAL_SIZE ((BUFFER_SIZE + 1 /* + 1 to mirror the MATLAB logic, romove if not needed*/) + (2 * HPF_PADDING_SIZE))
#define HPF_CONV_PADDED_SIGNAL_SIZE (HPF_PADDED_SIGNAL_SIZE + HPF_FILTER_ORDER)
// Artifact detection and removal
#define ARTIFACT_DETECT_WINDOW_WIDTH 101 // Window size for artifact detection
#define ARTIFACT_SIZE 60								 // Expected artifact size in samples
#define ARTIFACT_DETECT_THRESHOLD 500		 // Threshold for detecting artifacts
// NEO Transform
#define NEO_MAF_ED_SIGNAL_SIZE (BUFFER_SIZE + HPF_FILTER_ORDER - 1)
// Edge detection
#define ED_SCALE_VALUE 59

#endif // CONFIG_H