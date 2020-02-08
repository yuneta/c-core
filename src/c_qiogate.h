/****************************************************************************
 *          C_QIOGATE.H
 *          Qiogate GClass.
 *
 *          IOGate with persistent queue
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_iogate.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_QIOGATE_NAME "QIOGate"
#define GCLASS_QIOGATE gclass_qiogate()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_qiogate(void);

#ifdef __cplusplus
}
#endif
