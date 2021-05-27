#ifndef _PIXZO_CONFIG_H_
#define _PIXZO_CONFIG_H_

#define CONFIG_DEFAULT_FPS						24

#define CONFIG_DEFAULT_ROTATION					0
#define CONFIG_DEFAULT_SCALE_FACTOR				6

#define CONFIG_DEFAULT_MOVEMENT_THRESH			800
#define CONFIG_DEFAULT_NO_MOVEMENT_FRAMES      	60

#define CONFIG_DEFAULT_MAX_ACTIONS_MEM_SIZE		2

#define CONFIG_DEFAULT_VIDEOS_N_LOOPS			1

#define CONFIG_DEFAULT_ENABLE_WAIT_KEY			false
#define CONFIG_DEFAULT_WAIT_KEY_DELAY	      	100

#define CONFIG_DEFAULT_RECORD					false

#define CONFIG_DEFAULT_CAMS_SETTINGS			"config/cams.json"

#define CONFIG_DEFAULT_CONNECT					true

typedef struct Config {

	const char *type;

	const char *store_id;
	const char *store_name;

	const char *camera_name;
	const char *videos_path;

	unsigned int fps;

	unsigned int rotation;
	unsigned int scale_factor;

	unsigned int movement_thresh;
	unsigned int max_no_movement_frames;

	unsigned int max_actions_memory_size;

	unsigned int videos_n_loops;

	bool enable_wait_key;
	unsigned int wait_key_delay;

	bool record;
	const char *output_path;

	const char *cams_settings_filename;

	bool connect;

} Config;

extern void config_init (Config *config);

extern unsigned int config_validate_single (
	const Config *config
);

extern unsigned int config_validate_videos (
	const Config *config
);

extern unsigned int config_validate_load (
	const Config *config
);

extern unsigned int config_validate_register (
	const Config *config
);

extern void config_log (const Config *config);

extern void config_help (void);

extern void config_get_values (
	Config *config,
	int argc, char **argv
);

#endif