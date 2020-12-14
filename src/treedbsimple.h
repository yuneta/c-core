/****************************************************************************
 *          TREEDBSIMPLE.H
 *
 *          Simple TreeDB file for persistent attributes
 *
 *          Copyright (c) 2020 Niyamaka.
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
   Load writable persistent attributes from simple treedb
**rst**/
PUBLIC int treedb_load_persistent_attrs(hgobj gobj);

/**rst**
   Save writable persistent attributes from simple treedb
**rst**/
PUBLIC int treedb_save_persistent_attrs(hgobj gobj);

/**rst**
   Remove writable persistent attributes from simple treedb
**rst**/
PUBLIC int treedb_remove_persistent_attrs(hgobj gobj);

/**rst**
   List persistent attributes from simple treedb
**rst**/
PUBLIC json_t * treedb_list_persistent_attrs(void);


#ifdef __cplusplus
}
#endif
