#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <client/config.h>

#include <time.h>

#include <opencv2/imgproc.hpp>					// for resize
#include <opencv2/highgui/highgui.hpp>			// used for cv:cvWaitKey ();

#include <client/types/types.h>
#include <client/types/string.h>

#include <client/collections/dlist.h>

#include <client/client.h>
#include <client/packets.h>

#include <client/threads/thread.h>
#include <client/threads/jobs.h>

#include <client/utils/utils.h>
#include <client/utils/log.h>

#include "camera.hpp"
#include "frames.hpp"
#include "global.h"
#include "stream.hpp"

const char *stream_type_to_string (StreamType type) {

	switch (type) {
		#define XX(num, name, string) case STREAM_TYPE_##name: return #string;
		STREAM_TYPE_MAP(XX)
		#undef XX
	}

	return stream_type_to_string (STREAM_TYPE_NONE);

}

#pragma region main

Stream *stream_new (void) {

	Stream *stream = (Stream *) malloc (sizeof (Stream));
	if (stream) {
		stream->store = NULL;

		stream->id = 0;
		(void) memset (stream->name, 0, STREAM_NAME_SIZE);
		stream->type = STREAM_TYPE_NONE;

		stream->stream_thread_id = 0;

		stream->videos = NULL;

		stream->cam = NULL;
		stream->width = stream->height = 0;

		stream->scale_factor = 0;

		stream->frames_buffer = NULL;

		stream->movement_thread_id = 0;
		stream->movement = false;
		stream->movement_count = 0;
		stream->no_movement_frames = 0;
		stream->movement_thresh = 0;
		stream->max_no_movement_frames = 0;

		stream->record_thread_id = 0;

		stream->next_frame_id = 0;
		stream->raw_frame_saved_count = 0;

		stream->pose_size.width = 0;
		stream->pose_size.height = 0;

		stream->pose_width_scale = DEFAULT_STREAM_POSE_WIDTH_SCALE;
		stream->pose_height_scale = DEFAULT_STREAM_POSE_HEIGHT_SCALE;

		stream->pose_output_width = DEFAULT_STREAM_POSE_OUPUT_WIDTH;
		stream->pose_output_height = DEFAULT_STREAM_POSE_OUPUT_HEIGHT;

		stream->pose_output_x_offset = 0;
		stream->pose_output_y_offset = 0;

		stream->writer = NULL;

		stream->n_frames_read = 0;
		stream->n_frames_good = 0;
		stream->n_frames_bad = 0;
	}

	return stream;

}

void stream_delete (void *stream_ptr) {

	if (stream_ptr) {
		Stream *stream = (Stream *) stream_ptr;

		stream->store = NULL;

		dlist_delete (stream->videos);

		camera_delete (stream->cam);

		job_queue_delete (stream->frames_buffer);

		free (stream);
	}

}

int stream_comparator_by_id (
	const void *a, const void *b
) {

	if (a && b) {
		Stream *stream_a = (Stream *) a;
		Stream *stream_b = (Stream *) b;

		if (stream_a->id < stream_b->id) return -1;
		else if (stream_a->id == stream_b->id) return 0;
		else return 1;
	}

	return -1;

}

// sets the stream's name to be used for output filenames
void stream_set_name (
	Stream *stream, const char *name
) {

	if (stream && name) {
		(void) strncpy (stream->name, name, STREAM_NAME_SIZE - 1);
	}

}

// sets the stream's scale factor to be used
// when calculating movement in frame
void stream_set_scale_factor (
	Stream *stream, unsigned int scale_factor
) {

	if (stream) stream->scale_factor = scale_factor;

}

// sets the stream's parameters to be used to
// calculate movement & determine when an actions starts & ends
void stream_set_movement_values (
	Stream *stream,
	unsigned int movement_thresh, unsigned int max_no_movement_frames
) {

	if (stream) {
		stream->movement_thresh = movement_thresh;
		stream->max_no_movement_frames = max_no_movement_frames;
	}

}

