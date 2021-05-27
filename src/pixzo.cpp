#include <stdlib.h>
#include <stdio.h>

#include <client/types/types.h>

#include <client/client.h>
#include <client/connection.h>
#include <client/events.h>
#include <client/files.h>

#include <client/json/json.h>

#include <client/utils/log.h>
#include <client/utils/utils.h>

#include "camera.hpp"
#include "errors.h"
#include "frames.hpp"
#include "global.h"
#include "pixzo.h"
#include "store.h"
#include "stream.hpp"

void pixzo_close (void);

#pragma region start

static void pixzo_init_store_create_camera_resolution (
	Camera *cam, json_t *res_object
) {

	int width = 0;
	int height = 0;

	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (res_object) == JSON_OBJECT) {
		json_object_foreach (res_object, key, value) {
			if (!strcmp (key, "width")) {
				width = (int) json_integer_value (value);
			}

			else if (!strcmp (key, "height")) {
				height = (int) json_integer_value (value);
			}
		}
	}

	camera_set_resolution (cam, width, height);

}

static Camera *pixzo_init_store_create_camera (
	json_t *cam_json
) {

	Camera *cam = camera_create (CAMERA_TYPE_MEDIA);

	const char *key = NULL;
	json_t *value = NULL;
	json_object_foreach (cam_json, key, value) {
		if (!strcmp (key, "source")) {
			camera_set_source (cam, json_string_value (value));
		}

		else if (!strcmp (key, "rotation")) {
			camera_set_rotation (
				cam, (CameraRotation) json_integer_value (value)
			);
		}

		else if (!strcmp (key, "resolution")) {
			pixzo_init_store_create_camera_resolution (
				cam, value
			);
		}

		else if (!strcmp (key, "fps")) {
			(void) camera_set_fps (cam, (int) json_integer_value (value));
		}

		else if (!strcmp (key, "auto_exposure")) {
			cam->auto_exposure = json_real_value (value);
		}

		else if (!strcmp (key, "brightness")) {
			cam->brightness = json_real_value (value);
		}

		else if (!strcmp (key, "contrast")) {
			cam->contrast = json_real_value (value);
		}

		else if (!strcmp (key, "saturation")) {
			cam->saturation = json_real_value (value);
		}

		else if (!strcmp (key, "gain")) {
			cam->gain = json_real_value (value);
		}

		else if (!strcmp (key, "exposure")) {
			cam->exposure = json_real_value (value);
		}

		else if (!strcmp (key, "sharpness")) {
			cam->sharpness = json_real_value (value);
		}
	}

	return cam;

}

static void pixzo_init_store_create_stream_pose_size (
	Stream *stream, json_t *pose_size_object
) {

	int width = 0;
	int height = 0;

	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (pose_size_object) == JSON_OBJECT) {
		json_object_foreach (pose_size_object, key, value) {
			if (!strcmp (key, "width")) {
				width = (int) json_integer_value (value);
			}

			else if (!strcmp (key, "height")) {
				height = (int) json_integer_value (value);
			}
		}
	}

	(void) stream_set_pose_size (stream, width, height);

}

static void pixzo_init_store_create_stream_pose_output_size (
	Stream *stream, json_t *pose_output_size_object
) {

	int width = 0;
	int height = 0;

	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (pose_output_size_object) == JSON_OBJECT) {
		json_object_foreach (pose_output_size_object, key, value) {
			if (!strcmp (key, "width")) {
				width = (int) json_integer_value (value);
			}

			else if (!strcmp (key, "height")) {
				height = (int) json_integer_value (value);
			}
		}
	}

	(void) stream_set_pose_ouput_size (stream, width, height);

}

static void pixzo_init_store_create_stream_pose_output_offset (
	Stream *stream, json_t *pose_output_offset_object
) {

	int x = 0;
	int y = 0;

	const char *key = NULL;
	json_t *value = NULL;
	if (json_typeof (pose_output_offset_object) == JSON_OBJECT) {
		json_object_foreach (pose_output_offset_object, key, value) {
			if (!strcmp (key, "x")) {
				x = (int) json_integer_value (value);
			}

			else if (!strcmp (key, "y")) {
				y = (int) json_integer_value (value);
			}
		}
	}

	stream_set_pose_output_offset (stream, x, y);

}

