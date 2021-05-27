#include <stdlib.h>
#include <string.h>

#include <client/utils/log.h>

#include "config.h"
#include "pixzo.h"

static const char *true_str = "true";
static const char *false_str = "false";
static const char *null = "null";

void config_init (Config *config) {

	config->type = NULL;

	config->store_id = NULL;
	config->store_name = NULL;

	config->camera_name = NULL;
	config->videos_path = NULL;

	config->fps = CONFIG_DEFAULT_FPS;

	config->rotation = CONFIG_DEFAULT_ROTATION;
	config->scale_factor = CONFIG_DEFAULT_SCALE_FACTOR;

	config->movement_thresh = CONFIG_DEFAULT_MOVEMENT_THRESH;
	config->max_no_movement_frames = CONFIG_DEFAULT_NO_MOVEMENT_FRAMES;

	config->max_actions_memory_size = CONFIG_DEFAULT_MAX_ACTIONS_MEM_SIZE;

	config->videos_n_loops = CONFIG_DEFAULT_VIDEOS_N_LOOPS;

	config->enable_wait_key = CONFIG_DEFAULT_ENABLE_WAIT_KEY;
	config->wait_key_delay = CONFIG_DEFAULT_WAIT_KEY_DELAY;

	config->record = CONFIG_DEFAULT_RECORD;
	config->output_path = NULL;

	config->cams_settings_filename = CONFIG_DEFAULT_CAMS_SETTINGS;

	config->connect = CONFIG_DEFAULT_CONNECT;

}

static unsigned int config_validate_store_id (
	const char *store_id
) {

	unsigned int errors = 0;

	if (!store_id) {
		client_log_error ("Failed to get store id!");
		errors = 1;
	}

	return errors;
	
}

static unsigned int config_validate_store_name (
	const char *store_name
) {

	unsigned int errors = 0;

	if (!store_name) {
		client_log_error ("Failed to get store name!");
		errors = 1;
	}

	return errors;
	
}

static unsigned int config_validate_camera_name (
	const char *camera_name
) {

	unsigned int errors = 0;

	if (!camera_name) {
		client_log_error ("Failed to get camera name!");
		errors = 1;
	}

	return errors;
	
}

static unsigned int config_validate_videos_path (
	const char *videos_path
) {

	unsigned int errors = 0;

	if (!videos_path) {
		client_log_error ("Failed to get videos path!");
		errors = 1;
	}

	return errors;
	
}

unsigned int config_validate_single (
	const Config *config
) {

	unsigned int errors = 0;

	errors |= config_validate_store_id (config->store_id);

	errors |= config_validate_store_name (config->store_name);

	errors |= config_validate_camera_name (config->camera_name);

	return errors;

}

unsigned int config_validate_videos (
	const Config *config
) {

	unsigned int errors = 0;

	errors |= config_validate_store_id (config->store_id);

	errors |= config_validate_store_name (config->store_name);

	errors |= config_validate_videos_path (config->videos_path);

	return errors;

}

unsigned int config_validate_load (
	const Config *config
) {

	unsigned int errors = 0;

	errors |= config_validate_store_id (config->store_id);

	errors |= config_validate_store_name (config->store_name);

	return errors;

}

unsigned int config_validate_register (
	const Config *config
) {

	unsigned int errors = 0;

	errors |= config_validate_store_id (config->store_id);

	errors |= config_validate_store_name (config->store_name);

	return errors;

}

void config_log (const Config *config) {

	client_log_debug ("Type: %s", config->type ? config->type : null);

	client_log_debug ("Store ID: %s", config->store_id ? config->store_id : null);
	client_log_debug ("Store Name: %s", config->store_name ? config->store_name : null);

	client_log_debug ("Camera name: %s", config->camera_name ? config->camera_name : null);
	client_log_debug ("Videos path: %s", config->videos_path ? config->videos_path : null);

	client_log_debug ("FPS: %u", config->fps);

	client_log_debug ("Rotation: %u", config->rotation);
	client_log_debug ("Scale factor: %u", config->scale_factor);

	client_log_debug ("Movement thresh: %u", config->movement_thresh);
	client_log_debug ("Max no mov frames: %u", config->max_no_movement_frames);

	client_log_debug ("Max actions mem size: %u", config->max_actions_memory_size);
	client_log_debug ("Wait key delay: %u", config->wait_key_delay);

	client_log_debug ("Record: %s", config->record ? true_str : false_str);
	client_log_debug ("Output path: %s", config->output_path ? config->output_path : null);

	client_log_debug ("Cameras config file: %s", config->cams_settings_filename);

	client_log_debug ("Connect: %s", config->connect ? true_str : false_str);

}