void stream_set_movement_thresh (
	Stream *stream, unsigned int movement_thresh
) {

	if (stream) {
		stream->movement_thresh = movement_thresh;
	}

}

void stream_set_max_no_movement_frames (
	Stream *stream, unsigned int max_no_movement_frames
) {

	if (stream) {
		stream->max_no_movement_frames = max_no_movement_frames;
	}

}

// sets the stream's type
// can only be called when a new stream is created
void stream_set_type (
	Stream *stream, StreamType type
) {
	
	if (stream) stream->type = type;

}

// scale raw frame (into a new one) to this size to be used as pose input
int stream_set_pose_size (
	Stream *stream, int width, int height
) {

	int retval = 1;

	if (stream) {
		stream->pose_size.width = width;
		stream->pose_size.height = height;

		retval = 0;
	}

	return retval;

}

// sets the size of the preferred size of the rectangle to use for local frame resolution
// to create the hands frames based on hand coords from pose in hand_frame_create ()
int stream_set_pose_ouput_size (
	Stream *stream, int width, int height
) {

	int retval = 1;

	if (stream) {
		stream->pose_output_width = width;
		stream->pose_output_height = height;

		retval = 0;
	}

	return retval;

}

// sets the offset to be used when recreating frames
// for services using the pose output coords
void stream_set_pose_output_offset (
	Stream *stream, int x_offset, int y_offset
) {

	if (stream) {
		stream->pose_output_x_offset = x_offset;
		stream->pose_output_y_offset = y_offset;
	}

}

// sets the stream's values now that we have all the required info
// called automatically in store_stream_thread () after the camera has been opened
int stream_populate_values (Stream *stream) {

	int retval = 1;

	if (stream) {
		stream->width = stream->cam->real_width;
		stream->height = stream->cam->real_height;

		stream->pose_width_scale = stream->cam->real_width / stream->pose_size.width;
		stream->pose_height_scale = stream->cam->real_height / stream->pose_size.height;
	}

	return retval;

}

// creates and sets a new video writer to save a video
// returns 0 on success, 1 on error
unsigned int stream_set_video_writer (
	Stream *stream
) {

	unsigned int retval = 1;

	if (stream) {
		if (!stream->writer) {
			(void) snprintf (
				stream->video_output,
				STREAM_VIDEO_OUTPUT_FILENAME_SIZE - 1,
				"%s/%s/%ld.avi",
				global->config.output_path,
				stream->name,
				time (NULL)
			);

			// configured to output at pose size resolution
			stream->writer = new cv::VideoWriter (
				stream->video_output,
				cv::VideoWriter::fourcc ('M','J','P','G'),
				stream->cam->real_fps,
				stream->pose_size
			);

			if (stream->writer) {
				if (stream->writer->isOpened ()) {
					client_log_debug (
						"Opened stream's %u video writer %s file!",
						stream->id,
						stream->video_output
					);

					retval = 0;
				}
			}
		}
	}

	return retval;

}

// ends the current stream's video writer recording
// returns 0 on success, 1 on error
unsigned int stream_close_video_writer (
	Stream *stream
) {

	unsigned int retval = 1;

	if (stream) {
		if (stream->writer) {
			stream->writer->release ();
			delete (stream->writer);

			stream->writer = NULL;

			client_log_success (
				"Closed stream's %u video writer %s file!",
				stream->id,
				stream->video_output
			);

			retval = 0;
		}
	}

	return retval;

}

static Stream *stream_create_internal (void) {

	Stream *stream = stream_new ();
	if (stream) {
		stream->frames_buffer = job_queue_create (JOB_QUEUE_TYPE_JOBS);
	}

	return stream;

}

// stream's id is set when it is registered to a store
Stream *stream_create (StreamType type, Camera *cam) {

	Stream *stream = NULL;

	if (cam) {
		stream = stream_create_internal ();
		if (stream) {
			// stream->id = id;
			stream->type = type;

			stream->cam = cam;
		}
	}

	return stream;

}

