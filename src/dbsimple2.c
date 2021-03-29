/***********************************************************************
 *          DBSIMPLE2.H
 *
 *          Simple db for persistent attributes with Timeranger
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <unistd.h>
#include "dbsimple2.h"

/***************************************************************
 *              Constants
 ***************************************************************/

/***************************************************************
 *              Structures
 ***************************************************************/

/***************************************************************
 *              Prototypes
 ***************************************************************/

/***************************************************************
 *              Data
 ***************************************************************/
PRIVATE topic_desc_t db_desc[] = {
    // Topic Name,          Pkey        System Flag     Tkey   Topic Json Desc
    {"persistent_attrs",    "id",       sf_string_key,  0,     0},
    {0}
};

PRIVATE BOOL initialized = FALSE;
PRIVATE json_t *tranger = 0;
PRIVATE json_t *persistent_attrs_list = 0;

/***************************************************************************
   Setup simple db for persistent attrs
   HACK Idempotent, return local-db tranger;
 ***************************************************************************/
PUBLIC void *dbattrs_startup(void)
{
    if(initialized) {
        return tranger;
    }

    /*------------------------------*
     *      Open Timeranger db
     *------------------------------*/
    char path[PATH_MAX];
    if(!yuneta_realm_dir(path, sizeof(path), "local-db", TRUE)) {
        return 0;
    }

    json_t *jn_tranger = json_pack("{s:s, s:s, s:b, s:i}",
        "path", path,
        "filename_mask", "%Y",
        "master", 1,
        "on_critical_error", (int)(LOG_NONE)
    );
    tranger = tranger_startup(jn_tranger);
    if(!tranger) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "Cannot open dbsimple2 tranger",
            "path",         "%s", path,
            NULL
        );
    }

    /*---------------------------*
     *  Open topics as messages
     *---------------------------*/
    trmsg_open_topics(
        tranger,
        db_desc
    );
    persistent_attrs_list = trmsg_open_list(
        tranger,
        "persistent_attrs",     // topic
        json_pack("{s:i}",      // filter
            "max_key_instances", 1
        )
    );

    if(persistent_attrs_list) {
        initialized = TRUE;
    }

    return tranger;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int dbattrs_load_persistent(hgobj gobj)
{
    if(!initialized) {
        return -1;
    }

    if(!gobj_is_unique(gobj)) {
        // silence, not unique not save
        return -1;
    }

    const char *id = gobj_full_name(gobj);
    json_t *jn_attrs = trmsg_get_active_message(persistent_attrs_list, id);
    if(jn_attrs) {
        return json2sdata(gobj_hsdata(gobj), jn_attrs, SDF_PERSIST, 0, 0);
    } else {
        return -1;
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int dbattrs_save_persistent(hgobj gobj)
{
    if(!initialized) {
        return -1;
    }

    const char *id = gobj_full_name(gobj);
    json_t *jn_attrs = sdata2json(gobj_hsdata(gobj), SDF_PERSIST, 0);
    json_object_set_new(jn_attrs, "id", json_string(id));

    return trmsg_add_instance(
        tranger,
        "persistent_attrs",
        jn_attrs,  // owned
        0,
        0
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int dbattrs_remove_persistent(hgobj gobj)
{
    if(!initialized) {
        return -1;
    }

// TODO cambia a treedb
    const char *id = gobj_full_name(gobj);
    json_t *jn_attrs = json_object();
    json_object_set_new(jn_attrs, "id", json_string(id));

    return trmsg_add_instance(
        tranger,
        "persistent_attrs",
        jn_attrs,  // owned
        0,
        0
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC json_t *dbattrs_list_persistent(hgobj gobj)
{
    if(!initialized) {
        return 0;
    }

    if(!gobj) {
        return trmsg_active_records(persistent_attrs_list, 0);
    }

    const char *id = gobj_full_name(gobj);
    json_t *jn_filter = json_object();
    json_object_set_new(jn_filter, "id", json_string(id));

    return trmsg_active_records(
        persistent_attrs_list,
        jn_filter
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void dbattrs_end(void)
{
    if(tranger) {
        tranger_shutdown(tranger);
        tranger = 0;
    }
    initialized = 0;
}
