#ifndef _NV_H_
#define _NV_H_

#include "stm32f4xx_hal.h"

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint32_t ID;
	uint32_t ItemAddr;
	uint32_t Length;
	uint32_t Chk;
}NvHandler_t;
#pragma pack(pop)

#define _NV_HEAD_OFFSET_	((uint32_t)(sizeof(NvHandler_t)))


extern void Demo(void);


#endif
