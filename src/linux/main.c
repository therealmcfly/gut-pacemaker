#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include "global.h"
#include "mode_select.h"

int main(int argc, char *argv[])
{
	RunMode mode = select_mode();

	switch (mode)
	{
	case MODE_STATIC_DATASET:
		printf("\nRunning in Static Dataset Mode...\n");
		if (static_dataset_mode(argc, argv) != 0)
		{
			printf("\nError occured while running static dataset mode.\n");
		}
		printf("\nExiting Static Dataset Mode...\n");
		break;
	case MODE_REALTIME_DATASET:

		printf("\nRunning in Real-time Dataset Mode...\n");

		if (realtime_dataset_mode(argc, argv) != 0)
		{
			printf("\nError occured while running Real-time Dataset Mode.\n");
		}

		printf("\nExiting Real-time Dataset Mode...\n");

		break;
	case MODE_GUT_MODEL:
		printf("\nRunning in Gut Model Mode(TCP)...\n");

		if (gut_model_mode(argc, argv) != 0)
		{
			printf("\nError occured while running Gut Model Mode(TCP).\n");
		}

		printf("\nExiting Gut Model Mode(TCP)...\n");

		break;
	case MODE_GUT_MODEL_UART:
		printf("\nRunning in Gut Model Mode(UART)...\n");

		if (gm_mode_uart(argc, argv) != 0)
		{
			printf("\nError occured while running Gut Model Mode(UART).\n");
		}

		printf("\nExiting Gut Model Mode(UART)...\n");

		break;
	default:
		printf("\nUnknown mode.\n");
		return 1;
	}

	printf("\nExiting program...\n\n");
	return 0;
}
