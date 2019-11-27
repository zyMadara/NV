#include "nv.h"
#include <string.h>
#include <stdbool.h>

#if _ENABLE_NV_ITEM_

static uint32_t CheckSum(uint8_t* p, uint16_t len)
{
	uint32_t retval = 0;

	for (uint16_t i = 0; i < len; i++)
	{
		retval += *(p + i);
	}

	return retval;
}

static void ReadItemHdr(uint32_t addr, NvHandler_t* nv)
{
	uint8_t* p = (uint8_t*)nv;

	for (uint32_t i = 0; i < _NV_HEAD_INFO_OFFSET_; i++)
	{
		*(p + i) = (uint8_t) * ((uint8_t*)(addr)+i);
	}
}

static uint16_t FindItem(uint32_t id, NvHandler_t* nv, const uint32_t page)
{
	uint32_t index = 0;

	nv->Chk =
		nv->ID =
		nv->ItemAddr =
		nv->Length = _FLASH_BLANK_FLAG_;

	if (*(uint32_t*)(_FLASH_ITEM_ADDRESS_ALIGN_(/*_FLASH_ITEM_PAGE1_START_ADDRESS_*/page)) == _FLASH_BLANK_FLAG_)
	{
		return _ERR_NV_STAT_BLANK_;
	}
	else
	{
		index = _FLASH_ITEM_ADDRESS_ALIGN_(/*_FLASH_ITEM_PAGE1_START_ADDRESS_*/page);

		while (1)
		{
			ReadItemHdr(index, nv);

			if (nv->ID == id)
			{
				return _ERR_NV_STAT_ID_ALRD_;
			}
			else if (nv->ID == _FLASH_BLANK_FLAG_)
			{
				nv->ItemAddr = index;
				return _ERR_NV_STAT_ID_END_;
			}
			else if (nv->ID == 0)
			{
				// TO-DO
			}

			index += (_NV_HEAD_INFO_OFFSET_ + nv->Length);
			index = _FLASH_ITEM_ADDRESS_ALIGN_(index);
		}
	}
}

static void SetItem(NvHandler_t* nv, void* buf, uint32_t len)
{
	uint16_t* p = (uint16_t*)buf;
	uint32_t addrOfset = 0;

	HAL_FLASH_Unlock();

	addrOfset = nv->ItemAddr;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addrOfset, nv->ID);
	addrOfset += 4;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addrOfset, nv->ItemAddr);
	addrOfset += 4;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addrOfset, nv->Length);
	addrOfset += 4;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addrOfset, nv->Chk);
	addrOfset += 4;

	if (buf != NULL)
	{
		for (uint32_t i = 0; i < len; i += 2)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addrOfset + i, *(p++));
		}
	}

	HAL_FLASH_Lock();
}

static uint16_t CheckItem(NvHandler_t* nv)
{
	NvHandler_t sNv;
	uint32_t page = 0;

	_GET_INUSE_PAGE_ADDRESS_(page);

	if (FindItem(nv->ID, &sNv, page) == _ERR_NV_STAT_ID_ALRD_)
	{
		if (CheckSum((uint8_t*)(sNv.ItemAddr + _NV_HEAD_INFO_OFFSET_), sNv.Length) != nv->Chk)
		{
			sNv.ID = 0;
			sNv.ItemAddr = nv->ItemAddr;
			sNv.Length = nv->Length;
			sNv.Chk = nv->Chk;
			SetItem(&sNv, NULL, nv->Length);

			return _ERR_NV_STAT_CHECK_BAD_;
		}

		return _ERR_NV_STAT_OK_;
	}

	return _ERR_NV_STAT_ID_MISS_;
}

