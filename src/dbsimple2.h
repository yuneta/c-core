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
   HACK Idempotent, return local-db tranger;
**rst**/
PUBLIC void *dbattrs_startup(void);
PUBLIC void dbattrs_end(void);

/**rst**
   Load writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_load_persistent(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   Save writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_save_persistent(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   Remove writable persistent attributes from simple tranger
**rst**/
PUBLIC int dbattrs_remove_persistent(
    hgobj gobj,
    json_t *jn_attrs  // owned
);

/**rst**
   List persistent attributes from simple tranger
**rst**/
PUBLIC json_t * dbattrs_list_persistent(
    hgobj gobj,
    json_t *jn_attrs  // owned
);


#ifdef __cplusplus
}
#endif
