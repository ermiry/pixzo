#include <stdlib.h>
#include <stdbool.h>

#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>

#include <client/types/types.h>
#include <client/types/string.h>

#include <client/utils/utils.h>
#include <client/utils/log.h>

#include "camera.hpp"

static void camera_print_config (
	const Camera *cam
);

static void camera_print_real_capture_config (
	cv::VideoCapture *capture
);

const char *camera_type_to_string (CameraType type) {

	switch (type) {
		#define XX(num, name, string) case CAMERA_TYPE_##name: return #string;
		CAMERA_TYPE_MAP(XX)
		#undef XX
	}

	return camera_type_to_string (CAMERA_TYPE_NONE);

}

#pragma region main

static Camera *camera_new (void) {

	Camera *cam = (Camera *) malloc (sizeof (Camera));
	if (cam) {
		cam->type = CAMERA_TYPE_NONE;

		cam->device_idx = 0;
		(void) memset (cam->device_name, 0, CAMERA_NAME_SIZE);

		cam->address = NULL;
		cam->filename = NULL;

		cam->preferred_width = CAMERA_DEFAULT_WIDTH;
		cam->preferred_height = CAMERA_DEFAULT_HEIGHT;
		cam->preferred_fps = CAMERA_DEFAULT_FPS;

		cam->real_width = cam->real_height = cam->real_fps = 0;

		cam->auto_exposure = CAMERA_DEFAULT_AUTO_EXPOSURE;
		cam->brightness = CAMERA_DEFAULT_BRIGHTNESS;
		cam->contrast = CAMERA_DEFAULT_CONTRAST;
		cam->saturation = CAMERA_DEFAULT_SATURATION;
		cam->gain = CAMERA_DEFAULT_GAIN;
		cam->exposure = CAMERA_DEFAULT_EXPOSURE;
		cam->sharpness = CAMERA_DEFAULT_SHARPNESS;

		cam->total_frames = 0;
		cam->capture = NULL;
	}

	return cam;

}

void camera_delete (void *camera_ptr) {

	if (camera_ptr) {
		Camera *cam = (Camera *) camera_ptr;

		str_delete (cam->address);

		if (cam->capture) {
			cam->capture->release ();
			delete (cam->capture);
		}

		free (cam);
	}

}

Camera *camera_create (CameraType type) {

	Camera *cam = camera_new ();
	if (cam) {
		cam->capture = new cv::VideoCapture ();
		if (cam->capture) {
			cam->type = type;
		}

		else {
			free (cam);
			cam = NULL;
		}
	}

	return cam;

}

// creates a new camera with reference to a physical connected device
Camera *camera_create (u8 device_idx) {

	Camera *cam = camera_create (CAMERA_TYPE_MEDIA);
	if (cam) {
		cam->device_idx = device_idx;
	}

	return cam;

}

Camera *camera_create (const char *device_name) {

	Camera *cam = camera_create (CAMERA_TYPE_MEDIA);
	if (cam) {
		(void) strncpy (cam->device_name, device_name, CAMERA_NAME_SIZE - 1);
	}

	return cam;

}

// creates a new camera with reference to an IP camera
Camera *camera_create_with_address (const char *address) {

	Camera *cam = NULL;

	if (address) {
		cam = camera_create (CAMERA_TYPE_IP);
		if (cam) {
			cam->address = str_new (address);
		}
	}

	return cam;

}

// creates a new camera that will take video from file as the input
Camera *camera_create (const String *filename) {

	Camera *cam = NULL;

	if (filename) {
		cam = camera_create (CAMERA_TYPE_VIDEO);
		if (cam) {
			cam->filename = filename;
		}
	}

	return cam;

}

void camera_set_source (Camera *cam, u8 device_idx) {

	if (cam) {
		cam->device_idx = device_idx;
	}

}

void camera_set_source (Camera *cam, const char *device_name) {

	if (cam) {
		(void) strncpy (cam->device_name, device_name, CAMERA_NAME_SIZE - 1);
	}

}

// set the rotation to be applied to every new frame
void camera_set_rotation (
	Camera *cam, CameraRotation rotation
) {

	if (cam) cam->rotation = rotation;

}

// sets the preferred resolution
// returns 0 on success, 1 on error
unsigned int camera_set_resolution (
	Camera *cam, int width, int height
) {

	unsigned int retval = 1;

	if (cam) {
		if (cam->capture) {
			cam->preferred_width = width;
			cam->preferred_height = height;

			retval = 0;
		}
	}

	return retval;

}

// sets the preferred fps
// returns 0 on success, 1 on error
unsigned int camera_set_fps (Camera *cam, int fps) {

	unsigned int retval = 1;

	if (cam) {
		if (cam->capture) {
			cam->preferred_fps = fps;

			retval = 0;
		}
	}

	return retval;

}

