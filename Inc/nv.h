#ifndef _NV_H_
#define _NV_H_

#include "main.h"

#define _ENABLE_NV_ITEM_	1

#if _ENABLE_NV_ITEM_



#define _FLASH_ITEM_ADDRESS_ALIGN_(x) \
	((((x) >> 2) + 1) << 2)


#define _FLASH_PAGE_HEAD_OFSET_				sizeof(uint32_t)
	
#define _FLASH_PAGE_ADDR_PARA2_		0
#define _FLASH_PAGE_ADDR_PARA3_		0
#define _FLASH_PAGE_ADDR_PARA4_		0

#define _FLASH_ITEM_PAGE1_START_ADDRESS_	(FLASH_BASE + _FLASH_PAGE_ADDR_PARA2_ + _FLASH_PAGE_HEAD_OFSET_)
#define _FLASH_ITEM_PAGE2_START_ADDRESS_	(FLASH_BASE + _FLASH_PAGE_ADDR_PARA3_ + _FLASH_PAGE_HEAD_OFSET_)
#define _FLASH_ITEM_PAGE3_START_ADDRESS_	(FLASH_BASE + _FLASH_PAGE_ADDR_PARA4_ + _FLASH_PAGE_HEAD_OFSET_)
#define _FLASH_ITEM_PAGE_SIZE_				(0x800u)

#define _FLASH_PAGE_FLAG_INUSE_				(0xaaaaaaaaul)
#define _FLASH_PAGE_FLAG_UNUSE_				(0x00000000ul)

#define _FLASH_BLANK_FLAG_					0xfffffffful

/* 错误码 */
#define _ERR_NV_STAT_OK_					0x0000 /*  */
#define _ERR_NV_STAT_ID_END_				0x0011 /* 查找到末尾 */
#define _ERR_NV_STAT_ID_ALRD_				0x0012 /* 条目已经存在 */
#define _ERR_NV_STAT_ID_MISS_				0x0013 /* 无此条目 */
#define _ERR_NV_STAT_CHECK_BAD_				0x0014 /* 校验错误 */
#define _ERR_NV_STAT_BLANK_					0x0015 /* 空条目 */

/* NV 条目编号 */
#define _NV_ITEM_ID_DEF_MIN_				0x00002001
#define _NV_ITEM_ID_DEF_MAX_				0x00002ffe

#define _NV_ITEM_ID_DEF_UIP_				0x00002001
#define _NV_ITEM_ID_DEF_UPORT_				0x00002002
#define _NV_ITEM_ID_DEF_COMIP_				0x00002003
#define _NV_ITEM_ID_DEF_COMPORT_			0x00002004
#define _NV_ITEM_ID_DEF_CLIIP_				0x00002005
#define _NV_ITEM_ID_DEF_CLIPORT_			0x00002006
#define _NV_ITEM_ID_DEF_CYCLE_				0x00002007
#define _NV_ITEM_ID_DEF_ST_					0x00002008
#define _NV_ITEM_ID_DEF_PWD_				0x00002009
#define _NV_ITEM_ID_DEF_ID_					0x0000200a
#define _NV_ITEM_ID_DEF_MN_					0x0000200b
#define _NV_ITEM_ID_DEF_REALTIM_			0x0000200c
//
#define _NV_ITEM_ID_DEF_DENBASE_			0x0000200d
#define _NV_ITEM_ID_DEF_DEVADDR_			0x0000200e
#define _NV_ITEM_ID_DEF_SENSNUM_			0x0000200f
#define _NV_ITEM_ID_DEF_SENSADDR_			0x00002010

#define _NV_ITEM_ID_DEF_COEPM_				0x00002011
#define _NV_ITEM_ID_DEF_COETVOC_			0x00002012
#define _NV_ITEM_ID_DEF_COEYY_				0x00002013
//

#define _GET_PAGE1_INUSE_UNUSE_FLAG_ \
	(*(uint32_t*)(_FLASH_ITEM_PAGE1_START_ADDRESS_ - _FLASH_PAGE_HEAD_OFSET_))
	
#define _GET_PAGE2_INUSE_UNUSE_FLAG_ \
	(*(uint32_t*)(_FLASH_ITEM_PAGE2_START_ADDRESS_ - _FLASH_PAGE_HEAD_OFSET_))

#define _GET_INUSE_PAGE_ADDRESS_(addr)	\
	do \
	{ \
		if (_GET_PAGE1_INUSE_UNUSE_FLAG_ == _FLASH_PAGE_FLAG_INUSE_) \
		{ \
			addr = _FLASH_ITEM_PAGE1_START_ADDRESS_; \
		} \
		if (_GET_PAGE2_INUSE_UNUSE_FLAG_ == _FLASH_PAGE_FLAG_INUSE_) \
		{ \
			addr = _FLASH_ITEM_PAGE2_START_ADDRESS_; \
		} \
	}while(0)


//#pgma pack(push)
//#pragma pack(1)
typedef struct
{
	uint32_t ID; /* 条目ID */
	uint32_t ItemAddr; /* 条目起始地址 */
	uint32_t Length; /* 数据长度 */
	uint32_t Chk; /* 数据和校验 */
}NvHandler_t;
//#pragma pack(pop)

#define _NV_HEAD_INFO_OFFSET_	((uint32_t)(sizeof(NvHandler_t)))

extern uint16_t NvItemInit(uint32_t id, void* buf, uint32_t len);
extern uint16_t NvItemRead(uint32_t id, void* buf, uint32_t len);
extern uint16_t NvItemWrite(uint32_t id, void* buf, uint32_t len);
//extern uint16_t NvItemReadLength(void);

#endif

#endif
