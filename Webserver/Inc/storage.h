/*
 * storage.h
 * For varibale storage use only 4096 bytes storage sector of W25q16
 *  Created on: 20-Jul-2022
 *      Author: mj
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include <stdbool.h>
#include <stdlib.h>
#include "lwip.h"

#define DEFAULT_EEPROM_SECTOR_ADDRESS_CONFIG	0

typedef struct
{
	ip4_addr_t ip;
	ip4_addr_t mask;
	ip4_addr_t gway;
	char hostname[32];
	bool dhcp_en;
} eth_netw_config_t;


/*	Addresses for eeprom for respective configuration structs */
#define EEPROM_FLASH_ADDR_ETH_NETW_CFG	0


/* initialize storage with defaults if unwritten */
void init_storage(void);
bool write_eeprom_data(const uint8_t *struct_data, uint32_t eep_address,
		size_t data_size);

#endif /* INC_STORAGE_H_ */
