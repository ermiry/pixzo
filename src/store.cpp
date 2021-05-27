#include <stdlib.h>
#include <string.h>

#include <string>

#include <opencv2/imgproc.hpp>					// for resize
#include <opencv2/highgui/highgui.hpp>			// used for cv:cvWaitKey ();

#include <client/types/types.h>
#include <client/collections/dlist.h>

#include <client/threads/thread.h>

#include <client/utils/utils.h>
#include <client/utils/log.h>

#include "global.h"
#include "frames.hpp"

#include "store.h"
#include "camera.hpp"
#include "stream.hpp"

const char *store_status_to_string (StoreStatus status) {

	switch (status) {
		#define XX(num, name, string) case STORE_STATUS_##name: return #string;
		STORE_STATUS_MAP(XX)
		#undef XX
	}

	return store_status_to_string (STORE_STATUS_NONE);

}

#pragma region store

static Store *store_new (void) {

	Store *store = (Store *) malloc (sizeof (Store));
	if (store) {
		(void) memset (store, 0, sizeof (Store));

        store->status = STORE_STATUS_NONE;

        store->active = false;

        store->next_stream_id = 0;
		store->streams = NULL;

        store->mutex = NULL;
	}

	return store;

}

void store_delete (void *store_ptr) {

	if (store_ptr) {
		Store *store = (Store *) store_ptr;

        (void) pthread_mutex_lock (store->mutex);

		dlist_delete (store->streams);

        (void) pthread_mutex_unlock (store->mutex);
		(void) pthread_mutex_destroy (store->mutex);
		free (store->mutex);

		free (store);
	}

}

Store *store_create (void) {

	Store *store = store_new ();
    if (store) {
        store->streams = dlist_init (stream_delete, stream_comparator_by_id);

        store->mutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
		(void) pthread_mutex_init (store->mutex, NULL);
    }

	return store;

}

// gets the current store's status
StoreStatus store_status_get (Store *store) {

	StoreStatus retval = STORE_STATUS_NONE;

	if (store) {
		(void) pthread_mutex_lock (store->mutex);
		retval = store->status;
		(void) pthread_mutex_unlock (store->mutex);
	}

	return retval;

}

// set the store's status in the db
// returns 0 on succes, 1 on error
u8 store_status_set (Store *store, StoreStatus status) {

	u8 retval = 1;

    if (store) {
		(void) pthread_mutex_lock (store->mutex);
        store->status = status;
		(void) pthread_mutex_unlock (store->mutex);

		retval = 0;
    }

	return retval;

}

// sets the store's values
void store_set_values (
	Store *store,
	const char *id, const char *name, const char *location
) {

	if (store) {
		(void) pthread_mutex_lock (store->mutex);

		if (id) (void) strncpy (store->store_id, id, STORE_ID_SIZE - 1);
		if (name) (void) strncpy (store->name, name, STORE_NAME_SIZE - 1);
		if (location) (void) strncpy (store->location, location, STORE_LOCATION_SIZE - 1);
		
		(void) pthread_mutex_unlock (store->mutex);
	}

}

void store_print (Store *store) {

	if (store) {
		(void) pthread_mutex_lock (store->mutex);

		(void) printf ("\n");
		(void) printf ("Store: %s\n", store->name);
		(void) printf ("ID: %s\n", store->store_id);
		(void) printf ("Location: %s\n", store->location);
		(void) printf ("N streams: %lu\n", store->streams->size);

		for (ListElement *le = dlist_start (store->streams); le; le = le->next) {
			stream_print ((Stream *) le->data);
		}

		(void) printf ("\n");

		(void) pthread_mutex_unlock (store->mutex);
	}

}

#pragma endregion

#pragma region cameras

// registers a new camera to the store
// returns 0 on success, 1 on error
u8 store_register_camera (Store *store, Camera *cam) {

	u8 retval = 1;

	if (store && cam) {
		Stream *stream = stream_create (store, store->next_stream_id, cam);
		if (stream) {
			store->next_stream_id += 1;
			retval = dlist_insert_after (store->streams, dlist_end (store->streams), stream);
		}
	}

	return retval;

}

// registers a new camera to the store (media device)
// returns 0 on success, 1 on error
u8 store_register_camera_by_idx (Store *store, u8 device_idx) {

	u8 retval = 1;

	if (store) {
		Camera *cam = camera_create (device_idx);
		if (cam) {
			retval = store_register_camera (store, cam);
		}
	}

	return retval;

}

