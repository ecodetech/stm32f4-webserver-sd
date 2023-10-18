/*
 * webserver.c
 *
 *  Created on: 14-Jul-2022
 *  Author: Manoj Jadhao (Electrocode Technologies)
 *  Website: https://www.electrocode.in
 */
#include <stddef.h>
#include <string.h>
#include "webserver.h"
#include "lwjson/lwjson.h"
#include "json-maker/json-maker.h"
#include "httpd_post.h"
#include "lwip.h"
#include "dhcp.h"
#include "main.h"
#include "common_strings.h"
#include "w25qxx.h"
#include "storage.h"

typedef struct {
	const char *api_uri;
	api_handler_t api_handle;
} Api_Handle_t;

__attribute__((section(".ccmram"))) char response_data[1024] = "";

__attribute__((section(".ccmram"))) lwjson_token_t tokens[128];
__attribute__((section(".ccmram"))) lwjson_t lwjson;

static void handleEchoRequest(const char *request_payload,
		struct fs_file *post_resp_file) {
	memset(response_data, 0, 1024);
	sprintf( response_data,"%s%s",request_payload, "\r\n\r\n");

	post_resp_file->data = response_data; // handler echo request
	post_resp_file->index = 0;
	post_resp_file->len = strlen(response_data);
}


static void handleEraseEEPROM(const char *request_payload,
		struct fs_file *post_resp_file) {
	UNUSED(request_payload);
	char *p;
	//response_data = (char*) malloc(200);
	memset(response_data, 0, 200);
	p = response_data;
	size_t json_len = 200;
	p = json_objOpen(p, NULL, &json_len);
	p = json_bool(p, "status", true, &json_len);
	p = json_objClose(p, &json_len);
	p = json_end(p, &json_len);
	post_resp_file->len = (size_t) (p - response_data);
	post_resp_file->data = response_data;
	post_resp_file->index = 0;
	W25qxx_EraseBlock(0);
}

