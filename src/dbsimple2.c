/***********************************************************************
 *          DBSIMPLE2.H
 *
 *          Simple db for persistent attributes with Timeranger
 *          // TODO cambia a treedb???
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
PUBLIC int dbattrs_load_persistent(hgobj gobj, json_t *jn_attrs_)
{
    if(!initialized) {
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    if(!gobj_is_unique(gobj)) {
        // silence, not unique not save
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    const char *id = gobj_full_name(gobj);
    json_t *msg = trmsg_get_active_message(persistent_attrs_list, id);
    if(msg) {
        json_t *attrs = kw_clone_by_keys(
            json_incref(msg),     // owned
            jn_attrs_,   // owned
            FALSE
        );

        int ret = json2sdata(
            gobj_hsdata(gobj),
            attrs,   // Not owned
            SDF_PERSIST,
            0,
            0
        );
        JSON_DECREF(attrs);
        return ret;

    } else {
        JSON_DECREF(jn_attrs_);
        return -1;
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int dbattrs_save_persistent(hgobj gobj, json_t *jn_attrs_)
{
    if(!initialized) {
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    if(!gobj_is_unique(gobj)) {
        // silence, not unique not save
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    const char *id = gobj_full_name(gobj);
    json_t *jn_attrs = sdata2json(gobj_hsdata(gobj), SDF_PERSIST, 0);

    json_t *attrs = kw_clone_by_keys(
        jn_attrs,   // owned
        jn_attrs_,  // owned
        FALSE
    );

    json_object_set_new(attrs, "id", json_string(id));

    json_t *msg = trmsg_get_active_message(persistent_attrs_list, id);
    if(msg) {
        json_object_update_missing(attrs, msg);
    }

    // TODO elimina las keys que ya no sean persistent attrs

    return trmsg_add_instance(
        tranger,
        "persistent_attrs",
        attrs,  // owned
        0,
        0
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int dbattrs_remove_persistent(hgobj gobj, json_t *jn_attrs_)
{
    if(!initialized) {
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    if(!gobj_is_unique(gobj)) {
        // silence, not unique not save
        JSON_DECREF(jn_attrs_);
        return -1;
    }

    const char *id = gobj_full_name(gobj);

    json_t *msg = trmsg_get_active_message(persistent_attrs_list, id);
    if(!msg) {
        JSON_DECREF(jn_attrs_);
        return -1;
    }
    json_t *attrs = kw_clone_by_not_keys(
        json_incref(msg),   // owned
        jn_attrs_,  // owned
        FALSE
    );
    json_object_set_new(attrs, "id", json_string(id));

    return trmsg_add_instance(
        tranger,
        "persistent_attrs",
        attrs,  // owned
        0,
        0
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC json_t *dbattrs_list_persistent(hgobj gobj, json_t *jn_attrs_)
{
    if(!initialized) {
        JSON_DECREF(jn_attrs_);
        return 0;
    }

    json_t *jn_filter = 0;

    if(gobj) {
        const char *id = gobj_full_name(gobj);
        jn_filter = json_object();
        json_object_set_new(jn_filter, "id", json_string(id));
    }

    json_t *jn_result = json_object();
    json_object_set_new(jn_result, "yuno", json_string(gobj_short_name(gobj_yuno())));
    json_t *new_list = kw_get_list(jn_result, "persistent_attrs", json_array(), KW_CREATE);
    json_t *list = trmsg_active_records(persistent_attrs_list, jn_filter);
    int idx; json_t *record;
    json_array_foreach(list, idx, record) {
        json_t *new_record = kw_clone_by_keys(json_incref(record), json_incref(jn_attrs_), FALSE);
        json_object_set_new(
            new_record, "id", json_string(kw_get_str(record, "id", "", KW_REQUIRED))
        );
        json_array_append_new(new_list, new_record);
    }
    JSON_DECREF(jn_attrs_);
    JSON_DECREF(list);
    return jn_result;
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
