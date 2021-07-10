/****************************************************************************
 *          DBSIMPLE.H
 *
 *          Simple DB file for persistent attributes
 *
 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include "yuneta_environment.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************
 *              Prototypes
 **************************************************/
/**rst**
   Load writable persistent attributes from simple file db
   WARNING jn_attrs not used
**rst**/
PUBLIC int db_load_persistent_attrs(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   Save writable persistent attributes from simple file db
   WARNING jn_attrs not used
**rst**/
PUBLIC int db_save_persistent_attrs(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   Remove writable persistent attributes from simple file db
   WARNING jn_attrs not used
**rst**/
PUBLIC int db_remove_persistent_attrs(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   List persistent attributes from simple file db
   WARNING jn_attrs not used
**rst**/
PUBLIC json_t *db_list_persistent_attrs(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

#ifdef __cplusplus
}
#endif