static Stream *pixzo_init_store_create_stream (
	Camera *cam, json_t *cam_json
) {

	Stream *stream = stream_create (STREAM_TYPE_GENERAL, cam);

	const char *key = NULL;
	json_t *value = NULL;
	json_object_foreach (cam_json, key, value) {
		if (!strcmp (key, "id")) {
			stream->id = (u32) json_integer_value (value);
		}

		else if (!strcmp (key, "name")) {
			stream_set_name (stream, json_string_value (value));
		}

		else if (!strcmp (key, "pose_size")) {
			pixzo_init_store_create_stream_pose_size (
				stream, value
			);
		}

		else if (!strcmp (key, "pose_output_size")) {
			pixzo_init_store_create_stream_pose_output_size (
				stream, value
			);
		}

		else if (!strcmp (key, "pose_output_offset")) {
			pixzo_init_store_create_stream_pose_output_offset (
				stream, value
			);
		}

		else if (!strcmp (key, "scale_factor")) {
			(void) stream_set_scale_factor (stream, (unsigned int) json_integer_value (value));
		}

		else if (!strcmp (key, "movement_thresh")) {
			(void) stream_set_movement_thresh (stream, (unsigned int) json_integer_value (value));
		}

		else if (!strcmp (key, "max_no_movement_frames")) {
			(void) stream_set_max_no_movement_frames (stream, (unsigned int) json_integer_value (value));
		}
	}

	return stream;

}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static u8 pixzo_init_store_get_cameras (void) {

	u8 errors = 0;

	json_error_t json_error =  { 0 };
	json_t *json = json_load_file (
		global->config.cams_settings_filename, 0, &json_error
	);

	if (json) {
		// get values from json to create streams
		u8 n_cams = 0;

		Camera *cam = NULL;
		Stream *stream = NULL;

		const char *key = NULL;
		json_t *value = NULL;
		if (json_typeof (json) == JSON_OBJECT) {
			json_object_foreach (json, key, value) {
				if (!strcmp (key, "cams")) {
					n_cams = (u8) json_array_size (value);
					for (size_t i = 0; i < n_cams; i++) {
						cam = pixzo_init_store_create_camera (
							json_array_get (value, i)
						);

						stream = pixzo_init_store_create_stream (
							cam, json_array_get (value, i)
						);

						errors |= (u8) store_stream_register (global->store, stream);
					}
				}
			}
		}

		json_decref (json);
	}

	else {
		client_log_error (
			"pixzo_init_store_get_cameras () - json error on line %d: %s\n",
			json_error.line, json_error.text
		);
	}

	return errors;

}

#pragma GCC diagnostic pop

static u8 pixzo_init_store_single (void) {

	u8 retval = 1;

	if (!pixzo_init_store_get_cameras ()) {
		if (global->config.record) {
			// create streams output paths
			ListElement *le = NULL;
			Stream *stream = NULL;
			char buffer[STREAM_VIDEO_OUTPUT_FILENAME_SIZE] = { 0 };
			dlist_for_each (global->store->streams, le) {
				stream = (Stream *) le->data;

				(void) snprintf (
					buffer,
					STREAM_VIDEO_OUTPUT_FILENAME_SIZE - 1,
					"%s/%s",
					global->config.output_path,
					stream->name
				);

				(void) files_create_dir (buffer, 0777);
			}
		}

		retval = 0;
	}

	return retval;

}

static u8 pixzo_init_store_videos (void) {

	u8 retval = 1;

	DoubleList *videos = files_get_from_dir (global->config.videos_path);
	if (videos->size) {
		Camera *cam_1 = camera_create (CAMERA_TYPE_VIDEO);
		camera_set_resolution (cam_1, 3840, 2160);
		// camera_set_fps (cam_1, 30);
		Stream *stream_1 = stream_create (STREAM_TYPE_GENERAL, cam_1);
		stream_1->videos = videos;
		// stream_set_pose_size (stream_1, 640, 480);
		stream_set_pose_size (stream_1, 540, 960);
		// stream_set_pose_ouput_size (stream_1, 450, 450);
		stream_set_pose_ouput_size (stream_1, 150, 150);
		// stream_set_pose_output_offset (stream_1, 225, 225);
		stream_set_pose_output_offset (stream_1, 0, 0);
		store_stream_register (global->store, stream_1);

		retval = 0;
	}

	else {
		(void) printf ("\n");
		client_log_error ("No videos!");
		(void) printf ("\n");

		dlist_delete (videos);
	}

	return retval;

}

