#include <string.h>
#include "CANopen.h"
#include "storage/CO_storage.h"
#include "OD.h"
#include "CO_Flash.h"
#include "version.h"

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE

#ifndef PRODUCT_CODE
#define PRODUCT_CODE	0x0
#endif

#ifndef COMMIT_ID
#define COMMIT_ID		0x0
#endif

#define OD_MAGIC_START	0xCAFEBEEF
#define OD_MAGIC_END	0xBEEFCAFE

__attribute__((section(".OD_FACTORY_START"), used))
const Storage_Flash_t OD_FACTORY = {
	.Header = {
		.OD_Length = sizeof(OD_PERSIST_COMM_t),
		.NodeId = 0x7E,
		.Bitrate = 250,
		.ProductCode = PRODUCT_CODE,
		.RevisionNumber = COMMIT_ID,
		.SerialNumber = 0,
		.OD_MagicStart = OD_MAGIC_START,
	},
#include "OD_Flash.h"
	.OD_MagicEnd = OD_MAGIC_END,
};

__attribute__((section(".OD_RUNTIME_START"), used))
const Storage_Header_t OD_RUNTIME = {
	.OD_Length = 0xFFFFFFFF,
	.NodeId = 0x7E,
	.Bitrate = 250,
	.ProductCode = 0,
	.RevisionNumber = 0,
	.SerialNumber = 0,
	.OD_MagicStart = 0xFFFFFFFF,
};

CO_storage_t storage;

const CO_storage_entry_t storageEntries[] = {
#define OD_RUNTIME_INDEX	0
		{	.addr = &OD_PERSIST_COMM,
			.len = sizeof(OD_PERSIST_COMM),
			.subIndexOD = 2,
			.attr = CO_storage_cmd | CO_storage_restore,
			.eepromAddr = (uint32_t)&OD_RUNTIME
		},
#define OD_FACTORY_INDEX	1
		{
			.addr = &OD_PERSIST_COMM,
			.len = sizeof(OD_PERSIST_COMM),
			.subIndexOD = 4,
			.attr = CO_storage_restore,
			.eepromAddr = (uint32_t)&OD_FACTORY
		},
        CO_STORAGE_APPLICATION
    };
#endif

CO_ReturnError_t CO_Flash_init(CO_t *CO, uint32_t *storageInitError)
{
	CO_ReturnError_t err = CO_storage_init(
								&storage,
								CO->CANmodule,
								OD_ENTRY_H1010_storeParameters,
								OD_ENTRY_H1011_restoreDefaultParameters,
								CO_Flash_store,
								CO_Flash_restore,
								(CO_storage_entry_t *)&storageEntries[0],
								(sizeof(storageEntries) / sizeof(storageEntries[0]))
								);
	if (err == CO_ERROR_NO)
		*storageInitError = 0;
	else
		*storageInitError = 1;
	return err;
}

ODR_t CO_Flash_store(CO_storage_entry_t* entry, CO_CANmodule_t* CANmodule)
{
	return ODR_OK;
}

ODR_t CO_Flash_restore(CO_storage_entry_t* entry, CO_CANmodule_t* CANmodule)
{
	return ODR_OK;
}

void CO_Flash_auto_process(bool_t saveAll)
{

}

#if 0
/**
 * Initialize eeprom device, target system specific function.
 *
 * @param storageModule Pointer to storage module.
 *
 * @return True on success
 */
bool_t CO_eeprom_init(void* storageModule)
{
	return false;
}

/**
 * Get free address inside eeprom, target system specific function.
 *
 * Function is called several times for each storage block in the initialization phase after CO_eeprom_init().
 *
 * @param storageModule Pointer to storage module.
 * @param isAuto True, if variable is auto stored or false if protected
 * @param len Length of data, which will be stored to that location
 * @param [out] overflow set to true, if not enough eeprom memory
 *
 * @return Asigned eeprom address
 */
size_t CO_eeprom_getAddr(void* storageModule, bool_t isAuto, size_t len, bool_t* overflow)
{
	return 0;
}

/**
 * Read block of data from the eeprom, target system specific function.
 *
 * @param storageModule Pointer to storage module.
 * @param data Pointer to data buffer, where data will be stored.
 * @param eepromAddr Address in eeprom, from where data will be read.
 * @param len Length of the data block to be read.
 */
void CO_eeprom_readBlock(void* storageModule, uint8_t* data, size_t eepromAddr, size_t len)
{
	memset(data, 0, len);
}

/**
 * Write block of data to the eeprom, target system specific function.
 *
 * It is blocking function, so it waits, until all data is written.
 *
 * @param storageModule Pointer to storage module.
 * @param data Pointer to data buffer which will be written.
 * @param eepromAddr Address in eeprom, where data will be written. If data is stored across multiple pages, address
 * must be aligned with page.
 * @param len Length of the data block.
 *
 * @return true on success
 */
bool_t CO_eeprom_writeBlock(void* storageModule, uint8_t* data, size_t eepromAddr, size_t len)
{
	return false;
}

/**
 * Get CRC checksum of the block of data stored in the eeprom, target system specific function.
 *
 * @param storageModule Pointer to storage module.
 * @param eepromAddr Address of data in eeprom.
 * @param len Length of the data.
 *
 * @return CRC checksum
 */
uint16_t CO_eeprom_getCrcBlock(void* storageModule, size_t eepromAddr, size_t len)
{
	return 0;
}

/**
 * Update one byte of data in the eeprom, target system specific function.
 *
 * Function is used by automatic storage. It updates byte in eeprom only if differs from data.
 *
 * @param storageModule Pointer to storage module.
 * @param data Data byte to be written
 * @param eepromAddr Address in eeprom, from where data will be updated.
 *
 * @return true if write was successful or false, if still waiting previous data to finish writing.
 */
bool_t CO_eeprom_updateByte(void* storageModule, uint8_t data, size_t eepromAddr)
{
	return false;
}
#endif