static void camera_set_capture_value (
	cv::VideoCapture *capture,
	const int prop_id, const char *name, const double value
) {

	if (capture->set (prop_id, value)) {
		client_log_debug (
			"Set %s to %.2f",
			name, value
		);
	}

	else {
		client_log_error (
			"Failed to set %s to %.2f",
			name, value
		);
	}

}

static bool camera_open_internal (Camera *cam) {

	bool retval = false;

	switch (cam->type) {
		case CAMERA_TYPE_MEDIA: {
			if (cam->device_name) {
				retval = cam->capture->open (cam->device_name);
			}

			else {
				retval = cam->capture->open (cam->device_idx);
			}
		} break;

		case CAMERA_TYPE_IP: retval = cam->capture->open (cam->address->str); break;
		case CAMERA_TYPE_VIDEO: retval = cam->capture->open (cam->filename->str); break;

		default: break;
	}

	if (cam->capture->isOpened ()) {
		// set preferred options for camera
		(void) cam->capture->set (CV_CAP_PROP_FOURCC, CV_FOURCC ('M', 'J', 'P', 'G'));
		(void) cam->capture->set (CV_CAP_PROP_FPS, cam->preferred_fps);
		(void) cam->capture->set (CV_CAP_PROP_FRAME_WIDTH, cam->preferred_width);
		(void) cam->capture->set (CV_CAP_PROP_FRAME_HEIGHT, cam->preferred_height);

		switch (cam->rotation) {
			case CAMERA_ROTATION_90_CLOCKWISE:
				cam->real_height = cam->capture->get (cv::CAP_PROP_FRAME_WIDTH);
				cam->real_width = cam->capture->get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
			case CAMERA_ROTATION_90_COUNTERCLOCKWISE:
				cam->real_height = cam->capture->get (cv::CAP_PROP_FRAME_WIDTH);
				cam->real_width = cam->capture->get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
			case CAMERA_ROTATION_180:
				cam->real_width = cam->capture->get (cv::CAP_PROP_FRAME_WIDTH);
				cam->real_height = cam->capture->get (cv::CAP_PROP_FRAME_HEIGHT);
				break;

			default:
				cam->real_width = cam->capture->get (cv::CAP_PROP_FRAME_WIDTH);
				cam->real_height = cam->capture->get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
		}

		cam->real_fps = cam->capture->get (CV_CAP_PROP_FPS);

		// #ifdef PIXZO_DEBUG
		switch (cam->type) {
			case CAMERA_TYPE_MEDIA: {
				camera_print_config (cam);

				camera_print_real_capture_config (cam->capture);

				camera_set_capture_value (cam->capture, CV_CAP_PROP_AUTO_EXPOSURE, "auto exposure", cam->auto_exposure);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_BRIGHTNESS, "brightness", cam->brightness);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_CONTRAST, "contrast", cam->contrast);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_SATURATION, "saturation", cam->saturation);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_GAIN, "gain", cam->gain);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_EXPOSURE, "exposure", cam->exposure);
				camera_set_capture_value (cam->capture, CV_CAP_PROP_SHARPNESS, "sharpness", cam->sharpness);

				if (cam->device_name) {
					client_log_debug (
						"Video capture info for <%s> is: w: %d x h: %d -- fps: %d",
						cam->device_name,
						cam->real_width, cam->real_height, cam->real_fps
					);
				}

				else {
					client_log_debug (
						"Video capture info for <%d> is: w: %d x h: %d -- fps: %d",
						cam->device_idx,
						cam->real_width, cam->real_height, cam->real_fps
					);
				}

				camera_print_real_capture_config (cam->capture);
			} break;
			case CAMERA_TYPE_IP: {
				client_log_debug (
					"Video capture info for <%s> is: w: %d x h: %d -- fps: %d",
					cam->address->str,
					cam->real_width, cam->real_height, cam->real_fps
				);
			} break;
			case CAMERA_TYPE_VIDEO: {
				client_log_debug (
					"Video capture info for <%s> is: w: %d x h: %d -- fps: %d",
					cam->filename->str,
					cam->real_width, cam->real_height, cam->real_fps
				);
			} break;

			default: break;
		}
		// #endif
	}

	return retval;

}

// opens the camera using the values set on its creation
// returns true on success, false on error
bool camera_open (Camera *cam) {

	return cam ? camera_open_internal (cam) : false;

}

// opens a camera using the video file as the input
// camera must be of type CAMERA_TYPE_VIDEO
// returns true on success, false on error
bool camera_open (Camera *cam, const String *filename) {

	bool retval = false;

	if (cam && filename) {
		cam->filename = filename;
		retval = camera_open_internal (cam);
	}

	return retval;

}

