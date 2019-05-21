/****************************************************************************
 *          DBSIMPLE.H
 *
 *          Simple DB file for persistent attributes
 *
 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef __DBSIMPLE_H
#define __DBSIMPLE_H 1

#include "yuneta_environment.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************
 *              Prototypes
 **************************************************/
/**rst**
   Load writable persistent attributes from simple file db
**rst**/
PUBLIC int db_load_persistent_attrs(hgobj gobj);

/**rst**
   Save writable persistent attributes from simple file db
**rst**/
PUBLIC int db_save_persistent_attrs(hgobj gobj);

/**rst**
   Remove writable persistent attributes from simple file db
**rst**/
PUBLIC int db_remove_persistent_attrs(hgobj gobj);

/**rst**
   List persistent attributes from simple file db
**rst**/
PUBLIC json_t * db_list_persistent_attrs(void);

#ifdef __cplusplus
}
#endif

#endif
