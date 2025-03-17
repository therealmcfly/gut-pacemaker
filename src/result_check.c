#include "result_check.h"

int check_processing_result(double *signal, size_t signal_length, int channel_num, char *file_name, char *ver_code, int shift)
{
	printf("\nChecking %s result of %dth buffer...\n", ver_code, shift);
	// Load low pass results from Daryl's MATLAB code
	size_t mat_data_rows, mat_data_cols;
	char ver_filepath[200];
	sprintf(ver_filepath, "%sver_%s_%s_ch%d.csv", MATLAB_DIRECTORY, ver_code, strtok(file_name, "."), channel_num);
	double **ver_file_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);

	// Check results
	for (int k = 0; k < signal_length; k++)
	{
		// Check if the results match within a certain precision
		if ((signal[k] - ver_file_data[k][shift] > PRECISION) ||
				(ver_file_data[k][shift] - signal[k] > PRECISION))
		{
			printf("\nError: %s result mismatch at buffer %d, index %d.\n", ver_code, shift, k);
			printf("%s_signal[%d] = %.15f\n", ver_code, k, signal[k]);
			printf("%s_ver_data[%d][%d] = %.15f\n", ver_code, shift, k, ver_file_data[k][shift]);
			// Free allocated memory
			for (size_t i = 0; i < mat_data_rows; i++)
			{
				free(ver_file_data[i]);
			}
			free(ver_file_data);
			return 1;
		}
	}
	printf("Success: %dth buffer %s results match with verification data.\n", shift, ver_code);
	// Free allocated memory
	for (size_t i = 0; i < mat_data_rows; i++)
	{
		free(ver_file_data[i]);
	}
	free(ver_file_data);
	return 0;
}