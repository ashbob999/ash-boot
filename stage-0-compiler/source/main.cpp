
#include <fstream>
#include <iostream>
#include <string>
#include "cli.h"

int main(int argc, char** argv)
{
	cli::CLI cli(argc, argv);

	if (cli.run())
	{
		return 0;
	}
	else
	{
		return 1;
	}

	return 0;
}
