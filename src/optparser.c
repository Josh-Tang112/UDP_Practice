/* This is an example program that parses the options provided on the
 * command line that are needed for assignment 0. You may copy all or
 * parts of this code in the assignment */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>

#include "../includes/optparser.h"

error_t server_parser(int key, char *arg, struct argp_state *state) {
	struct server_arguments *args = state->input;
	error_t ret = 0;
	switch(key) {
	case 'p':
		/* Validate that port is correct and a number, etc!! */
		args->port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		break;
	case 'd':
		args->droprate = atoi(arg);
		break;
	case 500:
		args->condense = 2;
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

struct server_arguments server_parseopt(int argc, char *argv[]) {
	struct server_arguments args;

	/* bzero ensures that "default" parameters are all zeroed out */
	bzero(&args, sizeof(args));



	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port to be used for the server",0},
		{ "droprate", 'd', "droprate", 0,"The probability a datagram is purposefully dropped",0},
		{"condensed",500,0,OPTION_ARG_OPTIONAL,"Condense the datagram",0},
		{0}
	};
	struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };
	if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
		printf("Got an error condition when parsing\n");
	}

	/* What happens if you don't pass in parameters? Check args values
	 * for sanity and required parameters being filled in */

	/* If they don't pass in all required settings, you should detect
	 * this and return a non-zero value from main */
	

	return args;
}

error_t client_parser(int key, char *arg, struct argp_state *state) {
	struct client_arguments *args = state->input;
	error_t ret = 0;
	switch(key) {
	case 'a':
		/* validate that address parameter makes sense */
		strncpy(args->ip_address, arg, 16);
		if (0 /* ip address is goofytown */) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		/* Validate that port is correct and a number, etc!! */
		args->port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		break;
	case 'n':
		/* validate argument makes sense */
		args->num = atoi(arg);
		break;
	case 't':
		/* validate file */
		args->timeout = atoi(arg);
		break;
	case 500:
		args->condense = 2;
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}


struct client_arguments client_parseopt(int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0},
		{ "num", 'n', "num", 0, "The number of time requests to send to the server", 0},
		{ "timeout", 't', "timeout", 0, "Time in seconds till client shut down", 0},
		{"condensed",500,0,OPTION_ARG_OPTIONAL,"Condense the datagram",0},
		{0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };

	struct client_arguments args;
	bzero(&args, sizeof(args));

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
		printf("Got error in parse\n");
	}

	/* If they don't pass in all required settings, you should detect
	 * this and return a non-zero value from main */
	

	return args;
}

