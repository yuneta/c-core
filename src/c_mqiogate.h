/****************************************************************************
 *          C_MQIOGATE.H
 *          Mqiogate GClass.
 *
 *          Multiple Persistent Queue IOGate
 *
 *          Copyright (c) 2019 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_iogate.h"
#include "c_channel.h"
#include "c_resource.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_MQIOGATE_NAME "MQIOGate"
#define GCLASS_MQIOGATE gclass_mqiogate()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_mqiogate(void);

#ifdef __cplusplus
}
#endif
