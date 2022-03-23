/****************************************************************************
 *          C_RESOURCE2.H
 *          Resource2 GClass.
 *
 *          Resource Controler using flat files
 *
 *          Copyright (c) 2022 Niyamaka.
 *          All Rights Reserved.
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
/*
 *  Resources implemented with flat files
 */
#define GCLASS_RESOURCE2_NAME "Resource2"
#define GCLASS_RESOURCE2 gclass_resource2()


/***************************************************************
 *              Structures
 ***************************************************************/
/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_resource2(void);

#ifdef __cplusplus
}
#endif