void config_help (void) {

	(void) printf ("\n");
	(void) printf ("Usage: ./bin/pixzo-client -t [type] [args]\n");

	(void) printf ("-h                       Print this help\n");

	(void) printf ("-t [type]                Type to tun\n");
	(void) printf ("   normal                Run store with streams from media devices\n");
	(void) printf ("   videos                Run streams from videos\n");

	(void) printf ("-m [ip]                  Pixzo main's ip\n");
	(void) printf ("-i [id]                  The store's id\n");
	(void) printf ("-n [id]                  The store's name\n");

	(void) printf ("-c [name]                The name of the camera\n");
	(void) printf ("-v [path]                Select the videos path to be used as input\n");

	(void) printf ("--fps [val]              Video writer fps\n");
	(void) printf ("-r [val]                 The frames rotation\n");
	(void) printf ("-s [val]                 The scale factor to use\n");
	(void) printf ("-s [val]                 The scale factor to use\n");
	(void) printf ("--mov_thresh [val]       The movmenet threshold to count as movement\n");
	(void) printf ("--max_no_mov [n frames]  The max number of frames to allow without movement\n");

	(void) printf ("--actions_mem_size [n]   How many actions to keep in memory\n");

	(void) printf ("--videos_n_loops [n]     How many times to repeat the videos\n");

	(void) printf ("--enable_wait_key        Enables delay using waitKey ()\n");
	(void) printf ("--wait_key_delay [ms]    The delay for each stream's thread iteration\n");

	(void) printf ("--record                 Option to record videos from streams\n");
	(void) printf ("-o [output]              Specifies the output path for videos & images\n");

	(void) printf ("--cams [filename]        Specifies a custom cameras settings filename\n");

	(void) printf ("--connect [value]        Enables connection to the main cerver (defaults to TRUE)\n");
	(void) printf ("\n");

}

static bool config_get_bool_from_string (const char *value) {

	if (!strcasecmp (true_str, value)) return true;
	return false;

}

void config_get_values (
	Config *config,
	int argc, char **argv
) {

	int j = 0;
	const char *curr_arg = NULL;

	for (int i = 1; i < argc; i++) {
		curr_arg = argv[i];

		if (!strcmp (curr_arg, "-h")) config_help ();

		// get the type to run
		else if (!strcmp (curr_arg, "-t")) {
			j = i + 1;
			if (j <= argc) {
				config->type = argv[j];
				i++;
			}
		}

		// get the store id
		else if (!strcmp (curr_arg, "-i")) {
			j = i + 1;
			if (j <= argc) {
				config->store_id = argv[j];
				i++;
			}
		}

		// get the store name
		else if (!strcmp (curr_arg, "-n")) {
			j = i + 1;
			if (j <= argc) {
				config->store_name = argv[j];
				i++;
			}
		}

		// camera
		else if (!strcmp (curr_arg, "-c")) {
			j = i + 1;
			if (j <= argc) {
				config->camera_name = argv[j];
				i++;
			}
		}

		// get the videos dir
		else if (!strcmp (curr_arg, "-v")) {
			j = i + 1;
			if (j <= argc) {
				config->videos_path = argv[j];
				i++;
			}
		}

		// fps
		else if (!strcmp (curr_arg, "--fps")) {
			j = i + 1;
			if (j <= argc) {
				config->fps = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// rotation
		else if (!strcmp (curr_arg, "-r")) {
			j = i + 1;
			if (j <= argc) {
				config->rotation = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// scale factor
		else if (!strcmp (curr_arg, "-s")) {
			j = i + 1;
			if (j <= argc) {
				config->scale_factor = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// movement thresh
		else if (!strcmp (curr_arg, "--mov_thresh")) {
			j = i + 1;
			if (j <= argc) {
				config->movement_thresh = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// max_no_movement_frames
		else if (!strcmp (curr_arg, "--max_no_mov")) {
			j = i + 1;
			if (j <= argc) {
				config->max_no_movement_frames = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// max_actions_memory_size
		else if (!strcmp (curr_arg, "--max_actions_memory_size")) {
			j = i + 1;
			if (j <= argc) {
				config->max_actions_memory_size = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// videos_n_loops
		else if (!strcmp (curr_arg, "--videos_n_loops")) {
			j = i + 1;
			if (j <= argc) {
				config->videos_n_loops = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// enable_wait_key
		else if (!strcmp (curr_arg, "--enable_wait_key")) {
			config->enable_wait_key = true;
		}

		// wait_key_delay
		else if (!strcmp (curr_arg, "--wait_key_delay")) {
			j = i + 1;
			if (j <= argc) {
				config->wait_key_delay = (unsigned int) atoi (argv[j]);
				i++;
			}
		}

		// record
		else if (!strcmp (curr_arg, "--record")) {
			config->record = true;
		}

		// get the output dir
		else if (!strcmp (curr_arg, "-o")) {
			j = i + 1;
			if (j <= argc) {
				config->output_path = argv[j];
				i++;
			}
		}

		// get the cameras settings filename
		else if (!strcmp (curr_arg, "--cams")) {
			j = i + 1;
			if (j <= argc) {
				config->cams_settings_filename = argv[j];
				i++;
			}
		}

		// enables connection to the main cerver
		else if (!strcmp (curr_arg, "--connect")) {
			j = i + 1;
			if (j <= argc) {
				config->connect = config_get_bool_from_string (argv[j]);
				i++;
			}
		}

		else {
			client_log_warning ("Unknown argument: %s", curr_arg);
		}
	}

}
