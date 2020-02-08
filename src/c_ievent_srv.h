/****************************************************************************
 *          C_IEVENT_SRV.H
 *          Ievent_srv GClass.
 *
 *          Inter-event server side
 *
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#ifndef _C_IEVENT_SRV_H
#define _C_IEVENT_SRV_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_iogate.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Interface
 *********************************************************************/
/*
 *  Available subscriptions for ievent_srv's users
 */
#define I_IEVENT_SRV_SUBSCRIPTIONS    \
    {"EV_ON_SAMPLE1",               0,  0,  0}, \
    {"EV_ON_SAMPLE2",               0,  0,  0},


/**rst**
.. _ievent_srv-gclass:

**"Ievent_srv"** :ref:`GClass`
==============================

Inter-event server side

``GCLASS_IEVENT_SRV_NAME``
   Macro of the gclass string name, i.e **"iev_srv"**.

``GCLASS_IEVENT_SRV``
   Macro of the :func:`gclass_ievent_srv()` function.

**rst**/
PUBLIC GCLASS *gclass_ievent_srv(void);

#define GCLASS_IEVENT_SRV_NAME "IEvent_srv"
#define GCLASS_IEVENT_SRV gclass_ievent_srv()


#ifdef __cplusplus
}
#endif

#endif
