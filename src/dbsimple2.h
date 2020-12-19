/****************************************************************************
 *          TREEDBSIMPLE.H
 *
 *          Simple db for persistent attributes with Timeranger
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
   Setup simple db for persistent attrs
**rst**/
PUBLIC int dbattrs_startup(void);
PUBLIC void dbattrs_end(void);

/**rst**
   Load writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_load_persistent(hgobj gobj);

/**rst**
   Save writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_save_persistent(hgobj gobj);

/**rst**
   Remove writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_remove_persistent(hgobj gobj);

/**rst**
   List persistent attributes from simple tranger
**rst**/
PUBLIC json_t * dbattrs_list_persistent(void);


#ifdef __cplusplus
}
#endif