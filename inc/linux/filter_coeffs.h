#ifndef FILTER_COEFFS_H
#define FILTER_COEFFS_H

#define GOOD_SIG_LPF_COEFFS_LEN 65
#define BAD_SIG_LPF_COEFFS_LEN 59

extern const double good_sig_lpf_coeffs[];
extern const double bad_sig_lpf_coeffs[];
extern const double hpf_coeffs[];
extern const int hpf_coeffs_len;

#endif