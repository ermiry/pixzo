#include <stdlib.h>

#include <vector>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>

#include <client/types/types.h>

#include <client/collections/pool.h>

#include <client/utils/log.h>

#include "frames.hpp"

static Pool *frames_pool = NULL;

void pixzo_frame_delete_internal (void *pixzo_frame_ptr);

void *pixzo_frame_create (void);

static unsigned int pixzo_frames_init_pool (void) {

	unsigned int retval = 1;

	frames_pool = pool_create (pixzo_frame_delete_internal);
	if (frames_pool) {
		pool_set_create (frames_pool, pixzo_frame_create);
		pool_set_produce_if_empty (frames_pool, true);
		if (!pool_init (frames_pool, pixzo_frame_create, DEFAULT_FRAMES_POOL_INIT)) {
			retval = 0;
		}

		else {
			client_log_error ("Failed to init frames pool!");
		}
	}

	else {
		client_log_error ("Failed to create frames pool!");
	}

	return retval;	

}

unsigned int pixzo_frames_init (void) {

	unsigned int errors = 0;

	errors |= pixzo_frames_init_pool ();

	return errors;

}

void pixzo_frames_end (void) {

	pool_delete (frames_pool);
	frames_pool = NULL;

}

PixzoFrame *pixzo_frame_new (void) {

	PixzoFrame *pixzo_frame = (PixzoFrame *) malloc (sizeof (PixzoFrame));
	if (pixzo_frame) {
		(void) memset (&pixzo_frame->info, 0, sizeof (PixzoFrameInfo));

		pixzo_frame->frame = NULL;
	}

	return pixzo_frame;

}

void pixzo_frame_delete_internal (void *pixzo_frame_ptr) {

	if (pixzo_frame_ptr) {
		PixzoFrame *pixzo_frame = (PixzoFrame *) pixzo_frame_ptr;

		if (pixzo_frame->frame) {
			pixzo_frame->frame->release ();
			delete (pixzo_frame->frame);
		}

		free (pixzo_frame);
	}

}

void pixzo_frame_delete (void *pixzo_frame_ptr) {

	if (pixzo_frame_ptr) {
		PixzoFrame *pixzo_frame = (PixzoFrame *) pixzo_frame_ptr;

		(void) memset (&pixzo_frame->info, 0, sizeof (PixzoFrameInfo));

		if (pixzo_frame->frame) pixzo_frame->frame->release ();

		(void) pool_push (frames_pool, pixzo_frame_ptr);
	}

}

// compares two frames by their ids
int pixzo_frame_comparator (const void *a, const void *b) {

	if (a && b) {
		PixzoFrame *pixzo_frame_a = (PixzoFrame *) a;
		PixzoFrame *pixzo_frame_b = (PixzoFrame *) b;

		if (pixzo_frame_a->info.frame_id < pixzo_frame_b->info.frame_id) return -1;
		else if (pixzo_frame_a->info.frame_id == pixzo_frame_b->info.frame_id) return 0;
		else return 1;
	}

	if (a) return -1;
	else if (b) return 1;
	else return 0;

}

void *pixzo_frame_create (void) {

	PixzoFrame *pixzo_frame = pixzo_frame_new ();
	if (pixzo_frame) {
		pixzo_frame->frame = new cv::Mat ();
	}

	return pixzo_frame;

}

PixzoFrame *pixzo_frame_get (void) {

	return (PixzoFrame *) pool_pop (frames_pool);

}

// encodes a cv::Mat input image into a jpeg image that we can send to the pose cerver
std::vector <uchar> *pixzo_frame_encode_input (cv::Mat &input_image) {

	std::vector <uchar> *input_image_encoded = NULL;

	if (!input_image.empty ()) {
		input_image_encoded = new std::vector <uchar> ();
		if (input_image_encoded) {
			std::vector <int> params;
			params.push_back (cv::IMWRITE_JPEG_QUALITY);
			params.push_back (60);   //image quality
			cv::imencode (".jpg", input_image, *input_image_encoded, params);
		}
	}

	return input_image_encoded;

}