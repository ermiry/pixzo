#ifndef _PIXZO_STORE_H_
#define _PIXZO_STORE_H_

#include <stdbool.h>

#include <pthread.h>

#include <client/types/types.h>
#include <client/types/string.h>

#include <client/collections/dlist.h>

#define STORE_ID_SIZE				32
#define STORE_NAME_SIZE				1024
#define STORE_LOCATION_SIZE			1024

struct _Camera;
struct _Stream;

#define STORE_STATUS_MAP(XX)			\
	XX(0,	NONE, 		None)			\
	XX(1,	OPEN, 		Open)			\
	XX(2,	CLOSED, 	Closed)			\
	XX(3,	INNACTIVE, 	Innactive)

typedef enum StoreStatus {

    #define XX(num, name, string) STORE_STATUS_##name = num,
	STORE_STATUS_MAP (XX)
	#undef XX

} StoreStatus;

extern const char *store_status_to_string (StoreStatus status);

struct _Store {

	char store_id[STORE_ID_SIZE];
    char name[STORE_NAME_SIZE];
	char location[STORE_LOCATION_SIZE];

    StoreStatus status;

	bool active;

	// our camera streams
	u32 next_stream_id;
	DoubleList *streams;
	
    pthread_mutex_t *mutex;

};

typedef struct _Store Store;

extern void store_delete (void *store_ptr);

extern Store *store_create ();

// gets the current store's status
extern StoreStatus store_status_get (Store *store);

// set the store's status in the db
// returns 0 on succes, 1 on error
extern u8 store_status_set (Store *store, StoreStatus status);

// sets the store's values
extern void store_set_values (
	Store *store,
	const char *id, const char *name, const char *location
);

extern void store_print (Store *store);

#pragma region cameras

// registers a new camera to the store
// returns 0 on success, 1 on error
extern u8 store_register_camera (
	Store *store, struct _Camera *cam
);

// registers a new camera to the store (media device)
// returns 0 on success, 1 on error
extern u8 store_register_camera_by_idx (
	Store *store, u8 device_idx
);

// registers a new camera to the store (IP camera)
// returns 0 on success, 1 on error
extern u8 store_register_camera_by_address (
	Store *store, const char *address
);

// unregisters a camera (stream) from the store using the stream id
extern u8 store_unregister_camera (
	Store *store, struct _Stream *stream
);

#pragma endregion

#pragma region streams

// registers a new stream to the store
// returns 0 on succes, 1 on error
extern u8 store_stream_register (
	Store *store, struct _Stream *stream
);

// get the correct store stream by id
extern struct _Stream *store_stream_get_by_id (
	Store *store, u32 id
);

#pragma endregion

#pragma region main

// opens the streams to get all of their values so we can send them to the main cerver
// returns 0 on success, 1 on any error
extern unsigned int store_open (Store *store);

// starts the store's camera streams in a dedicated thread for each one
// returns 0 on success, 1 on any error
extern unsigned int store_start (void *store_ptr);

// correctly closes the store
extern void store_close (Store *store);

#pragma endregion

#endif