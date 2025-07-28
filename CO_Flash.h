#ifndef CO_FLASH_H
#define CO_FLASH_H

#include "storage/CO_storage.h"
#include "301/CO_ODinterface.h"
#include "OD.h"

#if ((CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE) || defined CO_DOXYGEN

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		uint32_t OD_Length;			// OD_PERSIST_ROM size in bytes
		uint32_t NodeId;
		uint32_t Bitrate;
		uint32_t ProductCode;
		uint32_t RevisionNumber;
		uint32_t SerialNumber;
		uint32_t OD_MagicStart;
	} Storage_Header_t;

	typedef struct {
		Storage_Header_t Header;
		OD_PERSIST_COMM_t OD_PERSIST_COMM;
		uint32_t OD_MagicEnd;
	} Storage_Flash_t;

	ODR_t CO_Flash_WriteRuntime(void);
	ODR_t CO_Flash_WriteEeprom(void);
	ODR_t CO_Flash_ReadEeprom(void);

	extern const Storage_Flash_t OD_FACTORY;
	extern const Storage_Header_t OD_RUNTIME;

	CO_ReturnError_t CO_Flash_init(CO_t *CO, uint32_t *storageInitError);
	void CO_Flash_auto_process(bool_t saveAll);
	ODR_t CO_Flash_store(CO_storage_entry_t* entry, CO_CANmodule_t* CANmodule);
	ODR_t CO_Flash_restore(CO_storage_entry_t* entry, CO_CANmodule_t* CANmodule);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE */
#endif /* CO_STORAGE_BLANK_H */
