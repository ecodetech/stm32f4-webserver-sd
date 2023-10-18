/*
 * httpd_post.c
 *
 *  Created on: 14-Jul-2022
 *  Author: Manoj Jadhao (Electrocode Technologies)
 *  Website: https://www.electrocode.in
 */

#include "httpd_post.h"

#include "main.h"
#include "httpd.h"
#include "usart.h"
#include "fatfs.h"
#include "fs.h"
#include "cmsis_os.h"
#include <string.h>
#include "webserver.h"

#define LWIP_HTTPD_POST_MAX_PAYLOAD_LEN	1024

static char *request_json = NULL;
static int http_post_content_len = 0;
static u16_t http_post_payload_len = 0;

static const char error_post_data_exceed[] =
		"{\"status\":\"Data Exceed Error!\"}";

static const char error_post_req_not_impld[] =
		"{\"status\":\"Request not implemented!\"}";

static void *current_connection;

struct fs_file post_resp_file;
//static const char handle_request_uri[] = "/request";
static const char post_response_uri[] = "/response.json";
static api_handler_t api_handler;

void Send_Uart(const char *string) {
	HAL_UART_Transmit(&huart1, (uint8_t*) string, strlen(string),
	HAL_MAX_DELAY);
}

err_t httpd_post_begin(void *connection, const char *uri,
		const char *http_request, u16_t http_request_len, int content_len,
		char *response_uri, u16_t response_uri_len, u8_t *post_auto_wnd) {
	//LWIP_UNUSED_ARG(connection);
	//LWIP_UNUSED_ARG(http_request);
	LWIP_UNUSED_ARG(http_request_len);
	//LWIP_UNUSED_ARG(content_len);
	LWIP_UNUSED_ARG(post_auto_wnd);
	Send_Uart("\r\nURI: ");
	Send_Uart(uri);
	Send_Uart("\r\nhttp_request: ");
	Send_Uart(http_request);
	if (content_len > LWIP_HTTPD_POST_MAX_PAYLOAD_LEN) {

		post_resp_file.data = error_post_data_exceed;
		post_resp_file.index = 0;
		post_resp_file.len = strlen(error_post_data_exceed);
		snprintf(response_uri, response_uri_len, post_response_uri);
		return ERR_VAL;
	}
	//	if (strcmp(handle_request_uri, uri) == 0) {
	//		request_json = (char *) handle_request_uri;
	//		current_connection = connection;
	//
	//		request_json = (char*)malloc(content_len * sizeof(char));
	//		memset(request_json, 0, content_len * sizeof(char));
	//		return ERR_OK;
	//	}
	api_handler = Get_Api_Uri_Handler(uri);
	if (api_handler != NULL) {
		current_connection = connection;
		if (request_json != NULL) {
			free(request_json);
			request_json = NULL;
		}
		request_json = (char*) malloc(content_len + 1);
		memset(request_json, 0, content_len + 1);
		http_post_content_len = content_len;
		http_post_payload_len = 0;
		return ERR_OK;
	}
	post_resp_file.data = error_post_req_not_impld;
	post_resp_file.index = 0;
	post_resp_file.len = strlen(error_post_req_not_impld);
	snprintf(response_uri, response_uri_len, post_response_uri);
	return ERR_VAL;
}

void httpd_post_finished(void *connection, char *response_uri,
		u16_t response_uri_len) {
	if (current_connection == connection) {
		post_resp_file.data = NULL;

		api_handler(request_json, &post_resp_file);
		Send_Uart("\r\nGot response:");
		Send_Uart(post_resp_file.data);

		Send_Uart("\r\nPOST req finished: ");
		if (strcmp(response_uri, post_response_uri) == 0) {
			snprintf(response_uri, response_uri_len, post_response_uri);
		} else {
			snprintf(response_uri, response_uri_len, post_response_uri);
		}
		current_connection = NULL;
	}
	//current_connection = NULL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
	struct pbuf *q = p;
	u32_t http_post_payload_full_flag = 0;
	if (current_connection == connection) {
		while (q != NULL) // Cache the received data to http_post_payload
		{
			if (http_post_payload_len + q->len <= http_post_content_len) {
				MEMCPY(request_json + http_post_payload_len, q->payload,
						q->len);
				http_post_payload_len += q->len;
			} else { // Buffer overflow Set overflow flag
				http_post_payload_full_flag = 1;
				break;
			}
			q = q->next;
		}

		*(request_json + http_post_payload_len) = 0;
		char tempBuf[30];
		sprintf(tempBuf, "\r\nPost req recv size:%d ", p->tot_len);
		Send_Uart(tempBuf);
		//pbuf_free(p);
		if (q != NULL && http_post_payload_full_flag == 1) {
			http_post_payload_len = 0;
			return ERR_VAL;
		}

		//	post_resp_file.data = request_json;
		//	post_resp_file.index = 0;
		//	post_resp_file.len = http_post_payload_len;

		//Send_Uart(request_json);

		//    if(post_resp_file != NULL){
		//        post_resp_file.data = NULL;
		//        free(post_resp_file);
		//        post_resp_file = NULL;
		//    }
		//    post_resp_file = (struct fs_file*) malloc(sizeof(struct fs_file));

		//	api_handler(request_json, post_resp_file);
		//	Send_Uart("\r\nGot response:");
		//	Send_Uart(post_resp_file.data);

		if (p != NULL) {
			pbuf_free(p);
		}

		return ERR_OK;
	}
	return ERR_VAL;
}

