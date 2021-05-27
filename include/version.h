#ifndef _PIXZO_VERSION_H_
#define _PIXZO_VERSION_H_

#define PIXZO_VERSION					"0.1"
#define PIXZO_VERSION_NAME				"Version 0.1"
#define PIXZO_VERSION_DATE				"26/05/2021"
#define PIXZO_VERSION_TIME				"23:42 CST"
#define PIXZO_VERSION_AUTHOR			"Erick Salas"

// print full pixzo version information 
extern void pixzo_version_print_full (void);

// print the version id
extern void pixzo_version_print_version_id (void);

// print the version name
extern void pixzo_version_print_version_name (void);

#endif