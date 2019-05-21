/****************************************************************************
 *          C_UDP_C0.H
 *          GClass of UDP client level 0 uv-mixin.
 *
 *          Mixin uv-gobj
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_UDP_C0_H
#define _C_UDP_C0_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_udp_c0(void);

#define GCLASS_UDP_C0_NAME  "UdpC0"
#define GCLASS_UDP_C0 gclass_udp_c0()


#ifdef __cplusplus
}
#endif


#endif