Stream *stream_create (Store *store, u32 id, Camera *cam) {

	Stream *stream = NULL;

	if (store && cam) {
		stream = stream_create_internal ();
		if (stream) {
			stream->store = store;

			stream->id = id;
			stream->cam = cam;
		}
	}

	return stream;

}

void stream_print (const Stream *stream) {

	if (stream) {
		(void) printf ("Stream %u\n", stream->id);

		(void) printf ("\tType: %s\n", stream_type_to_string (stream->type));

		(void) printf ("\tCam: \n");
		camera_print (stream->cam);

		(void) printf ("\tWidth: %u\n", stream->width);
		(void) printf ("\tHeight: %u\n", stream->height);

		(void) printf ("\tPose size width: %d\n", stream->pose_size.width);
		(void) printf ("\tPose size height: %d\n", stream->pose_size.height);

		(void) printf ("\tPose width scale: %.4f\n", (double) stream->pose_width_scale);
		(void) printf ("\tPose height scale: %.4f\n", (double) stream->pose_height_scale);
		(void) printf ("\tPose output width: %.4f\n", (double) stream->pose_output_width);
		(void) printf ("\tPose output height: %.4f\n", (double) stream->pose_output_height);

		(void) printf ("\tPose output x offset: %d\n", stream->pose_output_x_offset);
		(void) printf ("\tPose output y offset: %d\n", stream->pose_output_y_offset);
	}

}


#pragma endregion

#pragma region thread

static unsigned int stream_open_camera (Stream *stream) {

	unsigned int retval = 1;

	// open the camera
	if (camera_open (stream->cam)) {
		switch (stream->cam->type) {
			case CAMERA_TYPE_MEDIA: {
				client_log_success (
					"Opened device %d stream!",
					stream->cam->device_idx
				);
			} break;
			case CAMERA_TYPE_IP: {
				client_log_success (
					"Opened device %s stream!",
					stream->cam->address->str
				);
			} break;

			default: break;
		}

		retval = 0;
	}

	else {
		switch (stream->cam->type) {
			case CAMERA_TYPE_MEDIA: {
				client_log_error (
					"Failed to open device %d stream!",
					stream->cam->device_idx
				);
			} break;
			case CAMERA_TYPE_IP: {
				client_log_error (
					"Failed to open device %s stream!",
					stream->cam->address->str
				);
			} break;

			default: break;
		}
	}

	return retval;

}

static unsigned int stream_open_internal (Stream *stream) {

	unsigned int retval = 1;

	// sets the stream's values now that we have all the required info
	stream_populate_values (stream);

	retval = 0;

	return retval;

}

// opens the stream's camera (actual device) & gets all remaining values
// returns 0 on success, 1 on error
unsigned int stream_open (Stream *stream) {

	unsigned int retval = 1;

	if (stream) {
		switch (stream->cam->type) {
			case CAMERA_TYPE_MEDIA: {
				if (strlen (stream->cam->device_name)) {
					client_log_debug (
						"Opening stream: %s",
						stream->cam->device_name
					);
				}

				else {
					client_log_debug (
						"Opening stream: %d",
						stream->cam->device_idx
					);
				}
			} break;
			case CAMERA_TYPE_IP: {
				client_log_debug (
					"Opening stream: %s",
					stream->cam->address->str
				);
			} break;

			default: break;
		}

		if (stream->cam->type != CAMERA_TYPE_VIDEO) {
			if (!stream_open_camera (stream)) {
				retval = stream_open_internal (stream);
			}
		}

		else {
			retval = stream_open_internal (stream);
		}
	}

	return retval;

}

