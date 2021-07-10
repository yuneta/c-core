/****************************************************************************
 *          C_UDP_S0.H
 *          GClass of UDP server level 0 uv-mixin.
 *
 *          Mixin uv-gobj
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_udp_s0(void);

#define GCLASS_UDP_S0_NAME  "UdpS0"
#define GCLASS_UDP_S0 gclass_udp_s0()


#ifdef __cplusplus
}
#endif
