#include <stdio.h>
#include "mode_select.h"

RunMode select_mode(void)
{
	int choice;
	while (1)
	{
		printf("\nWelcome to Gut Pacemaker!\n");
		printf("\nPlease select a mode:\n");
		printf("\n1. Static Dataset Mode\n");
		printf("2. Real-time Dataset Mode\n");
		printf("3. Gut Model Mode\n");
		printf("\nEnter choice (1-3): ");
		if (scanf("%d", &choice) != 1)
		{
			while (getchar() != '\n')
				; // clear stdin buffer
			printf("Invalid input. Please enter a number.\n");
			continue;
		}
		if (choice >= 1 && choice <= 3)
			return (RunMode)choice;
		else
			printf("Invalid choice. Try again.\n");
	}
}