static u8 stream_thread_handle_frame (
	Stream *stream, PixzoFrame *pixzo_frame
) {

	u8 retval = 1;

	(void) strncpy (
		pixzo_frame->info.store_id,
		stream->store->store_id,
		STORE_ID_SIZE - 1
	);

	pixzo_frame->info.stream_id = stream->id;
	(void) time (&pixzo_frame->info.timestamp);

	pixzo_frame->info.width = stream->cam->real_width;
	pixzo_frame->info.height = stream->cam->real_height;

	stream->n_frames_good += 1;

	if (global->connected || global->config.record) {
		// create a scaled version of the frame
		// resize raw frame to correct size to be used as pose input
		cv::Mat pose_frame;
		cv::resize (*pixzo_frame->frame, pose_frame, stream->pose_size);

		if (global->type == PIXZO_GLOBAL_TYPE_VIDEOS) {
			cv::imshow ("video", pose_frame);
		}

		// save frame to current video
		if (global->config.record) {
			stream->writer->write (pose_frame);
		}
	}

	retval = 0;

	return retval;

}

static unsigned int stream_thread_check_movement (
	cv::Mat frame
) {

	unsigned int contador = 0;
	uchar *data = (uchar *) frame.data;

	// client_log_line_break ();
	// client_log_debug (
	// 	"Frame rows x cols: %d",
	// 	frame.rows * frame.cols
	// );
	// client_log_line_break ();

	register int x = 0;
	register int y = 0;
	for (; y < frame.rows; y++) {
		for (x = 0; x < frame.cols; x++) {
			if (*data) {
				contador++;
			}

			data++;
		}
	}

	return contador;

}

static void stream_thread_handle_movement (
	Stream *stream, PixzoFrame *pixzo_frame
) {

	if (!stream->movement) {
		if (stream->movement_count >= stream->movement_thresh) {
			client_log_success ("First movement...");

			if (global->config.record) {
				if (stream_set_video_writer (stream)) {
					client_log_error (
						"Failed to open stream's %d new video writer!",
						stream->id
					);
				}
			}

			(void) stream_thread_handle_frame (
				stream, pixzo_frame
			);
			
			stream->no_movement_frames = 0;
			stream->movement = true;
		}
	}

	else {
		(void) stream_thread_handle_frame (
			stream, pixzo_frame
		);

		// check if there is still movement
		if (stream->movement_count >= stream->movement_thresh) {
			stream->no_movement_frames = 0;
		}

		else {
			stream->no_movement_frames += 1;
			if (stream->no_movement_frames >= stream->max_no_movement_frames) {
				client_log_warning ("Max no movement frames reached!");
				stream->movement = false;

				(void) stream_thread_handle_frame (
					stream, pixzo_frame
				);

				(void) stream_close_video_writer (stream);
			}
		}
	}

}

void *stream_movement_thread (void *stream_ptr) {

	Stream *stream = (Stream *) stream_ptr;

	char thread_name[THREAD_NAME_BUFFER_SIZE] = { 0 };
	(void) snprintf (
		thread_name, THREAD_NAME_BUFFER_SIZE,
		"stream-movement-%u", stream->id
	);

	(void) thread_set_name (thread_name);
	client_log_success ("%s THREAD has started!", thread_name);

	int scaled_width = (int) (stream->cam->real_width / stream->scale_factor);
	int scaled_height = (int) (stream->cam->real_height / stream->scale_factor);

	#ifdef PIXZO_DEBUG
	client_log_debug ("Scaled width: %d", scaled_width);
	client_log_debug ("Scaled height: %d", scaled_height);
	#endif

	cv::Mat previous_frame_gray (scaled_height, scaled_width, CV_8U, cv::Scalar (0));
	cv::Mat gray_scale; // (scaled_height, scaled_with, CV_8U, Scalar (0));
	cv::Mat diference (scaled_height, scaled_width, CV_8U, cv::Scalar (0));

	cv::Mat resized;

	Job *job = NULL;
	PixzoFrame *pixzo_frame = NULL;
	while (stream->store->active) {
		bsem_wait (stream->frames_buffer->has_jobs);

		if (stream->store->active) {
			job = (Job *) job_queue_pull (stream->frames_buffer);
			if (job) {
				pixzo_frame = (PixzoFrame *) job->args;

				// check for movement in frame
				cv::resize (*pixzo_frame->frame, resized, cv::Size (scaled_width, scaled_height));
				cv::cvtColor (resized, gray_scale, cv::COLOR_BGR2GRAY);
				// fastNlMeansDenoising (gray_scale, gray_scale, 3.0, 3, 3);
				cv::absdiff (gray_scale, previous_frame_gray, diference);
				(void) cv::threshold (diference, diference, 45, 255, cv::THRESH_BINARY);

				stream->movement_count = stream_thread_check_movement (diference);

				#ifdef STREAM_DEBUG
				client_log_debug ("Movement: %u", stream->movement_count);
				#endif

				stream_thread_handle_movement (
					stream, pixzo_frame
				);

				if (!stream->movement) {
					pixzo_frame_delete (pixzo_frame);
				}

				previous_frame_gray = gray_scale.clone ();

				job_return (stream->frames_buffer, job);
			}
		}
	}

	client_log_success ("%s has exited!", thread_name);

	return NULL;

}

