/****************************************************************************
 *          C_NODE.H
 *          Resource GClass.
 *
 *          Resources with treedb
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.

                        Sample resource schema
                        ======================


tb_resources    NOT recursive (no tree), all tables must be defined in this table
------------
    realms:         ASN_ITER,       SDF_RESOURCE
    binaries,       ASN_ITER,       SDF_RESOURCE
    configurations: ASN_ITER,       SDF_RESOURCE
    yunos:          ASN_ITER,       SDF_RESOURCE|SDF_PURECHILD
                                                            ▲
                                                            |
                        HACK PureChild: mark SDF_PURECHILD in own table and in parent's field!
                                                            | and mark SDF_PARENTID the own field pointing to parent
                        ┌───────────────┐                   |     |
                        │     realms    │                   |     |
                        └───────────────┘                   |     |
                                ▲ n (dl 'yunos')            |     |
                                ┃                           |     |
                                ┃                           |     |
                                ▼ 1 ('realm_id')            |     |
                ┌───────────────────────────────────────┐   |     |
                │               yunos                   │   |     |
                └───────────────────────────────────────┘   |     |
                        ▲ 1 ('binary_id')       ▲ n (dl 'configurations')
                        ┃                       ┃           |     |
                        ┃                       ┃           |     |
                        ▼ n (dl 'yunos')        ▼ n (dl 'yunos')  |
                ┌────────────────┐      ┌────────────────┐  |     |
                │   binaries     │      │ configurations │  |     |
                └────────────────┘      └────────────────┘  |     |
                                                            |     |
Realms                                                      |     |
------                                                      |     |
    id:             ASN_COUNTER64,  SDF_PERSIST|SDF_PKEY    ▼     |
    yunos:          ASN_ITER,       SDF_RESOURCE|SDF_PURECHILD,   |     "yunos"
                                                                  |
Yunos                                                             |
-----                                                             |
    id:             ASN_COUNTER64,  SDF_PERSIST|SDF_PKEY          ▼
    realm_id:       ASN_COUNTER64,  SDF_PERSIST|SDF_PARENTID,           "realms"
    binary_id:      ASN_COUNTER64,  SDF_PERSIST,                        "binaries"
    config_ids:     ASN_ITER,       SDF_RESOURCE,                       "configurations"

Binaries
--------
    id:             ASN_COUNTER64,  SDF_PERSIST|SDF_PKEY

configurations
--------------
    id:             ASN_COUNTER64,  SDF_PERSIST|SDF_PKEY



 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "yuneta_environment.h"
#include "c_timer.h"
#include "c_yuno.h"
#include "msglog_yuneta.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_NODE_NAME "Node"
#define GCLASS_NODE gclass_node()


/***************************************************************
 *              Structures
 ***************************************************************/

/*
 *
 *  HACK All data conversion must be going through json.
 *  SData and others are peripherals, leaf nodes. Json is a through-node.
 *
 *  Here the edges are the conversion algorithms.
 *  In others are the communication links.
 *
 *  Is true "What is above, is below. What is below, is above"?
 *
 */

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_node(void);


#ifdef __cplusplus
}
#endif
