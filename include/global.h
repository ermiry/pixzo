#ifndef _PIXZO_GLOBAL_H_
#define _PIXZO_GLOBAL_H_

#include <stdbool.h>

#include <pthread.h>

#include <client/client.h>

#include "config.h"
#include "store.h"

#define GLOBAL_TYPE_MAP(XX)				\
	XX(0,	NONE, 		None)			\
	XX(1,	SINGLE, 	Single)			\
	XX(2,	VIDEOS, 	Videos)			\
	XX(3,	LOAD, 		Load)			\
	XX(4,	REGISTER, 	Register)		\
	XX(5,	RECORD, 	Record)			\
	XX(6,	TEST, 		Test)

typedef enum GlobalType {

	#define XX(num, name, string) PIXZO_GLOBAL_TYPE_##name = num,
	GLOBAL_TYPE_MAP (XX)
	#undef XX

} GlobalType;

extern const char *global_type_to_string (const GlobalType type);

extern GlobalType global_type_from_string (const char *type);

#define GLOBAL_STATUS_MAP(XX)													\
	XX(0,	NONE, 			None, 			None)								\
	XX(1,	CONNECTED, 		Connected, 		Connected to main cerver)			\
	XX(2,	READY, 			Ready, 			Connected and ready to start)		\
	XX(3,	IDLE, 			Idle, 			Not working)						\
	XX(4,	WORK, 			Work, 			Currently wokring)					\
	XX(5,	DISCONNECTED, 	Disconnected, 	Not connected to the main cerver)	\
	XX(6,	ENDING, 		Ending, 		Ending local processes)				\
	XX(10,	ERROR, 			Error,			A critical error has ocurred)

typedef enum GlobalStatus {

	#define XX(num, name, string, description) PIXZO_GLOBAL_STATUS_##name = num,
	GLOBAL_STATUS_MAP (XX)
	#undef XX

} GlobalStatus;

extern const char *global_status_to_string (
	const GlobalStatus status
);

extern const char *global_status_description (
	const GlobalStatus status
);

struct _Global {

	GlobalType type;
	GlobalStatus status;

	Config config;

	Client *client;
	Connection *connection;

	bool connected;

	Store *store;

	pthread_mutex_t *mutex;

};

typedef struct _Global Global;

extern Global *global;

extern unsigned int global_init (int argc, char **argv);

extern void global_end (void);

extern void global_set_status (const GlobalStatus status);

extern GlobalStatus global_get_status (void);

#endif