// gets the next camera frame
// applies configuration to the new frame
u8 camera_get (Camera *cam, cv::Mat *frame) {

	u8 retval = 1;

	*cam->capture >> *frame;

	if (!frame->empty ()) {
		switch (cam->rotation) {
			case CAMERA_ROTATION_90_CLOCKWISE:
				cv::rotate (*frame, *frame, cv::ROTATE_90_CLOCKWISE);
				break;

			case CAMERA_ROTATION_90_COUNTERCLOCKWISE:
				cv::rotate (*frame, *frame, cv::ROTATE_90_COUNTERCLOCKWISE);
				break;

			case CAMERA_ROTATION_180:
				cv::rotate (*frame, *frame, cv::ROTATE_180);
				break;

			default: break;
		}

		retval = 0;
	}

	return retval;

}

cv::Mat *camera_get (Camera *cam) {

	cv::Mat *frame = new cv::Mat ();
	if (frame) {
		(void) camera_get (cam, frame);
	}

	return frame;

}

// closes the camera's video capture
void camera_close (Camera *cam) {

	if (cam) {
		if (cam->capture) {
			cam->capture->release ();
		}
	}

}

void camera_print (const Camera *cam) {

	if (cam) {
		(void) printf ("\t\tCamera type: %s\n", camera_type_to_string (cam->type));

		switch (cam->type) {
			case CAMERA_TYPE_NONE: break;

			case CAMERA_TYPE_MEDIA: {
				if (cam->device_name) {
					(void) printf ("\t\tDevice idx: %d\n", cam->device_idx);
				}

				else {
					(void) printf ("\t\tDevice name: %s\n", cam->device_name);
				}
			} break;

			case CAMERA_TYPE_IP:
				(void) printf ("\t\tAddress: %s\n", cam->address->str);
				break;

			default: break;
		}

		(void) printf ("\t\tPreferred width: %u\n", cam->preferred_width);
		(void) printf ("\t\tPreferred height: %u\n", cam->preferred_height);
		(void) printf ("\t\tPreferred fps: %u\n", cam->preferred_fps);

		(void) printf ("\t\tReal width: %u\n", cam->real_width);
		(void) printf ("\t\tReal height: %u\n", cam->real_height);
		(void) printf ("\t\tReal fps: %u\n", cam->real_fps);

		(void) printf ("\t\tAuto exposure: %.2f\n", cam->auto_exposure);
		(void) printf ("\t\tBrightness: %.2f\n", cam->brightness);
		(void) printf ("\t\tContrast: %.2f\n", cam->contrast);
		(void) printf ("\t\tSaturation: %.2f\n", cam->saturation);
		(void) printf ("\t\tGain: %.2f\n", cam->gain);
		(void) printf ("\t\tExposure: %.2f\n", cam->exposure);
		(void) printf ("\t\tSharpness: %.2f\n", cam->sharpness);
	}

}

static void camera_print_config (
	const Camera *cam
) {

	(void) printf ("Camera Configuration: \n");
	(void) printf ("\tAuto exposure: %.2f\n", cam->auto_exposure);
	(void) printf ("\tBrightness: %.2f\n", cam->brightness);
	(void) printf ("\tContrast: %.2f\n", cam->contrast);
	(void) printf ("\tSaturation: %.2f\n", cam->saturation);
	(void) printf ("\tGain: %.2f\n", cam->gain);
	(void) printf ("\tExposure: %.2f\n", cam->exposure);
	(void) printf ("\tSharpness: %.2f\n\n", cam->sharpness);

}

static void camera_print_real_capture_config (
	cv::VideoCapture *capture
) {

	(void) printf ("Capture REAL Configuration: \n");
	(void) printf ("\tAuto Exposure: %.2f\n", capture->get (cv::CAP_PROP_AUTO_EXPOSURE));
	(void) printf ("\tBrightness: %.2f\n", capture->get (cv::CAP_PROP_BRIGHTNESS));
	(void) printf ("\tContrast: %.2f\n", capture->get (cv::CAP_PROP_CONTRAST));
	(void) printf ("\tSaturation: %.2f\n", capture->get (cv::CAP_PROP_SATURATION));
	(void) printf ("\tGain: %.2f\n", capture->get (cv::CAP_PROP_GAIN));
	(void) printf ("\tExposure: %.2f\n", capture->get (cv::CAP_PROP_EXPOSURE));
	(void) printf ("\tSharpness: %.2f\n\n", capture->get (cv::CAP_PROP_SHARPNESS));

}

#pragma endregion