void *stream_record_thread (void *stream_ptr) {

	Stream *stream = (Stream *) stream_ptr;

	char thread_name[THREAD_NAME_BUFFER_SIZE] = { 0 };
	(void) snprintf (
		thread_name, THREAD_NAME_BUFFER_SIZE,
		"stream-record-%u", stream->id
	);

	(void) thread_set_name (thread_name);
	client_log_success ("%s THREAD has started!", thread_name);

	// create video file
	if (stream_set_video_writer (stream)) {
		client_log_error (
			"Failed to open stream's %d new video writer!",
			stream->id
		);
	}

	Job *job = NULL;
	PixzoFrame *pixzo_frame = NULL;
	while (stream->store->active) {
		bsem_wait (stream->frames_buffer->has_jobs);

		if (stream->store->active) {
			job = (Job *) job_queue_pull (stream->frames_buffer);
			if (job) {
				pixzo_frame = (PixzoFrame *) job->args;

				// create a scaled version of the frame
				// resize raw frame to correct size to be used as pose input
				cv::Mat pose_frame;
				cv::resize (*pixzo_frame->frame, pose_frame, stream->pose_size);

				// save frame to current video
				stream->writer->write (pose_frame);

				pixzo_frame_delete (pixzo_frame);

				job_return (stream->frames_buffer, job);
			}
		}
	}

	// close video file
	(void) stream_close_video_writer (stream);

	client_log_success ("%s has exited!", thread_name);

	return NULL;

}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

// dedicated thread for each stream to read from its camera
void *stream_thread (void *stream_ptr) {

	Stream *stream = (Stream *) stream_ptr;

	char thread_name[THREAD_NAME_BUFFER_SIZE] = { 0 };
	(void) snprintf (
		thread_name, THREAD_NAME_BUFFER_SIZE,
		"stream-%u", stream->id
	);

	(void) thread_set_name (thread_name);
	client_log_success ("%s THREAD has started!", thread_name);

	// reset stream values
	stream->movement = false;
	stream->movement_count = 0;

	int64_t fps = 0;
	int64_t delta_ticks = 0;
	struct timespec start = { 0 };
	struct timespec end = { 0 };

	PixzoFrame *pixzo_frame = NULL;
	while (stream->store->active) {
		(void) clock_gettime (CLOCK_MONOTONIC_RAW, &start);

		// get new frame from device
		pixzo_frame = pixzo_frame_get ();
		if (pixzo_frame) {
			pixzo_frame->info.frame_id = stream->next_frame_id;

			if (!camera_get (stream->cam, pixzo_frame->frame)) {
				stream->n_frames_read += 1;
				stream->next_frame_id += 1;

				if (!pixzo_frame->frame->empty ()) {
					// push to frames buffer
					(void) job_queue_push (
						stream->frames_buffer,
						job_create (NULL, pixzo_frame)
					);
				}

				else {
					#ifdef PIXZO_DEBUG
					client_log_error (
						"Pixzo frame %lu is empty in stream %d",
						pixzo_frame->info.frame_id, stream->id
					);
					#endif

					pixzo_frame_delete (pixzo_frame);
				}
			}

			else {
				client_log_error (
					"Failed to get frame from stream %d",
					stream->id
				);

				pixzo_frame_delete (pixzo_frame);
			}
		}

		if (global->config.enable_wait_key) {
			(void) cvWaitKey (global->config.wait_key_delay);
		}

		// count fps
		(void) clock_gettime (CLOCK_MONOTONIC_RAW, &end);
		delta_ticks += labs ((end.tv_nsec - start.tv_nsec) / 1000000);
		fps++;

		if (delta_ticks >= 1000) {
			(void) printf ("fps: %ld\n", fps);
			delta_ticks = 0;
			fps = 0;
		}
	}

	// correctly close any on going video writer
	(void) stream_close_video_writer (stream);

	switch (stream->cam->type) {
		case CAMERA_TYPE_MEDIA: {
			client_log_success (
				"Stream from: %d has ended!",
				stream->cam->device_idx
			);
		} break;
		case CAMERA_TYPE_IP: {
			client_log_success (
				"Stream from: %s has ended!",
				stream->cam->address->str
			);
		} break;

		default: break;
	}

	client_log_success ("%s has exited!", thread_name);

	return NULL;

}

