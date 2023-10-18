# STM32F4 Ethernet Webserver using SD card

## Board Used for test:
![Development Board](Board%20Details/front%20pins.jpg)
#### Micro-controller: STM32F407VETx

## Ethernet module used:
![Ethernet module](Board%20Details/lan8720-module.png)
### I hope it works with other modules like DP83848, although it wasn't success for me though.

## SD Card used:
1. File System: Fat32
2. Size: 2GB

> Note: For more details on hardware go through Folder: [Board Details](Board%20Details)


## Software:
1. STM32Cube IDE 1.7.0
2. STM32F407 Firmware Package: 1.27.2

## Description:

    This project aims to showcase a simple Webserver using STM32F4 and LAN8720 ethernet module.
    * It should be able to access files on SD card using HTTP-GET method at location "/html" on SD card.
    This should also demonstrate how the HTTP-POST request should be handled in.
    * This webserver shall be able to serve large size files to the HTTP-POST or HTTP-GET requests.
    * It is hard to find any stable webserver option performing HTTP POST requests.

    I am sharing this code in hope that this template will be improved further for more stable and better Webserver on STM32F4 series.
    Please use and develop this further to make it better.


## Issues Known:
1. The server is not very stable and stops after sometime, reason unknown.
2. Sometimes it stops sending some part of the file from SD card: could be SD card reading  error.

# Note:


    I hope someone can have a look and solve this for others as well.

    Whoever knows LwIP stack on STM32F4 might guide us here.


## Sample post request to query the device in netowrk for test:
#### Test echo response from server:
```console
curl --location --request POST '<ip of device>/echo_request' \
--header 'Content-Type: application/json' \
--data-raw '{
    "hello":"world"
}'
```

#### Test Ethernet configuration:
```console
curl --location --request POST '<ip of device>/eth_config'
```

## Following functions need some work to be done:
```c
err_t httpd_post_begin(void *connection, const char *uri,
		const char *http_request, u16_t http_request_len, int content_len,
		char *response_uri, u16_t response_uri_len, u8_t *post_auto_wnd);
```

```c
void httpd_post_finished(void *connection, char *response_uri,
		u16_t response_uri_len);
```

```c
err_t httpd_post_receive_data(void *connection, struct pbuf *p);
```

```c
int fs_open_custom(struct fs_file *file, const char *name);

```
```c
void fs_close_custom(struct fs_file *file);

```
```c
int fs_read_custom(struct fs_file *file, char *buffer, int count);
```

These functions are in file: [httpd_post.c](Webserver/httpd_post.c)

## Syntax for adding HTTP POST request processing function:
These functions can be added to file: [webserver.c](Webserver/webserver.c)

```c
static void postRequestHandler(const char *request_payload,
		struct fs_file *post_resp_file) {
	memset(response_data, 0, 1024);
	/* 
    process request here and prepare resonse and save it in response data or other variable
    */

    // put information similar to following
	post_resp_file->data = response_data; // handler response
	post_resp_file->index = 0;
	post_resp_file->len = strlen(response_data); // response length
}
```
Later add this function as following:

```c
static const Api_Handle_t api_handles[] = {
    //---previous handlers and endpoints		
	{
		.api_uri = "/erase_eeprom",
		.api_handle = &handleEraseEEPROM
	},
    // --- next endpoints and handlers
};
```

### In future I hope I could adjust it so as to have GET and POST request handler together here.

