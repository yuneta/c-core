/****************************************************************************
 *          C_GSS_UDP_S0.H
 *          GssUdpS0 GClass.
 *
 *          Gossamer UDP Server level 0
 *
            Api Gossamer
            ------------

            Input Events:
            - EV_SEND_MESSAGE

            Output Events:
            - EV_ON_OPEN
            - EV_ON_CLOSE
            - EV_ON_MESSAGE

 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_GSS_UDP_S0_H
#define _C_GSS_UDP_S0_H 1

#include <ginsfsm.h>
#include "c_timer.h"
#include "c_udp_s0.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_gss_udp_s0(void);

#define GCLASS_GSS_UDP_S0_NAME "GssUdpS0"
#define GCLASS_GSS_UDP_S0 gclass_gss_udp_s0()

#ifdef __cplusplus
}
#endif


#endif
