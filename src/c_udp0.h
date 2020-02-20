/****************************************************************************
 *          C_UDP0.H
 *          Udp0 GClass.
 *
 *          GClass of UDP level 0 uv-mixin
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_UDP0_NAME "Udp0"
#define GCLASS_UDP0 gclass_udp0()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_udp0(void);

#ifdef __cplusplus
}
#endif