#pragma O0
static uint16_t WriteItem(uint32_t id, void* buf, uint32_t len)
{
#define _FLASH_COPY_BUFFER_LENGTH_		20 

	uint8_t copyBuf[_FLASH_PAGE_HEAD_OFSET_ + _FLASH_COPY_BUFFER_LENGTH_] = { 0 };
	uint16_t err;
	volatile uint32_t srcAddr = 0;
	volatile uint32_t dstAddr = 0;
	uint32_t page = 0;
	NvHandler_t nv;

	_GET_INUSE_PAGE_ADDRESS_(page);

	if (FindItem(id, &nv, page) == _ERR_NV_STAT_ID_ALRD_)
	{

		if (page == _FLASH_ITEM_PAGE1_START_ADDRESS_)
		{
			srcAddr = _FLASH_ITEM_PAGE1_START_ADDRESS_;
			dstAddr = _FLASH_ITEM_PAGE2_START_ADDRESS_;
		}
		else
		{
			srcAddr = _FLASH_ITEM_PAGE2_START_ADDRESS_;
			dstAddr = _FLASH_ITEM_PAGE1_START_ADDRESS_;
		}

		// to erase flash page
		//FlashPageErase(dstAddr, 1);

		HAL_FLASH_Unlock();
		for (uint32_t xid = _NV_ITEM_ID_DEF_MIN_; xid <= _NV_ITEM_ID_DEF_MAX_; xid++)
		{
			err = FindItem(xid, &nv, page);

			if (nv.Length > _FLASH_COPY_BUFFER_LENGTH_)
			{
				return 0xffff;
			}

			if (err == _ERR_NV_STAT_ID_ALRD_)
			{
				for (uint32_t cAddr = 0; cAddr < nv.Length; cAddr++)
				{
					copyBuf[cAddr] = (uint8_t) * (uint8_t*)(cAddr + nv.ItemAddr + _NV_HEAD_INFO_OFFSET_);
				}

				if (nv.ID == id)
				{
					memcpy(copyBuf, buf, len);
					nv.Chk = CheckSum(copyBuf, len);
				}

				nv.ItemAddr = dstAddr + nv.ItemAddr - page;;
				SetItem(&nv, copyBuf, nv.Length);
			}
			else if (err == _ERR_NV_STAT_ID_END_)
			{
				break;
			}

			memset(copyBuf, 0, _FLASH_PAGE_HEAD_OFSET_ + 20);
		}
		// write inuse or unuse flag to the head of page selected
		//FlashWriteInuseFlag(dstAddr, _FLASH_PAGE_FLAG_INUSE_);
		//FlashWriteInuseFlag(srcAddr, _FLASH_PAGE_FLAG_UNUSE_);
		HAL_FLASH_Lock();

		page = (page == _FLASH_ITEM_PAGE1_START_ADDRESS_) ? _FLASH_ITEM_PAGE2_START_ADDRESS_ : _FLASH_ITEM_PAGE1_START_ADDRESS_;

	CHECKERR:
		if (FindItem(id, &nv, page) == _ERR_NV_STAT_ID_ALRD_)
		{
			if (CheckItem(&nv) == _ERR_NV_STAT_CHECK_BAD_)
			{
				NvHandler_t newPosNv;

				if (FindItem(_NV_ITEM_ID_DEF_MAX_, &newPosNv, page) == _ERR_NV_STAT_ID_END_)
				{
					nv.ItemAddr = newPosNv.ItemAddr;
					SetItem(&nv, buf, len);
					goto CHECKERR;
				}
				else
				{
					//bug: next id data already stored maybe overwrited
					//nv.ItemAddr += _NV_HEAD_INFO_OFFSET_ + nv.Length;
				}
			}
		}
	}

	return _ERR_NV_STAT_OK_;
}

