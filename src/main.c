#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#include <client/client.h>
#include <client/version.h>

#include <client/utils/log.h>

#include "config.h"
#include "global.h"
#include "pixzo.h"
#include "version.h"

int main (int argc, char **argv) {

	(void) signal (SIGINT, pixzo_quit);
	(void) signal (SIGTERM, pixzo_quit);

	client_init ();

	client_log_line_break ();
	cerver_client_version_print_full ();
	client_log_line_break ();

	pixzo_version_print_full ();
	client_log_line_break ();

	if (argc > 1) {
		if (!global_init (argc, argv)) {
			pixzo_start ();
		}
	}

	else {
		config_help ();
	}

	global_end ();

	client_log_line_break ();
	client_log_success ("Done!");
	client_log_line_break ();

	client_end ();

	return 0;

}
