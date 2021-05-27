#ifndef _PIXZO_ERRORS_H_
#define _PIXZO_ERRORS_H_

typedef enum PixzoError {

	PIXZO_ERROR_NONE						= 0,

	PIXZO_ERROR_TEST                   	 	= 1,

	PIXZO_ERROR_STORE_REGISTER          	= 2,
	PIXZO_ERROR_STORE_UNREGISTER       		= 3,

	PIXZO_ERROR_STORE_START					= 4,
	PIXZO_ERROR_STORE_CLOSE					= 5

} PixzoError;

#endif