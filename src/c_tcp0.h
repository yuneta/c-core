/****************************************************************************
 *          C_TCP0.H
 *          GClass of TCP level 0 uv-mixin.
 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 *

    Two types of tcp: client and clisrv.

    client
    ------

    Pure tcp client.
    Begin the connection with start(●).
    Broke the connection with pause(■) or
    It's auto-stop(■) if the connection is broken by the peer.
    Re-use with manual start(●).

    Input Events        CLIENT tcp              Output Events
                    ┌───────────────────┐
        start   ━━━▷│●                  │
                    │-------------------│
                    │                   │====▷  EV_CONNECTED
    EV_TX_DATA ====▷│                   │====▷  EV_TX_READY
    EV_DROP    ====▷│                   │====▷  EV_RX_DATA
                    │-------------------│
        stop    ━━━▷│■  ◁---(auto)      │====▷  EV_DISCONNECTED
                    │                   │====▷  EV_STOPPED
                    └───────────────────┘


    clisrv
    ------
    A tcp from accepted server connection.
    Connected from the beginning, auto-start(●).
    Broke the connection with pause(■) or
    It's auto-stop(■) if the connection is broken by the peer.
    ONLY one use!. You must stop(■) and destroy when it's disconnected.

    Input Events        SRVCLI tcp              Output Events
                    ┌───────────────────┐
                    │●  ◁---(auto)      │
                    │-------------------│
                    │                   │====▷  EV_CONNECTED
    EV_TX_DATA ====▷│                   │====▷  EV_TX_READY
                    │                   │====▷  EV_RX_DATA
                    │-------------------│
        stop    ━━━▷│■  ◁---(auto)      │====▷  EV_DISCONNECTED
                    │                   │====▷  EV_STOPPED
                    └───────────────────┘




    Api IOGadget
    ------------

    Connect with gobj_play()        OLD EV_CONNECT
    Disconnect with gobj_pause()    OLD EV_DROP

    Input Events:
    * EV_TX_DATA

    Output Events:
    * EV_CONNECTED
    * EV_RX_DATA
    * EV_TX_READY
    * EV_DISCONNECTED
    * EV_STOPPED   (Only NOW you can destroy the gobj)


 ****************************************************************************/

#ifndef _C_TCP0_H
#define _C_TCP0_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_tcp0(void);

#define GCLASS_TCP0_NAME  "Tcp0"
#define GCLASS_TCP0 gclass_tcp0()

PUBLIC int accept_connection0(
    hgobj clisvr,
    void *uv_server_socket
);

#ifdef __cplusplus
}
#endif


#endif

