/****************************************************************************
 *          C_CONNEX.H
 *          GClass of CONNEX: auto-connection and multi-destine over tcp.
 *

    Input Events        CLIENT tcp              Output Events
                    ┌───────────────────┐
        start   ━━━▷│●                  │
                    │-------------------│
                    │                   │====▷  EV_CONNECTED
    EV_TX_DATA ====▷│                   │====▷  EV_TX_READY
    EV_DROP    ====▷│                   │====▷  EV_RX_DATA
                    │                   │====▷  EV_DISCONNECTED
                    │-------------------│
        stop    ━━━▷│■                  │
                    │                   │====▷  EV_STOPPED
                    └───────────────────┘


 *
 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_tcp0.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_connex(void);

#define GCLASS_CONNEX_NAME "Connex"
#define GCLASS_CONNEX gclass_connex()

#ifdef __cplusplus
}
#endif
