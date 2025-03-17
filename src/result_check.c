#include "result_check.h"

int check_processing_result(double *signal, size_t signal_length, int channel_num, char *file_name, char *ver_code, int shift)
{
	printf("\nChecking %s result of %dth buffer...\n", ver_code, shift + 1);
	// Load low pass results from Daryl's MATLAB code
	size_t mat_data_rows, mat_data_cols;
	char ver_filepath[200];
	char file_name_copy[200];
	strncpy(file_name_copy, file_name, sizeof(file_name_copy) - 1);
	file_name_copy[sizeof(file_name_copy) - 1] = '\0';
	sprintf(ver_filepath, "%sver_%s_%s_ch%d.csv", MATLAB_DIRECTORY, ver_code, strtok(file_name_copy, "."), channel_num);

	double **ver_file_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);
	if (ver_file_data == NULL)
	{
		printf("\nError: Failed to import verification data from %s\n", ver_filepath);
		return 1;
	}

	// Check results
	for (int k = 0; k < signal_length; k++)
	{
		// Check if the results match within a certain precision
		if ((signal[k] - ver_file_data[k][shift] > PRECISION) ||
				(ver_file_data[k][shift] - signal[k] > PRECISION))
		{
			printf("\nError: %s result mismatch at buffer %d, index %d.\n", ver_code, shift + 1, k);
			printf("%s_signal      [%4d] %20.15f\n", ver_code, k, signal[k]);
			printf("%s_ver_data[%2d][%4d] %20.15f\n", ver_code, shift, k, ver_file_data[k][shift]);
			double diff = fabs(signal[k] - ver_file_data[k][shift]);
			printf("Debug: Index %d | Difference = %.15f | PRECISION = %.15f\n", k, diff, PRECISION);
			// Free allocated memory
			for (size_t i = 0; i < mat_data_rows; i++)
			{
				free(ver_file_data[i]);
			}
			free(ver_file_data);
			return 1;
		}
		// else
		// {
		// 	printf("\n%s_signal      [%4d] %20.15f\n", ver_code, k, signal[k]);
		// 	printf("%s_ver_data[%2d][%4d] %20.15f\n", ver_code, shift, k, ver_file_data[k][shift]);
		// }
	}
	printf("Success: %dth buffer %s results match with verification data.\n", shift + 1, ver_code);
	// Free allocated memory
	for (size_t i = 0; i < mat_data_rows; i++)
	{
		free(ver_file_data[i]);
	}
	free(ver_file_data);
	return 0;
}