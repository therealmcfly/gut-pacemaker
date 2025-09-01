#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for close(), usleep()
#include "global.h"
#include "mode_select.h"

int logging_enabled = 1; // Flag to enable/disable logging

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
	case MODE_SIL:
		printf("\nRunning in Software-in-the-loop(TCP) Mode...\n");

		if (sil_mode_tcp(argc, argv) != 0)
		{
			printf("\nError occured while running Software-in-the-loop(TCP) Mode.\n");
		}

		printf("\nExiting Software-in-the-loop(TCP) Mode...\n");

		break;
	case MODE_HIL:
		printf("\nRunning Hardware-in-the-loop(UART) Mode...\n");

		if (hil_mode_uart(argc, argv) != 0)
		{
			printf("\nError occured while running Hardware-in-the-loop(UART) Mode.\n");
		}

		printf("\nExiting Hardware-in-the-loop(UART) Mode...\n");

		break;
	case MODE_HIL_NO_LOG:
		printf("\nRunning Hardware-in-the-loop(UART) Mode(no logging)...\n");
		logging_enabled = 0; // Disable logging for this mode
		if (hil_mode_uart(argc, argv) != 0)
		{
			printf("\nError occured while running Hardware-in-the-loop(UART) Mode(no logging).\n");
		}

		printf("\nExiting Hardware-in-the-loop(UART) Mode(no logging)...\n");

		break;
	default:
		printf("\nUnknown mode.\n");
		return 1;
	}

	printf("\nExiting program...\n\n");
	return 0;
}
