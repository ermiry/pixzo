#ifndef _PIXZO_STREAM_HPP_
#define _PIXZO_STREAM_HPP_

#include <stdbool.h>

#include <pthread.h>

#include <client/types/types.h>
#include <client/types/string.h>

#include <client/threads/thread.h>
#include <client/threads/jobs.h>

#include <client/collections/dlist.h>

#include "camera.hpp"
#include "store.h"

#define STREAM_NAME_SIZE							128

#define STREAM_VIDEO_OUTPUT_FILENAME_SIZE			1024

#define DEFAULT_STREAM_MEMORY_MAX_SIZE				50
#define DEFAULT_STREAM_MEMORY_BATCH_SIZE			10

#define DEFAULT_STREAM_POSE_WIDTH_SCALE				1
#define DEFAULT_STREAM_POSE_HEIGHT_SCALE			1

#define DEFAULT_STREAM_POSE_OUPUT_WIDTH				150
#define DEFAULT_STREAM_POSE_OUPUT_HEIGHT			150

#define DEFAULT_STREAM_FPS                     		24

#define DEFAULT_STREAM_SCALE_FACTOR					6
#define DEFAULT_STREAM_MOVEMENT_THRESH				800
#define DEFAULT_STREAM_NO_MOVEMENT_FRAMES      		60

struct _Store;

#define STREAM_TYPE_MAP(XX)				\
	XX(0,	NONE, 		None)			\
	XX(1,	GENERAL, 	General)		\
	XX(2,	FACE, 		Face)			\
	XX(3,	CUSTOM, 	Custom)

typedef enum StreamType {

	#define XX(num, name, string) STREAM_TYPE_##name = num,
	STREAM_TYPE_MAP (XX)
	#undef XX

} StreamType;

extern const char *stream_type_to_string (StreamType type);

#pragma region main

struct _Stream {

	// the store this stream belongs to
	struct _Store *store;

	u32 id;
	char name[STREAM_NAME_SIZE];
	StreamType type;

	pthread_t stream_thread_id;

	DoubleList *videos;			// list of videos to use as inputs

	Camera *cam;
	unsigned int width, height;

	unsigned int scale_factor;

	JobQueue *frames_buffer;

	pthread_t movement_thread_id;
	bool movement;
	unsigned int movement_count;
	unsigned int no_movement_frames;
	unsigned int movement_thresh;
	unsigned int max_no_movement_frames;

	pthread_t record_thread_id;

	u64 next_frame_id;
	u32 raw_frame_saved_count;
	
	cv::Size pose_size;
	float pose_width_scale;
	float pose_height_scale;
	float pose_output_width;
	float pose_output_height;

	int pose_output_x_offset;
	int pose_output_y_offset;

	cv::VideoWriter *writer;
	char video_output[STREAM_VIDEO_OUTPUT_FILENAME_SIZE];

	// stats
	u64 n_frames_read;			// total number of capture.read (input_image) performed
	u64 n_frames_good;			// good input frames 
	u64 n_frames_bad;			// bad input frames

};

typedef struct _Stream Stream;

extern Stream *stream_new (void);

extern void stream_delete (void *stream_ptr);

extern int stream_comparator_by_id (
	const void *a, const void *b
);

// sets the stream's name to be used for output filenames
extern void stream_set_name (
	Stream *stream, const char *name
);

// sets the stream's type
// can only be called when a new stream is created
extern void stream_set_type (
	Stream *stream, StreamType type
);

// sets the stream's scale factor to be used
// when calculating movement in frame
extern void stream_set_scale_factor (
	Stream *stream, unsigned int scale_factor
);

// sets the stream's parameters to be used to
// calculate movement & determine when an actions starts & ends
extern void stream_set_movement_values (
	Stream *stream,
	unsigned int movement_thresh, unsigned int max_no_movement_frames
);

extern void stream_set_movement_thresh (
	Stream *stream, unsigned int movement_thresh
);

extern void stream_set_max_no_movement_frames (
	Stream *stream, unsigned int max_no_movement_frames
);

// scale raw frame (into a new one) to this size to be used as pose input
extern int stream_set_pose_size (
	Stream *stream, int width, int height
);

// sets the size of the preferred size of the rectangle to use for local frame resolution
// to create the hands frames based on hand coords from pose in hand_frame_create ()
extern int stream_set_pose_ouput_size (
	Stream *stream, int width, int height
);

// sets the offset to be used when recreating frames for services using the pose output coords
extern void stream_set_pose_output_offset (
	Stream *stream, int x_offset, int y_offset
);

// sets the stream's values now that we have all the required info
// called automatically in store_stream_thread () after the camera has been opened
extern int stream_populate_values (Stream *stream);

// creates and sets a new video writer to save a video
// returns 0 on success, 1 on error
extern unsigned int stream_set_video_writer (
	Stream *stream
);

// ends the current stream's video writer recording
// returns 0 on success, 1 on error
extern unsigned int stream_close_video_writer (
	Stream *stream
);

// stream's id is set when it is registered to a store
extern Stream *stream_create (
	StreamType type, Camera *cam
);

extern Stream *stream_create (
	struct _Store *store, u32 id, Camera *cam
);

extern void stream_print (const Stream *stream);

#pragma endregion

#pragma region frames

// get the original frame by searching it by its id
// in the stream's actions memories
extern struct _PixzoFrame *stream_memory_get_frame (
	Stream *stream, u32 action_id, u64 frame_id
);

#pragma endregion

#pragma region thread

// opens the stream's camera (actual device) & gets all remaining values
// returns 0 on success, 1 on error
extern unsigned int stream_open (Stream *stream);

extern void *stream_movement_thread (void *stream_ptr);

extern void *stream_record_thread (void *stream_ptr);

// dedicated thread for each stream to read from its camera
extern void *stream_thread (void *stream_ptr);

extern void *stream_videos_thread (void *stream_ptr);

#pragma endregion

#endif