int fs_open_custom(struct fs_file *file, const char *name) {
	FRESULT fresult;
	char file_name_buff[100] = "";

	if (strcmp(name, post_response_uri) == 0) {
		//prepare response
		Send_Uart(": opening post file\r\n");
		file->data = NULL;
		file->len = post_resp_file.len;
		file->index = 0;
		file->pextension = post_response_uri;
		char tempBuf[20];
		sprintf(tempBuf, "\r\nFile Size:%d\r\n", post_resp_file.len);
		Send_Uart(tempBuf);
		return 1;
	}

	if (f_mount(&SDFatFS, "/", 1) != FR_OK) {
		Send_Uart("\r\nSD mount error");
	} else {
		Send_Uart("\r\nSD mounted");
	}
	sprintf(file_name_buff, "/html%s", name);

	Send_Uart("\r\n");
	Send_Uart(file_name_buff);
	FILINFO fileInfo;
	fresult = f_stat(file_name_buff, &fileInfo);
	if (fresult == FR_OK) {
		//Send_Uart(": opening file\r\n");
		fresult = f_open(&SDFile, file_name_buff, FA_READ);
		if (fresult == FR_OK) {
			file->data = NULL;
			file->len = fileInfo.fsize;
			file->index = 0;
			file->pextension = NULL;

			char tempBuf[20];
			sprintf(tempBuf, "\r\nFile Size:%d\r\n", file->len);
			Send_Uart(tempBuf);
			return 1;
		}
	}
	return 0;
}

void fs_close_custom(struct fs_file *file) {

	if (request_json != NULL) {
		free(request_json);
		request_json = NULL;
	}
	Send_Uart(":Closing file\r\n");

	/* Checking if file from POST response */
	if (file->pextension == post_response_uri) {
		file->pextension = NULL;
	} else if (file && file->pextension) {
		free(file->pextension);
		file->pextension = NULL;
		f_close(&SDFile);
	}

}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {
	UINT read;
	Send_Uart("\r\n reading the file\r\n");
	memset(buffer, 0, count);
	if (strcmp(file->pextension, post_response_uri) == 0) {
		//	if (post_resp_file.data != 0 ) {
		if (post_resp_file.index < post_resp_file.len) {
			char temp_buf[50];
			sprintf(temp_buf, "\r\n count=%d, index=%d\r\n", count,
					post_resp_file.index);
			Send_Uart(temp_buf);
			memcpy(buffer, post_resp_file.data + (post_resp_file.index), count);
			//Send_Uart(post_resp_file.data);
			read = count;
			post_resp_file.index += read;
		} else {
			read = FS_READ_EOF;
		}
	} else

	{
		/* if we do add this line, it add random chars to the buffer: could byte alignment issue*/
		if ((count % 4) != 0 && count > 4)
			count -= (count % 4);
		/*----------------- This could be improved --------------------------*/

		if (file) {
			char temp_buf[50];
			sprintf(temp_buf, "\r\n count=%d, index=%d\r\n", count,
					file->index);
			Send_Uart(temp_buf);
			FRESULT fresult = f_read(&SDFile, buffer, count, (UINT*) &read); /* Read a chunk of source file */
			//Send_Uart(buffer);
			if (fresult != FR_OK) {
				char bytes[10] = "";
				Send_Uart("\r\n error reading the file\r\n");
				Send_Uart(itoa(fresult, bytes, 10));
				read = FS_READ_EOF;
			}
			file->index += count;
		} else {
			Send_Uart("\r\n File exist/open error \r\n");
			read = FS_READ_EOF;
		}
	}
	return read;
}

//int
//fs_read_async_custom(struct fs_file *file, char *buffer, int count, fs_wait_cb callback_fn, void *callback_arg)
//{
//  LWIP_ASSERT("not implemented in this example configuration", 0);
//  LWIP_UNUSED_ARG(file);
//  LWIP_UNUSED_ARG(buffer);
//  LWIP_UNUSED_ARG(count);
//  LWIP_UNUSED_ARG(callback_fn);
//  LWIP_UNUSED_ARG(callback_arg);
//  /* Return
//     - FS_READ_EOF if all bytes have been read
//     - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
//  /* all bytes read already */
//  return FS_READ_EOF;
//}

