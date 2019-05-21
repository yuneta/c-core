/****************************************************************************
 *          C_STORE.H
 *          Store GClass.
 *
 *          Store service
 *
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#ifndef _C_STORE_H
#define _C_STORE_H 1

#include <ginsfsm.h>
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Interface
 *********************************************************************/
/*
 *  Available subscriptions for store's users
 */
#define I_STORE_SUBSCRIPTIONS    \
    {"EV_WRITE_RESOURCE_ACK",   0, 0, 0, 0}, \
    {"EV_READ_RESOURCE_ACK",    0, 0, 0, 0}, \
    {"EV_DELETE_RESOURCE_ACK",  0, 0, 0, 0},


/**rst**
.. _store-gclass:

**"Store"** :ref:`GClass`
================================

Store service

``GCLASS_STORE_NAME``
   Macro of the gclass string name, i.e **"Store"**.

``GCLASS_STORE``
   Macro of the :func:`gclass_store()` function.

**rst**/
PUBLIC GCLASS *gclass_store(void);

#define GCLASS_STORE_NAME "Store"
#define GCLASS_STORE gclass_store()


#ifdef __cplusplus
}
#endif

#endif
