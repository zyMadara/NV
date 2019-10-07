#include "nv.h"

#include <string.h>

#define _NV_ITEM_BACKUP_START_ADDRESS_	0x08040000
#define _NV_ITEM_START_ADDRESS_			0x08060000


#define _NV_BLANK_FLAG_			0xffff

#define _ERR_ITEM_NONE_			0x0000
#define _ERR_ITEM_END_			0x0001
#define _ERR_ITEM_ALRD_			0x1000


uint32_t NvOfsetInSector = 0;
uint32_t TotalLength = 0;

static void FlashSectorErase(uint32_t secAddr)
{
	FLASH_EraseInitTypeDef pEraseInit;
	uint32_t sectorError = 0;

	pEraseInit.Banks = FLASH_BANK_1;
	pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	if (sectorError == _NV_ITEM_BACKUP_START_ADDRESS_)
	{
		pEraseInit.Sector = FLASH_SECTOR_6;
	}
	else
	{
		pEraseInit.Sector = FLASH_SECTOR_7;
	}
	pEraseInit.NbSectors = 1;

	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&pEraseInit, &sectorError);
	HAL_FLASH_Lock();
}



static void ReadItemHdr(uint32_t addr, NvHandler_t* nv)
{
	uint8_t* p = (uint8_t*)nv;

	for (uint32_t i = 0; i < _NV_HEAD_OFFSET_; i++)
	{
		*(p + i) = (uint8_t) * ((uint8_t*)(addr)+i);
	}
}

static uint16_t FindItem(uint32_t id, uint32_t* srcAddr)
{
	NvHandler_t nv;
	uint32_t index = 0;

	*srcAddr = 0;

	if (*(uint16_t*)(_NV_ITEM_START_ADDRESS_) == _NV_BLANK_FLAG_)
	{
		return _ERR_ITEM_NONE_;
	}
	else
	{
		index = _NV_ITEM_START_ADDRESS_;

		while (1)
		{
			ReadItemHdr(index, &nv);

			if (nv.ID == id)
			{
				*srcAddr = index;
				return _ERR_ITEM_ALRD_;
			}
			if (nv.ID == 0xffffffff)
			{
				*srcAddr = index;
				return _ERR_ITEM_END_;
			}

			index += (_NV_HEAD_OFFSET_ + nv.Length);
		}
	}
}

uint8_t copyBuf[2048];

static uint16_t WriteItem(uint32_t id, void* buf, uint32_t len)
{
	uint32_t dstAddr;
	

	if (TotalLength > 2048)
	{
		return 0;
	}
	else
	{
		for (uint32_t i = 0; i < TotalLength; i++)
		{
			copyBuf[i] = (uint8_t)*(uint8_t*)(_NV_ITEM_START_ADDRESS_ + i);
		}

		if (FindItem(id, &dstAddr) == _ERR_ITEM_ALRD_)
		{
			if (*(uint32_t*)(dstAddr - _NV_ITEM_START_ADDRESS_ + copyBuf) == id)
			{
				uint32_t ndx = (uint32_t)(dstAddr - _NV_ITEM_START_ADDRESS_ + _NV_HEAD_OFFSET_);


				for (uint32_t k = 0; k < len; k++)
				{
					copyBuf[ndx + k] = *(((uint8_t*)(buf)) + k);
				}

				FlashSectorErase(_NV_ITEM_START_ADDRESS_);

				HAL_FLASH_Unlock();
				
				for (uint32_t j = 0; j < TotalLength; j++)
				{
					dstAddr = *(uint8_t *)(((uint32_t)(copyBuf)) + j);
					HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, _NV_ITEM_START_ADDRESS_ + j, dstAddr);
				}
				HAL_FLASH_Lock();
			}
		}
	}
	
	return 0;
}


static void ReadItemData(uint32_t id, void* buf, uint32_t len)
{
	uint32_t itemAddr;

	if (FindItem(id, &itemAddr) == _ERR_ITEM_ALRD_)
	{
		if (buf != NULL)
		{
			for (uint32_t i = 0; i < len; i++)
			{
				*((uint8_t*)(buf)+i) = (uint8_t) * (((uint8_t*)(itemAddr + _NV_HEAD_OFFSET_)) + i);
			}
		}
	}
	else
	{

	}
}

static void SetItem(NvHandler_t* nv, void* buf, uint32_t len)
{
	uint8_t* p = (uint8_t*)nv;


	HAL_FLASH_Unlock();

	for (uint32_t i = 0; i < _NV_HEAD_OFFSET_; i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, nv->ItemAddr + i, *(p + i));
	}

	for (uint32_t i = 0; i < len; i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, nv->ItemAddr + _NV_HEAD_OFFSET_ + i, (uint64_t) * ((uint8_t*)(buf)+i));
	}

	HAL_FLASH_Lock();
}

static uint16_t InitItem(uint32_t id, void* buf, uint32_t len)
{
	NvHandler_t nv;

	if (*(uint16_t*)(_NV_ITEM_START_ADDRESS_) == _NV_BLANK_FLAG_)
	{
		nv.ID = id;
		nv.ItemAddr = _NV_ITEM_START_ADDRESS_;
		nv.Length = len;
		nv.Chk = 0x11;
		SetItem(&nv, buf, len);

		NvOfsetInSector = nv.ItemAddr + nv.Length + _NV_HEAD_OFFSET_;
	}
	else
	{
		uint16_t err;
		uint32_t addr;

		err = FindItem(id, &addr);

		if (err == _ERR_ITEM_ALRD_)
		{
			return _ERR_ITEM_ALRD_;
		}
		else if (err == _ERR_ITEM_END_)
		{
			nv.ID = id;
			nv.ItemAddr = addr;
			nv.Length = len;
			nv.Chk = 0x11;
			SetItem(&nv, buf, len);

			NvOfsetInSector = nv.ItemAddr + nv.Length + _NV_HEAD_OFFSET_;
		}
	}

	return 0;
}

uint16_t NvItemInit(uint32_t id, void* buf, uint32_t len)
{
	return InitItem(id, buf, len);
}

uint16_t NvItemRead(uint32_t id, void* buf, uint32_t len)
{
	ReadItemData(id, buf, len);

	return 0;
}

uint16_t NvItemWrite(uint32_t id, void* buf, uint32_t len)
{
	WriteItem(id, buf, len);

	return 0;
}



uint8_t cc[12 + 20];

void Demo(void)
{

	char* p = "my name is zy";

	NvItemInit(0x0001, p, strlen(p));

	NvItemRead(0x0001, cc, 13);

	FindItem(0xffffffff, &TotalLength);
	
	TotalLength -= _NV_ITEM_START_ADDRESS_;

	p = "my name is tt";
	NvItemWrite(0x0001, p, 13);

	NvItemRead(0x0001, cc, 13);
}

