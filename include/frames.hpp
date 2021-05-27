#ifndef _PIXZO_FRAMES_HPP_
#define _PIXZO_FRAMES_HPP_

#include <stdlib.h>

#include <time.h>

#include <vector>

#include <opencv2/core/mat.hpp>

#include <client/types/types.h>

#define DEFAULT_FRAMES_POOL_INIT			64

extern unsigned int pixzo_frames_init (void);

extern void pixzo_frames_end (void);

struct _PixzoFrameInfo {

	char store_id[32];			// the store this frame belongs to
	u32 stream_id;				// the stream this frame belongs to
	u64 frame_id;         		// the unique id of this frame
	u32 action_id;				// thec action this frame belongs to
	time_t timestamp;           // the time when the frame was taken

	unsigned int width;
	unsigned int height;

};

typedef struct _PixzoFrameInfo PixzoFrameInfo;

struct _PixzoFrame {

	PixzoFrameInfo info;

	cv::Mat *frame;				// the original frame that we read from media device

};

typedef struct _PixzoFrame PixzoFrame;

extern PixzoFrame *pixzo_frame_new (void);

extern void pixzo_frame_delete_internal (
	void *pixzo_frame_ptr
);

extern void pixzo_frame_delete (
	void *pixzo_frame_ptr
);

// compares two frames by their ids
extern int pixzo_frame_comparator (
	const void *a, const void *b
);

// creates a new pixzo frame
extern void *pixzo_frame_create (void);

extern PixzoFrame *pixzo_frame_get (void);

// encodes a cv::Mat input image into a jpeg image
// that can be sent to the pose cerver
extern std::vector <uchar> *pixzo_frame_encode_input (
	cv::Mat &input_image
);

#endif