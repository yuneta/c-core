/****************************************************************************
 *          C_ROUTER.H
 *          GClass of ROUTER gobj.
 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

/*^^ ======================== start sphinx doc ================================

Router GClass
=============

    Interchange of events through yunos.

    Persistent ievents:
        Store: "router"
        Room:  my_yuno_role+my_yuno_name
        Shelf: iev_dst_role+iev_dst_yuno

WARNING: the symbol "+" cannot be in role or yuno name!!! ("^" "*" neither!)

Diagram
=======

            Input Events            Counter             Output Events
                            ┌───────────────────┐
                start   ━━━▷│●                  │
                            │-------------------│
                            │                   │
  EV_SEND_INTER_EVENT ====▷ │                   │
                            │                   │
                            │-------------------│
                stop    ━━━▷│■                  │
                            │                   │
                            └───────────────────┘


Attributes
==========



Input Events
============

EV_SEND_INTER_EVENT
-------------------

Fields:

    * __inter_event__

        Type: INTER_EVENT

        Inter-event to send, previously created with inter_event_create().

    * __route__

        Type: int (route's handler)

        If you know the route you can directly send an inter-event through it.

    * __subscribe_event__

        Type: BOOL

        Subscribe to this event in external yuno's service.


    * __unsubscribe_event__

        Type: BOOL

        Unsubscribe to this event in external yuno's service.


    * __persistent_event__

        Type: BOOL

        True if you want store the inter-event and remove it only
        when it arrived his destiny
        NOTICE: see c_store.h for more tags


    * __event_priority__

        Type: int

        IEvent priority

    * __event_age__     TODO

        Type: int

        IEvent age

    * __event_max_retries__

        Type: int

        Maximum retries

    * __event_retries__

        Type: int

        Current retries


Output Events
=============


============================= end sphinx doc ============================= ^^*/

#pragma once

#include <ginsfsm.h>
#include "yuneta_version.h"
#include "msglog_yuneta.h"
#include "c_store.h"
#include "c_websocket.h"
#include "c_yuno.h"
#include "c_tcp_s0.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_router(void);

#define GCLASS_ROUTER_NAME  "Router"
#define GCLASS_ROUTER gclass_router()

/*
 *  To use this function you must be aware if the route is connected.
 *  (Subscribing to ON_OPEN_ROUTE/ON_CLOSE_ROUTE router's events)
 */
PUBLIC int send_inter_event_by_route(
    hsdata route_dst,   // WARNING route_dst must be connected
    hsdata inter_event  // WARNING: inter_event is decref
);


#define GET_ROUTER_ROUTE(kw) (hsdata)kw_get_int((kw), "__route__", 0, FALSE)


#ifdef __cplusplus
}
#endif
