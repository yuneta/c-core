/****************************************************************************
 *          C_TCP_S0.H
 *          GClass of TCP server level 0 mixin-uv.
 *

    Input Events                                Output Events
                        ┌───────────────────┐
            start   ━━━▷│●                  │
                        │-------------------│
                        │                   │
        EV_STOPPED  ===▷│                   │
                        │                   │
                        │-------------------│
            stop    ━━━▷│■                  │====▷  EV_STOPPED
                        └───────────────────┘


 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_TCP_S0_H
#define _C_TCP_S0_H 1

#include <ginsfsm.h>
#include "c_yuno.h"
#include "c_tcp0.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_tcp_s0(void);

#define GCLASS_TCP_S0_NAME "TcpS0"
#define GCLASS_TCP_S0 gclass_tcp_s0()

#ifdef __cplusplus
}
#endif


#endif

