#include <stdio.h>
#include <stdlib.h>

/**
 *  -M	--mgroup	specify multicast group
 *  -P	--port		specify receive port
 *  -p	--player	specify decoder
 *  -H	--help		display help
 *  implement by getopt_long
 */

int main(int argc, char** argv)
{
	/**
	 * Initialization:
	 * 	Level:(value from)	
	 * 		default value,
	 * 		configuration files,
	 * 		environment value,
	 *		command line parameter
	 * 		

	getopt_long();

	socket();

	pipe();

	fork();

	// child: call decoder
	// parent: receive packets, pipe to child.

	exit(0);
}
