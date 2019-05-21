/****************************************************************************
 *          C_TIMETRANSITION.H
 *          Timetransition GClass.
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_TIMETRANSITION_H
#define _C_TIMETRANSITION_H 1

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


#endif