static void handleIP_Config(const char *request_payload,
		struct fs_file *post_resp_file) {
	char *p;
	//response_data = (char*) malloc(200);
	memset(response_data, 0, 200);
	p = response_data;
	size_t json_len = 200;
	if (*request_payload == 0) {
		//char temp_buff [20]="";
#ifdef DP83848_PHY_ADDRESS
		struct netif *gnetif = netif_get_by_index(DP83848_PHY_ADDRESS);
#else
        // find new index value
		struct netif *gnetif = netif_get_by_index(LAN8742A_PHY_ADDRESS);
#endif

		uint8_t *mac = gnetif->hwaddr;
		eth_netw_config_t temp_eth_cfg;
//		W25qxx_ReadBytes((uint8_t*) &temp_eth_cfg,
//		EEPROM_FLASH_ADDR_ETH_NETW_CFG, sizeof(temp_eth_cfg));
		char temp_buff[18];
		p = json_objOpen(p, NULL, &json_len);
		p = json_str(p, str_ip, ip4addr_ntoa(&gnetif->ip_addr), &json_len);
		p = json_str(p, str_mask, ip4addr_ntoa(&gnetif->netmask), &json_len);
		p = json_str(p, str_gway, ip4addr_ntoa(&gnetif->gw), &json_len);
		sprintf(temp_buff, "%02X:%02X:%02X:%02X:%02X:%02X", *(mac), *(mac + 1),
				*(mac + 2), *(mac + 3), *(mac + 4), *(mac + 5));
		p = json_str(p, str_mac, temp_buff, &json_len);
		p = json_str(p, str_hostname, temp_eth_cfg.hostname, &json_len);
		p = json_bool(p, str_dhcp, temp_eth_cfg.dhcp_en, &json_len);
		p = json_objClose(p, &json_len);
		p = json_end(p, &json_len);

	} else {
		bool ret_err = false;
		eth_netw_config_t temp_eth_cfg;
		lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens)); // don't forget to free lwjson buffer later

		if (lwjson_parse(&lwjson, request_payload) == lwjsonOK) {
			const lwjson_token_t *t;
			W25qxx_ReadBytes((uint8_t*) &temp_eth_cfg, 0, sizeof(temp_eth_cfg));
			Send_Uart("\r\n");
			Send_Uart(ip4addr_ntoa(&temp_eth_cfg.ip));
			Send_Uart("\r\nJSON parsed..\r\n");

			if ((t = lwjson_find(&lwjson, "eeprom")) != NULL) {
				p = json_objOpen(p, NULL, &json_len);
				p = json_str(p, str_ip, ip4addr_ntoa(&temp_eth_cfg.ip),
						&json_len);
				p = json_str(p, str_mask, ip4addr_ntoa(&temp_eth_cfg.mask),
						&json_len);
				p = json_str(p, str_gway, ip4addr_ntoa(&temp_eth_cfg.gway),
						&json_len);
				p = json_str(p, str_hostname, temp_eth_cfg.hostname, &json_len);
				p = json_bool(p, str_dhcp, temp_eth_cfg.dhcp_en, &json_len);
				p = json_objClose(p, &json_len);
				p = json_end(p, &json_len);
			} else {
				/* Find ip, mask gateway key in JSON */
				if ((t = lwjson_find(&lwjson, str_ip)) != NULL) {
					//char temp_buff[lwjson_get_val_string_length(t)];
					size_t str_len = lwjson_get_val_string_length(t);
					char *ip_string = (char*) malloc(sizeof(char) * str_len);
					strncpy(ip_string, lwjson_get_val_string(t, &str_len),
							str_len);
					*(ip_string + str_len) = 0;
					if (ip4addr_aton((const char*) ip_string, &temp_eth_cfg.ip)
							== 0) {
						Send_Uart("IP error");
						ret_err = true;
					}
					Send_Uart(ip_string);
					Send_Uart("\r\n");
					Send_Uart(ip4addr_ntoa(&temp_eth_cfg.ip));
					Send_Uart("\r\n");
					free(ip_string);
					ip_string = NULL;

				}

				if ((t = lwjson_find(&lwjson, str_mask)) != NULL) {
					//char temp_buff[lwjson_get_val_string_length(t)];
					size_t str_len = lwjson_get_val_string_length(t);
					char *ip_string = (char*) malloc(sizeof(char) * str_len);
					strncpy(ip_string, lwjson_get_val_string(t, &str_len),
							str_len);
					*(ip_string + str_len) = 0;
					if (ip4addr_aton((const char*) ip_string,
							&temp_eth_cfg.mask) == 0) {
						Send_Uart("mask error");
						ret_err = true;
					}
					Send_Uart(ip_string);
					Send_Uart("\r\n");
					free(ip_string);
					ip_string = NULL;
				}

				if ((t = lwjson_find(&lwjson, str_gway)) != NULL) {
					//char temp_buff[lwjson_get_val_string_length(t)];
					size_t str_len = lwjson_get_val_string_length(t);
					char *ip_string = (char*) malloc(sizeof(char) * str_len);
					strncpy(ip_string, lwjson_get_val_string(t, &str_len),
							str_len);
					*(ip_string + str_len) = 0;
					if (ip4addr_aton((const char*) ip_string,
							&temp_eth_cfg.gway) == 0) {
						Send_Uart("gateway error");
						ret_err = true;
					}
					Send_Uart(ip_string);
					Send_Uart("\r\n");
					free(ip_string);
					ip_string = NULL;
				}
				if ((t = lwjson_find(&lwjson, str_hostname)) != NULL) {
					//char temp_buff[lwjson_get_val_string_length(t)];
					size_t str_len = lwjson_get_val_string_length(t);

					strncpy(temp_eth_cfg.hostname,
							lwjson_get_val_string(t, &str_len), str_len);
					temp_eth_cfg.hostname[str_len] = 0;
				}
				if ((t = lwjson_find(&lwjson, str_dhcp)) != NULL) {
					//char temp_buff[lwjson_get_val_string_length(t)];
					temp_eth_cfg.dhcp_en = (t->type != LWJSON_TYPE_FALSE);
				}
				// if no error in config received, saved the configuration to eeprom
				if (ret_err == false) {
					if (!write_eeprom_data((uint8_t*) &temp_eth_cfg,
					EEPROM_FLASH_ADDR_ETH_NETW_CFG, sizeof(temp_eth_cfg))) {
						ret_err = true;
					}
					Send_Uart("\r\nsaved check:");
					W25qxx_ReadBytes((uint8_t*) &temp_eth_cfg, 0,
							sizeof(temp_eth_cfg));
					Send_Uart(ip4addr_ntoa(&temp_eth_cfg.ip));
					Send_Uart("\r\n");
				}
				p = json_objOpen(p, NULL, &json_len);
				p = json_bool(p, str_error, ret_err, &json_len);
				p = json_objClose(p, &json_len);
				p = json_end(p, &json_len);
			}

		}
		/* Call this when not used anymore */
		lwjson_free(&lwjson);
	}
	Send_Uart(response_data);
	post_resp_file->len = (size_t) (p - response_data);
	post_resp_file->data = response_data;
	post_resp_file->index = 0;
}

static const Api_Handle_t api_handles[] = {
		{
				.api_uri = "/echo_request",
				.api_handle = &handleEchoRequest
		},
		{
				.api_uri = "/eth_config",
				.api_handle = &handleIP_Config
		},
		{
				.api_uri = "/erase_eeprom",
				.api_handle = &handleEraseEEPROM
		},

};

api_handler_t Get_Api_Uri_Handler(const char *request_uri) {
	int api_handle_count = sizeof(api_handles) / sizeof(api_handles[0]);
	for (int i = 0; i < api_handle_count; i++) {
		if (strcmp(request_uri, api_handles[i].api_uri) == 0) {
			return api_handles[i].api_handle;
		}
	}
	return NULL;
}
