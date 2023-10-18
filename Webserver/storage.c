/*
 * storage.c
 *
 *  Created on: 14-Jul-2022
 *  Author: Manoj Jadhao (Electrocode Technologies)
 *  Website: https://www.electrocode.in
 */

#include "storage.h"
#include "w25qxx.h"
#include <string.h>
#include <math.h>



void init_storage(void) {

}

bool write_eeprom_data(const uint8_t *struct_data, uint32_t eep_address,
		size_t data_size) {
	if (eep_address + data_size > 4096) {
		return false;
	}
	uint8_t *temp_storage;

	temp_storage = (uint8_t*) malloc(w25qxx.SectorSize * sizeof(uint8_t));
	memset(temp_storage, 0, w25qxx.SectorSize * sizeof(uint8_t));

	//backup data
	W25qxx_ReadSector(temp_storage, DEFAULT_EEPROM_SECTOR_ADDRESS_CONFIG, 0,
			w25qxx.SectorSize);
	//modify chnages
	memcpy(temp_storage + eep_address, struct_data, data_size);
	//erase sector before write
	W25qxx_EraseSector(DEFAULT_EEPROM_SECTOR_ADDRESS_CONFIG);
	// write sector with modified data
	W25qxx_WriteSector(temp_storage, DEFAULT_EEPROM_SECTOR_ADDRESS_CONFIG, 0,
			w25qxx.SectorSize);

	free(temp_storage);
	temp_storage = NULL;
	return true;
}
