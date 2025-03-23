#include "result_check.h"

int check_processing_result(double *signal, size_t signal_length, int channel_num, char *file_name, char *ver_code, int shift, double ver_percision)
{
	printf("\nChecking %s result of %dth buffer...\n", ver_code, shift + 1);
	// Load low pass results from Daryl's MATLAB code
	size_t mat_data_rows, mat_data_cols;
	char ver_filepath[100];
	char file_name_copy[50];
	// check if file name char until it reaches \0
	for (int i = 0; i < 200; i++)
	{
		if (file_name[i] == '\0')
		{
			file_name_copy[i] = '\0';
			break;
		}
		file_name_copy[i] = file_name[i];
	}

	// strncpy(file_name_copy, file_name, sizeof(file_name_copy) - 1);
	sprintf(ver_filepath, "%sver_%s_%s_ch%d.csv", MATLAB_DIRECTORY, ver_code, strtok(file_name_copy, "."), channel_num);

	double **ver_file_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);
	if (ver_file_data == NULL)
	{
		printf("\nError: Failed to import verification data from %s\n", ver_filepath);
		return 1;
	}
	// for (int i = 0; i < signal_length; i++)
	// {
	// 	printf("\n%s_signal      [%4d] %20.15f\n", ver_code, i, signal[i]);
	// 	printf("%s_ver_data[%2d][%4d] %20.15f\n\n", ver_code, shift, i, ver_file_data[i][shift]);
	// }
	// Check results
	for (int k = 0; k < signal_length; k++)
	{

		// Check if the results match within a certain precision
		if ((signal[k] - ver_file_data[k][shift] > ver_percision) ||
				(ver_file_data[k][shift] - signal[k] > ver_percision))
		{
			printf("\nError: %s result mismatch at buffer %d, index %d.\n", ver_code, shift + 1, k);
			printf("%s_signal      [%4d] %20.15f\n", ver_code, k, signal[k]);
			printf("%s_ver_data[%2d][%4d] %20.15f\n", ver_code, shift, k, ver_file_data[k][shift]);
			double diff = fabs(signal[k] - ver_file_data[k][shift]);
			printf("Debug: Index %d | Difference = %.15f | PRECISION = %.15f\n", k, diff, ver_percision);
			// Free allocated memory
			for (size_t i = 0; i < mat_data_rows; i++)
			{
				free(ver_file_data[i]);
			}
			free(ver_file_data);
			return 1;
		}
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

int check_activations(int *activation_indices, int num_activations, int channel_num, char *file_name)
{
	printf("\nChecking activations detection results...\n");
	// Load low pass results from Daryl's MATLAB code
	size_t mat_data_rows, mat_data_cols;
	char ver_filepath[100];
	char file_name_copy[50];
	// check if file name char until it reaches \0
	for (int i = 0; i < 200; i++)
	{
		if (file_name[i] == '\0')
		{
			file_name_copy[i] = '\0';
			break;
		}
		file_name_copy[i] = file_name[i];
	}

	// strncpy(file_name_copy, file_name, sizeof(file_name_copy) - 1);
	sprintf(ver_filepath, "%sver_actdpre_%s_ch%d.csv", MATLAB_DIRECTORY, strtok(file_name_copy, "."), channel_num);

	double **ver_file_data = import_file(ver_filepath, &mat_data_rows, &mat_data_cols);
	if (ver_file_data == NULL)
	{
		printf("\nError: Failed to import verification data from %s\n", ver_filepath);
		return 1;
	}
	// for (int i = 0; i < signal_length; i++)
	// {
	// 	printf("\n%s_signal      [%4d] %20.15f\n", ver_code, i, signal[i]);
	// 	printf("%s_ver_data[%2d][%4d] %20.15f\n\n", ver_code, shift, i, ver_file_data[i][shift]);
	// }
	// Check results
	for (int k = 0; k < num_activations; k++)
	{

		// Check if the results match within a certain precision
		if ((activation_indices[k] + 1 != (int)ver_file_data[k][0]))
		{
			printf("\nError: activation detection result mismatch at index %d.\n", k);
			printf("activation idx      [%2d] %d\n", k, activation_indices[k]);
			printf("actd_ver_data[%2d] %d\n", k, (int)ver_file_data[k][0]);
			// Free allocated memory
			for (size_t i = 0; i < mat_data_rows; i++)
			{
				free(ver_file_data[i]);
			}
			free(ver_file_data);
			return 1;
		}
	}

	printf("Success: activation detection results match with verification data.\n");
	// Free allocated memory
	for (size_t i = 0; i < mat_data_rows; i++)
	{
		free(ver_file_data[i]);
	}
	free(ver_file_data);
	return 0;
}