#pragma GCC diagnostic pop

static u8 stream_videos_thread_internal_get_frame (
	Stream *stream, PixzoFrame *pixzo_frame
) {

	u8 retval = 1;

	if (stream->cam->capture->read (*pixzo_frame->frame)) {
		stream->n_frames_read += 1;
		stream->next_frame_id += 1;

		if (!pixzo_frame->frame->empty ()) {
			retval = stream_thread_handle_frame (
				stream, pixzo_frame
			);
		}

		else {
			client_log_warning ("stream_videos_thread () - empty frame");
		}
	}

	else {
		client_log_warning ("stream_videos_thread () - failed to read frame");
	}

	return retval;

}

static void stream_videos_thread_internal (
	Stream *stream, const String *filename
) {

	if (camera_open (stream->cam, filename)) {
		PixzoFrame *pixzo_frame = NULL;

		// get next frame unti the end of the video
		while (1) {
			pixzo_frame = pixzo_frame_get ();
			if (pixzo_frame) {
				pixzo_frame->info.frame_id = stream->next_frame_id;

				if (stream_videos_thread_internal_get_frame (
					stream, pixzo_frame
				)) {
					// we have reached the end of the video
					client_log_debug (
						"Video %s in stream %d has ended!\n\n",
						filename->str, stream->id
					);

					camera_close (stream->cam);

					pixzo_frame_delete (pixzo_frame);

					break;
				}
			}

			if (global->config.enable_wait_key) {
				(void) cv::waitKey (global->config.wait_key_delay);
			}
		}
	}

	else {
		client_log_error (
			"Failed to open %s in stream %d",
			filename->str, stream->id
		);
	}

}

void *stream_videos_thread (void *stream_ptr) {

	Stream *stream = (Stream *) stream_ptr;

	char thread_name[THREAD_NAME_BUFFER_SIZE] = { 0 };
	(void) snprintf (
		thread_name, THREAD_NAME_BUFFER_SIZE,
		"stream-%u", stream->id
	);

	(void) thread_set_name (thread_name);
	client_log_success ("%s THREAD has started!", thread_name);

	String *filename = NULL;
	for (unsigned int i = 0; i < global->config.videos_n_loops; i++) {
		for (ListElement *le = dlist_start (stream->videos); le; le = le->next) {
			filename = (String *) le->data;

			client_log_debug ("Opening %s in stream %d", filename->str, stream->id);

			stream_videos_thread_internal (
				stream, filename
			);
		}
	}

	client_log_success ("%s has exited!", thread_name);

	return NULL;

}

#pragma endregion
