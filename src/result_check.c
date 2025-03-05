#include "result_check.h"
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

int verify_signals(float *signal, size_t num_of_data, float **verify_data, size_t *verify_num_rows, size_t *verify_num_cols)
{
	// compare rows and columns of data and verify_data
	if (*verify_num_rows != num_of_data)
	{
		printf("Error: Number of rows do not match.\n\n");
		printf("verify_num_rows: %zu\n", *verify_num_rows);
		printf("num_of_data: %zu\n", num_of_data);
		return 1;
	}

	printf("Row match : %zu row(s)\n", *verify_num_rows);

	if (*verify_num_cols != 1)
	{
		printf("Error: Number of columns do not match.\n\n");
		printf("verify_num_cols: %zu\n", *verify_num_cols);
		printf("data_num_cols: %d\n", 1);
		return 1;
	}

	printf("Column match : %zu column(s)\n", *verify_num_cols);

	// Print Table Header
	printf("\n---------------------------------------------------\n");
	printf("| Index |      signal      |    verify_data   |\n");
	printf("---------------------------------------------------\n");

	// compare each row of data and verify_data
	int count = 0;
	for (size_t row = 0; row < num_of_data; row++)
	{
		// Print data row with proper alignment
		if (row >= START_ROW && row <= END_ROW)
		{
			printf("| %5zu | %16.6f | %16.6f |\n", row, signal[row], verify_data[row][0]);
		}
		if (row == END_ROW + 1)
		{
			printf("---------------------------------------------------\n");
			printf("... and %zu more rows\n", num_of_data - END_ROW - 1);
			break;
		}

		// Check for mismatch
		if (signal[row] != verify_data[row][0])
		{
			printf("---------------------------------------------------\n");
			printf("Error: Data mismatch at row %zu.\n\n", row);
			printf("signal in row %zu: %f\n", row, signal[row]);
			printf("verify_data in row %zu: %f\n", row, verify_data[row][0]);
			return 1;
		}

		count++;
	}

	// Print closing line
	printf("Data match : %lld data(s) checked\n", num_of_data);
	return 0;
}