static uint16_t ReadItemData(uint32_t id, void* buf, uint32_t len)
{
	NvHandler_t nv;
	uint32_t page;
	uint8_t cnt = 0;

	_GET_INUSE_PAGE_ADDRESS_(page);

READLOOP:
	if (FindItem(id, &nv, page) == _ERR_NV_STAT_ID_ALRD_)
	{
		if (buf != NULL)
		{
			for (uint32_t i = 0; i < len; i++)
			{
				*((uint8_t*)(buf)+i) = (uint8_t) * (((uint8_t*)(nv.ItemAddr + _NV_HEAD_INFO_OFFSET_)) + i);
			}

			if (CheckSum((uint8_t*)(nv.ItemAddr + _NV_HEAD_INFO_OFFSET_), nv.Length) != nv.Chk)
			{
				page = (page == _FLASH_ITEM_PAGE1_START_ADDRESS_) ? _FLASH_ITEM_PAGE2_START_ADDRESS_ : _FLASH_ITEM_PAGE1_START_ADDRESS_;

				cnt++;

				if (cnt < 2)
				{
					goto READLOOP;
				}
				else
				{
					return _ERR_NV_STAT_CHECK_BAD_;
				}
			}
		}
	}
	else
	{
		return _ERR_NV_STAT_ID_MISS_;
	}

	return _ERR_NV_STAT_OK_;
}



static uint16_t InitItem(uint32_t id, void* buf, uint32_t len)
{
	bool wr = false;
	NvHandler_t nv;
	uint16_t err;

WRITELOOP:
	if (*(uint32_t*)(_FLASH_ITEM_ADDRESS_ALIGN_(_FLASH_ITEM_PAGE1_START_ADDRESS_)) == _FLASH_BLANK_FLAG_)
	{
		nv.ItemAddr = _FLASH_ITEM_ADDRESS_ALIGN_(_FLASH_ITEM_PAGE1_START_ADDRESS_);
		wr = true;
	}
	else
	{
		err = FindItem(id, &nv, _FLASH_ITEM_PAGE1_START_ADDRESS_);

		if (err == _ERR_NV_STAT_ID_ALRD_)
		{
			return _ERR_NV_STAT_ID_ALRD_;
		}
		else if (err == _ERR_NV_STAT_ID_END_)
		{
			wr = true;
		}
	}

	if (wr)
	{
		nv.ID = id;
		nv.ItemAddr = nv.ItemAddr;
		nv.Length = len;
		nv.Chk = CheckSum(buf, len);
		SetItem(&nv, buf, len);

		if (CheckItem(&nv) == _ERR_NV_STAT_CHECK_BAD_)
		{
			nv.ItemAddr += _NV_HEAD_INFO_OFFSET_ + nv.Length;

			goto WRITELOOP;
		}
	}

	return _ERR_NV_STAT_OK_;
}

//************************************
// Method:    NvItemInit
// Access:    public 
// Returns:   uint16_t
// Qualifier: 条目初始化，仅需调用一次
// Parameter: uint32_t id
// Parameter: void * buf
// Parameter: uint32_t len
//************************************
uint16_t NvItemInit(uint32_t id, void* buf, uint32_t len)
{
	return InitItem(id, buf, len);
}

//************************************
// Method:    NvItemRead
// Access:    public 
// Returns:   uint16_t
// Qualifier: 读条目内容
// Parameter: uint32_t id
// Parameter: void * buf
// Parameter: uint32_t len
//************************************
uint16_t NvItemRead(uint32_t id, void* buf, uint32_t len)
{
	return ReadItemData(id, buf, len);
}

//************************************
// Method:    NvItemWrite
// Access:    public 
// Returns:   uint16_t
// Qualifier: 更改条目内容
// Parameter: uint32_t id
// Parameter: void * buf
// Parameter: uint32_t len
//************************************
uint16_t NvItemWrite(uint32_t id, void* buf, uint32_t len)
{
	WriteItem(id, buf, len);

	return _ERR_NV_STAT_OK_;
}

//************************************
// Method:    NvItemReadLength
// Access:    public 
// Returns:   uint16_t
// Qualifier: 获取条目总长度
// Parameter: void
//************************************
//uint16_t NvItemReadLength(void)
//{
//	NvHandler_t nv;
//	uint32_t page;

//	_GET_INUSE_PAGE_ADDRESS_(page);
//	FindItem(_NV_ITEM_ID_DEF_MAX_, &nv, page);

//	return (nv.ItemAddr - page + _FLASH_PAGE_HEAD_OFSET_);
//}

#endif



