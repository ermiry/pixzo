#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <client/client.h>

#include <client/utils/log.h>

#include "config.h"
#include "global.h"
#include "store.h"

Global *global = NULL;

const char *global_type_to_string (const GlobalType type) {

	switch (type) {
		#define XX(num, name, string) case PIXZO_GLOBAL_TYPE_##name: return #string;
		GLOBAL_TYPE_MAP(XX)
		#undef XX
	}

	return global_type_to_string (PIXZO_GLOBAL_TYPE_NONE);

}

GlobalType global_type_from_string (const char *type) {

	GlobalType global_type = PIXZO_GLOBAL_TYPE_NONE;

	if (!strcasecmp (type, "single")) {
		global_type = PIXZO_GLOBAL_TYPE_SINGLE;
	}

	else if (!strcasecmp (type, "videos")) {
		global_type = PIXZO_GLOBAL_TYPE_VIDEOS;
	}

	else if (!strcasecmp (type, "load")) {
		global_type = PIXZO_GLOBAL_TYPE_LOAD;
	}

	else if (!strcasecmp (type, "register")) {
		global_type = PIXZO_GLOBAL_TYPE_REGISTER;
	}

	else if (!strcasecmp (type, "record")) {
		global_type = PIXZO_GLOBAL_TYPE_RECORD;
	}

	else if (!strcasecmp (type, "test")) {
		global_type = PIXZO_GLOBAL_TYPE_TEST;
	}

	else {
		client_log_error ("Unknown type: %s", type);
	}

	return global_type;

}

const char *global_status_to_string (const GlobalStatus status) {

	switch (status) {
		#define XX(num, name, string, description) case PIXZO_GLOBAL_STATUS_##name: return #string;
		GLOBAL_STATUS_MAP(XX)
		#undef XX
	}

	return global_status_to_string (PIXZO_GLOBAL_STATUS_NONE);

}

const char *global_status_description (const GlobalStatus status) {

	switch (status) {
		#define XX(num, name, string, description) case PIXZO_GLOBAL_STATUS_##name: return #string;
		GLOBAL_STATUS_MAP(XX)
		#undef XX
	}

	return global_status_description (PIXZO_GLOBAL_STATUS_NONE);

}

static Global *global_new (void) {

	Global *g = (Global *) malloc (sizeof (Global));
	if (g) {
		g->status = PIXZO_GLOBAL_STATUS_NONE;

		config_init (&g->config);

		g->client = NULL;
		g->connection = NULL;

		g->connected = false;

		g->store = NULL;

		g->mutex = NULL;
	}

	return g;

}

static void global_delete (void *global_ptr) {

	if (global_ptr) {
		Global *g = (Global *) global_ptr;

		(void) pthread_mutex_lock (g->mutex);

		(void) client_teardown (g->client);

		store_delete (g->store);

		(void) pthread_mutex_unlock (g->mutex);
		(void) pthread_mutex_destroy (g->mutex);
		free (g->mutex);

		free (global_ptr);
	}

}

static Global *global_create (void) {

	Global *g = global_new ();
	if (g) {
		g->client = client_create ();
		if (g->client) {
			g->store = store_create ();
			if (g->store) {
				g->mutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
				(void) pthread_mutex_init (g->mutex, NULL);
			}

			else {
				client_log_error ("Failed to create global store!");
				global_delete (g);
				g = NULL;
			}
		}

		else {
			client_log_error ("Failed to create global client!");
			global_delete (g);
			g = NULL;
		}
	}

	return g;

}

unsigned int global_init (int argc, char **argv) {

	unsigned int retval = 1;

	global = global_create ();
	if (global) {
		config_get_values (&global->config, argc, argv);

		if (global->config.type) {
			global->type = global_type_from_string (global->config.type);

			switch (global->type) {
				case PIXZO_GLOBAL_TYPE_NONE: break;

				case PIXZO_GLOBAL_TYPE_SINGLE:
					retval = config_validate_single (&global->config);
					break;

				case PIXZO_GLOBAL_TYPE_VIDEOS:
					retval = config_validate_videos (&global->config);
					break;

				case PIXZO_GLOBAL_TYPE_LOAD:
					retval = config_validate_load (&global->config);
					break;

				case PIXZO_GLOBAL_TYPE_REGISTER:
					retval = config_validate_register (&global->config);
					break;

				case PIXZO_GLOBAL_TYPE_TEST:
					retval = 0;
					break;

				default: break;
			}

			config_log (&global->config);
		}

		else {
			client_log_error ("Failed to get type!");
		}
	}

	return retval;

}

void global_end (void) {

	global_delete (global);

}

void global_set_status (const GlobalStatus status) {

	(void) pthread_mutex_lock (global->mutex);
	global->status = status;
	(void) pthread_mutex_unlock (global->mutex);

}

GlobalStatus global_get_status (void) {

	GlobalStatus retval = PIXZO_GLOBAL_STATUS_NONE;

	(void) pthread_mutex_lock (global->mutex);
	retval = global->status;
	(void) pthread_mutex_unlock (global->mutex);

	return retval;

}