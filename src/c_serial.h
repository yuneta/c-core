/****************************************************************************
 *          C_SERIAL.H
 *          Serial GClass.
 *
 *          Manage Serial Ports
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "yuneta_environment.h"
#include "c_timer.h"
#include "msglog_yuneta.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_SERIAL_NAME "Serial"
#define GCLASS_SERIAL gclass_serial()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_serial(void);

#ifdef __cplusplus
}
#endif
