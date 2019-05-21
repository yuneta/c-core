/****************************************************************************
 *          C_RSTATS.H
 *          Rstats GClass.
 *
 *          Read Statistics Files uv-mixin
 *
 *          Copyright (c) 2019 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_timer.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_RSTATS_NAME "Rstats"
#define GCLASS_RSTATS gclass_rstats()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_rstats(void);

#ifdef __cplusplus
}
#endif
