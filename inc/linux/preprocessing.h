#ifndef PREPROCESSING_H
#define PREPROCESSING_H

int lowpass_filter(double *lpf_signal, int signal_length, int is_bad_signal, void (*callback_unlock_mutex)(void));
int highpass_filter(double *in_signal, int in_signal_len, double *out_hpf_signal, int *out_hpf_signal_len);
int detect_remove_artifacts(double *in_signal, int signal_length);

// // Sub-functions for filtering and artifact detection
// static int lowpass_fir_filter(const double *coeffs, int coeff_len, const double *in_signal, double *out_signal, int signal_length, int is_bad_signal);
// static int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, int in_signal_len, double *out_signal, int out_signal_len);
// static int detect_artifact(const double *window, int window_size, double threshold);
// static void remove_artifact(double *window, int window_len, int x1, int x2);

// // util functions
// static int apply_padding(double *in_signal, int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size);
// static int apply_padding_from_rb(int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size);
// static void compute_single_spline(double x1, double y1, double dy1,
// 																	double x2, double y2, double dy2,
// 																	double *a, double *b, double *c, double *d);
// static double evaluate_spline(double a, double b, double c, double d, double x1, double x2, double x_query);

#endif