#include "version.h"

#include <client/utils/log.h>

// print full version information 
void pixzo_version_print_full (void) {

	client_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Pixzo Version: %s", PIXZO_VERSION_NAME
	);

	client_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Release Date & time: %s - %s", PIXZO_VERSION_DATE, PIXZO_VERSION_TIME
	);

	client_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Author: %s", PIXZO_VERSION_AUTHOR
	);

}

// print the version id
void pixzo_version_print_version_id (void) {

	client_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Pixzo Version ID: %s", PIXZO_VERSION
	);

}

// print the version name
void pixzo_version_print_version_name (void) {

	client_log_both (
		LOG_TYPE_NONE, LOG_TYPE_NONE,
		"Pixzo Version: %s", PIXZO_VERSION_NAME
	);

}
