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
PRIVATE json_t * get_old_persistent_attrs(hgobj gobj); // TODO remove when migration done
PRIVATE int remove_old_persistent_attrs(hgobj gobj); // TODO remove when migration done

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
 *
 ***************************************************************************/
PUBLIC int dbattrs_startup(void)
{
    if(initialized) {
        return 0;
    }

    /*------------------------------*
     *      Open Timeranger db
     *------------------------------*/
    char path[PATH_MAX];
    if(!yuneta_realm_dir(path, sizeof(path), "local-db", TRUE)) {
        return -1;
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

    return 0;
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

    // TODO load old persistent attrs
    json_t *old_attrs = get_old_persistent_attrs(gobj);
    if(old_attrs) {
        const char *id = gobj_full_name(gobj);
        json_object_set_new(old_attrs, "id", json_string(id));
        if(trmsg_add_instance(
            tranger,
            "persistent_attrs",
            old_attrs,  // owned
            0,
            0
        )==0) {
            remove_old_persistent_attrs(gobj);
        }
    }

    const char *id = gobj_full_name(gobj);
    json_t *jn_attrs = trmsg_get_message(persistent_attrs_list, id);
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
PUBLIC json_t *dbattrs_list_persistent(void)
{
    if(!initialized) {
        return 0;
    }

    return trmsg_active_records(persistent_attrs_list, 0);
}


/***************************************************************************
 *  TODO remove when migration done
 ***************************************************************************/
PRIVATE char *get_persistent_sdata_full_filename(
    hgobj gobj,
    char *bf,
    int bflen,
    const char *label
)
{
    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), "%s-%s-%s.json",
        gobj_gclass_name(gobj),
        gobj_name(gobj),
        label
    );

    return yuneta_realm_file(bf, bflen, "data", filename, FALSE);
}

PRIVATE int remove_old_persistent_attrs(hgobj gobj)
{
    char path[PATH_MAX];
    get_persistent_sdata_full_filename(gobj, path, sizeof(path), "persistent-attrs");
    if(access(path, 0)==0) {
        if(unlink(path)<0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "unlink() FAILED",
                "path",         "%s", path,
                "errno",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            return -1;
        }
    }
    return 0;
}

PRIVATE json_t * get_old_persistent_attrs(hgobj gobj)
{
    char path[PATH_MAX];
    get_persistent_sdata_full_filename(gobj, path, sizeof(path), "persistent-attrs");
    if(empty_string(path) || access(path, 0)!=0) {
        // No persistent attrs saved
        return 0;
    }

    size_t flags = 0;
    json_error_t error;
    json_t *jn_dict = json_load_file(path, flags, &error);
    if(!jn_dict) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_JSON_ERROR,
            "msg",          "%s", "JSON data INVALID",
            "line",         "%d", error.line,
            "column",       "%d", error.column,
            "position",     "%d", error.position,
            "json",         "%s", error.text,
            "path",         "%s", path,
            NULL
        );
        return 0;
    }

    return jn_dict;
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
