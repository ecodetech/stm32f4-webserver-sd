/*
 * Webserver.h
 *
 *  Created on: Jul 16, 2022
 *      Author: mj
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fs.h"
#include <stdbool.h>

/* HTTP POST  or GET handler function */
typedef void (*api_handler_t)	(const char* , struct fs_file *post_resp_file);


api_handler_t Get_Api_Uri_Handler(const char* request_uri);	// api handler type for requests

#ifdef __cplusplus
}
#endif

#endif /* WEBSERVER_H_ */
