/**
 * @harishan_assignment1
 * @author  Harishankar Vishwanathan <harishan@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
 
#include "../include/global.h"


int main(int argc, char *argv[])
{


	if (argc != 3)
	{
	    printf("Not enough arguments\n");
	    exit(1);
	}

	if (!strcmp(argv[1], "s"))
	{
		int p = atoi(argv[2]);
		if (p == 0)
		{
			printf("Please enter correct port number\n");
			exit(0);
		}

		server(argv[2]);
	}

	else if (!strcmp(argv[1], "c"))
	{
		int p = atoi(argv[2]);
		if (p == 0)
		{
			printf("Please enter correct port number\n");
			exit(0);
		}

		client(argv[2]);
	}

	else
	{
		printf("Invalid argument(s)\n");
		exit(0);
	}
		

exit (0);

}

