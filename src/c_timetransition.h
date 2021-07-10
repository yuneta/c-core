/****************************************************************************
 *          C_TIMETRANSITION.H
 *          Timetransition GClass.
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_timetransition(void);

#define GCLASS_TIMETRANSITION_NAME "Timetransition"
#define GCLASS_TIMETRANSITION gclass_timetransition()

#ifdef __cplusplus
}
#endif
