/****************************************************************************
 *          C_PERSISTENCE.H
 *          GClass of PERSITENCE g-object.
 *
 *          Copyright (c) 2013 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

/*

    Etiquetas usadas por el servicio persist

    __persistent_event__            bool, marca para hacer un ivent persistente
    __persistence_reference__       str, referencia ievent

    __event_priority__              int, prioridad ievent
    __event_age__                   int, edad máxima del ievent (TODO)

    // estos están repetidos por api_iev, router
    __event_max_retries__           int, máximo número de reintentos
    __event_retries__               int, número reitento actual

 */


#ifndef _C_PERSISTENCE_H
#define _C_PERSISTENCE_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "yuneta_environment.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_persistence(void);

#define GCLASS_PERSISTENCE_NAME  "Persist"
#define GCLASS_PERSISTENCE gclass_persistence()

/*********************************************************************
 *      Macros
 *********************************************************************/
#define IS_PERSISTENT_IEV(kw) kw_get_bool((kw), "__persistent_event__", 0, 0)
#define GET_PERSISTENT_REFERENCE(kw) kw_get_str((kw), "__persistence_reference__", 0, 0)


#ifdef __cplusplus
}
#endif


#endif