// registers a new camera to the store (IP camera)
// returns 0 on success, 1 on error
u8 store_register_camera_by_address (Store *store, const char *address) {

	u8 retval = 1;

	if (store) {
		Camera *cam = camera_create (address);
		if (cam) {
			retval = store_register_camera (store, cam);
		}
	}

	return retval;

}

// unregisters a camera (stream) from the store using the stream id
u8 store_unregister_camera (Store *store, Stream *stream) {

	u8 retval = 1;

	if (store && stream) {
		void *data = dlist_remove (store->streams, stream, NULL);
		if (data) {
			stream_delete (data);
			retval = 0;
		}
	}

	return retval;

}

#pragma endregion

#pragma region streams

// registers a new stream to the store
// returns 0 on succes, 1 on error
u8 store_stream_register (Store *store, Stream *stream) {

	u8 retval = 1;

	if (store && stream) {
		// stream->id = store->next_stream_id;
		// store->next_stream_id += 1;

		stream->store = store;

		retval = dlist_insert_after (
			store->streams, dlist_end (store->streams), stream
		);
	}

	return retval;

}

// get the correct store stream by id
Stream *store_stream_get_by_id (Store *store, u32 id) {

	Stream *retval = NULL;

	if (store) {
		Stream *query = stream_new ();
		if (query) {
			query->id = id;
			retval = (Stream *) dlist_search (store->streams, query, NULL);
			stream_delete (query);
		}
	}

	return retval;

}

#pragma endregion

#pragma region main

// opens the streams to get all of their values so we can send them to the main cerver
// returns 0 on success, 1 on any error
unsigned int store_open (Store *store) {

	unsigned int errors = 0;

	if (store) {
		client_log_debug ("Opening store %s ...", store->store_id);

		for (ListElement *le = dlist_start (store->streams); le; le = le->next) {
			errors |= stream_open ((Stream *) le->data);
		}
	}

	return errors;

}

// starts the store's camera streams in a dedicated thread for each one
// returns 0 on success, 1 on any error
unsigned int store_start (void *store_ptr) {

	unsigned int errors = 0;

	if (store_ptr) {
		Store *store = (Store *) store_ptr;

		client_log_debug ("Starting store %s ...", store->name);

		store->active = true;

		void *(*stream_thread_work) (void *) = NULL;
		switch (global->type) {
			case PIXZO_GLOBAL_TYPE_SINGLE: {
				stream_thread_work = stream_thread;
			} break;

			case PIXZO_GLOBAL_TYPE_VIDEOS: {
				stream_thread_work = stream_videos_thread;
			} break;

			default: break;
		}

		// create a dedicated thread for each stream (camera)
		Stream *stream = NULL;
		for (ListElement *le = dlist_start (store->streams); le; le = le->next) {
			stream = (Stream *) le->data;

			// TODO: move logic to stream thread
			if (thread_create_detachable (
				&stream->stream_thread_id, 
				stream_thread_work, 
				stream
			)) {
				client_log_error (
					"store_start () - "
					"failed to create stream's %d MAIN thread",
					stream->id
				);

				errors |= 1;
			}

			// create stream's custom thread
			switch (global->type) {
				case PIXZO_GLOBAL_TYPE_SINGLE: {
					if (thread_create_detachable (
						&stream->movement_thread_id, 
						stream_movement_thread,
						stream
					)) {
						client_log_error (
							"store_start () - "
							"failed to create stream's %d MOVEMENT thread!",
							stream->id
						);

						errors |= 1;
					}
				} break;

				case PIXZO_GLOBAL_TYPE_RECORD: {
					if (thread_create_detachable (
						&stream->record_thread_id,
						stream_record_thread,
						stream
					)) {
						client_log_error (
							"store_start () - "
							"failed to create stream's %d RECORD thread!",
							stream->id
						);

						errors |= 1;
					}
				} break;

				default: break;
			}
		}

		// set the store status in the db
		errors |= store_status_set (store, STORE_STATUS_OPEN);

		client_log_success ("Store %s has started!", store->name);
	}

	return errors;

}

// correctly closes the store
void store_close (Store *store) {

	if (store) {
		// set the store status in the db
		store_status_set (store, STORE_STATUS_CLOSED);

		// streams threads are created as detachable that listen to this condition
		store->active = false;

		client_log_success ("Store %s has been closed!", store->name);
	}

}

#pragma endregion