static u8 pixzo_init_store_record (void) {

	// TODO:

}

static u8 pixzo_init_store (void) {

	u8 retval = 1;

	(void) strncpy (global->store->store_id, global->config.store_id, STORE_ID_SIZE - 1);
	(void) strncpy (global->store->name, global->config.store_name, STORE_NAME_SIZE - 1);

	switch (global->type) {
		case PIXZO_GLOBAL_TYPE_SINGLE: {
			retval = pixzo_init_store_single ();
		} break;

		case PIXZO_GLOBAL_TYPE_VIDEOS: {
			retval = pixzo_init_store_videos ();
		} break;

		case PIXZO_GLOBAL_TYPE_LOAD: break;

		case PIXZO_GLOBAL_TYPE_REGISTER: break;

		case PIXZO_GLOBAL_TYPE_RECORD: {
			retval = pixzo_init_store_record ();
		} break;

		default: break;
	}

	return retval;

}

// init local pixzo values like the local store & streams
static u8 pixzo_init (void) {

	u8 retval = 1;

	if (!pixzo_frames_init ()) {
		retval = pixzo_init_store ();
	}

	return retval;

}

void pixzo_main_thread (void) {

	while (1) {
		// pixzo_send_test ();

		(void) sleep (1);
	}

}

static void pixzo_start_record (void) {

	if (!pixzo_init ()) {
		if (!store_open (global->store)) {
			store_print (global->store);

			// TODO: maybe start store here?

			pixzo_main_thread ();
		}

		else {
			client_log_debug (
				"Failed to open store %s!",
				global->store->name
			);
		}
	}

}

void pixzo_start (void) {

	switch (global->type) {
		case PIXZO_GLOBAL_TYPE_SINGLE:
		case PIXZO_GLOBAL_TYPE_VIDEOS:
			break;

		case PIXZO_GLOBAL_TYPE_LOAD:
			break;

		case PIXZO_GLOBAL_TYPE_REGISTER:
			break;

		case PIXZO_GLOBAL_TYPE_RECORD:
			pixzo_start_record ();
			break;

		default: break;
	}

}

static PixzoError pixzo_open_actual (void) {

	PixzoError retval = PIXZO_ERROR_NONE;

	switch (global->type) {
		case PIXZO_GLOBAL_TYPE_SINGLE:
		case PIXZO_GLOBAL_TYPE_VIDEOS: {
			// start the store --- start reading from media devices
			if (!store_start (global->store)) {
				global_set_status (PIXZO_GLOBAL_STATUS_WORK);
			}

			else {
				retval = PIXZO_ERROR_STORE_START;
			}
		} break;

		default: break;
	}

	return retval;

}

// once we have connected to the main cerver
// we can send a request to register a new store
// and upon a success request, we can now start the client to receive packets
// and also start the local store & start streaming
static PixzoError pixzo_open (void) {

	PixzoError retval = PIXZO_ERROR_NONE;

	switch (global_get_status ()) {
		case PIXZO_GLOBAL_STATUS_READY: {
			(void) sleep (1);

			pixzo_open_actual ();
		} break;

		case PIXZO_GLOBAL_STATUS_ERROR:
		default: retval = PIXZO_ERROR_STORE_REGISTER; break;
	}

	return retval;

}

#pragma endregion

#pragma region end

static u8 pixzo_end (void) {

	client_log_debug ("Closing %s...", global->store->name);

	store_close (global->store);

	pixzo_frames_end ();

	// give a grace period for all streams to stop
	client_log_warning ("Exiting in (3)...");
	(void) sleep (1);
	client_log_warning ("Exiting in (2)...");
	(void) sleep (1);
	client_log_warning ("Exiting in (1)...");
	(void) sleep (1);

	return 0;

}

void pixzo_close (void) {

	(void) printf ("\n\n");
	client_log_debug ("Ending...");
	(void) printf ("\n");

	// close the store locally
	pixzo_end ();

	global_end ();

	(void) printf ("\n");
	client_log_success ("Done!");
	(void) printf ("\n");

	client_end ();

	exit (0);

}

void pixzo_quit (int dummy) { pixzo_close (); }

#pragma endregion