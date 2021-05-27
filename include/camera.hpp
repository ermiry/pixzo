#ifndef _PIXZO_CAMERA_HPP_
#define _PIXZO_CAMERA_HPP_

#include <opencv2/videoio.hpp>

#include <client/types/types.h>
#include <client/types/string.h>

#define CAMERA_NAME_SIZE				256

#define CAMERA_DEFAULT_WIDTH			640
#define CAMERA_DEFAULT_HEIGHT			480
#define CAMERA_DEFAULT_FPS				30

// #define CAMERA_DEFAULT_AUTO_EXPOSURE	1
#define CAMERA_DEFAULT_AUTO_EXPOSURE	0.25
#define CAMERA_DEFAULT_BRIGHTNESS		40
#define CAMERA_DEFAULT_CONTRAST			10
#define CAMERA_DEFAULT_SATURATION		80
#define CAMERA_DEFAULT_GAIN				90
#define CAMERA_DEFAULT_EXPOSURE			155
#define CAMERA_DEFAULT_SHARPNESS		0

#define CAMERA_TYPE_MAP(XX)				\
	XX(0,	NONE, 		None)			\
	XX(1,	MEDIA, 		Media)			\
	XX(2,	IP, 		IP)				\
	XX(3,	VIDEO, 		Video)			\
	XX(4,	OTHER, 		Other)

typedef enum CameraType {

	#define XX(num, name, string) CAMERA_TYPE_##name = num,
	CAMERA_TYPE_MAP (XX)
	#undef XX

} CameraType;

extern const char *camera_type_to_string (CameraType type);

typedef enum CameraRotation {

	CAMERA_ROTATION_NONE					= 0,
	CAMERA_ROTATION_90_CLOCKWISE			= 1,
	CAMERA_ROTATION_90_COUNTERCLOCKWISE		= 2,
	CAMERA_ROTATION_180						= 3,

} CameraRotation;

struct _Camera {

	CameraType type;

	u8 device_idx;				// open normal devices (webcam)

	// open device by its name
	char device_name[CAMERA_NAME_SIZE];

	String *address;			// open an ip camera

	const String *filename;		// open video from file

	CameraRotation rotation;

	unsigned int preferred_width, preferred_height;
	unsigned int real_width, real_height;

	unsigned int preferred_fps;
	unsigned int real_fps;

	double auto_exposure;
	double brightness;
	double contrast;
	double saturation;
	double gain;
	double exposure;
	double sharpness;

	u64 total_frames;
	cv::VideoCapture *capture;

};

typedef struct _Camera Camera;

extern void camera_delete (void *camera_ptr);

extern Camera *camera_create (CameraType type);

// creates a new camera with reference to a physical connected device
extern Camera *camera_create (u8 device_idx);

extern Camera *camera_create (const char *device_name);

// creates a new camera with reference to an IP camera
extern Camera *camera_create_with_address (const char *address);

// creates a new camera that will take video from file as the input
extern Camera *camera_create (const String *filename);

extern void camera_set_source (
	Camera *cam, u8 device_idx
);

extern void camera_set_source (
	Camera *cam, const char *device_name
);

// set the rotation to be applied to every new frame
extern void camera_set_rotation (
	Camera *cam, CameraRotation rotation
);

// sets the prefered resolution
// returns 0 on success, 1 on error
extern unsigned int camera_set_resolution (
	Camera *cam, int width, int height
);

// sets the preferred fps
// returns 0 on success, 1 on error
extern unsigned int camera_set_fps (Camera *cam, int fps);

// opens the camera using the values set on its creation
// returns true on success, false on error
extern bool camera_open (Camera *cam);

// opens a camera using the video file as the input
// camera must be of type CAMERA_TYPE_VIDEO
// returns true on success, false on error
extern bool camera_open (
	Camera *cam, const String *filename
);

// gets the next camera frame
// applies configuration to the new frame
extern u8 camera_get (Camera *cam, cv::Mat *frame);

extern cv::Mat *camera_get (Camera *cam);

// closes the camera's video capture
extern void camera_close (Camera *cam);

extern void camera_print (const Camera *cam);

#endif