/***********************************************************************
 *          C_RESOURCE2.C
 *          Resource2 GClass.
 *
 *          Resource Controler using flat files
 *
 *          Get persistent messages of a "resource" or "topic".
 *          Each resource/topic has a unique own flat file to save his message
 *
 *          If you want history then use queues, history, series/time.
 *
 *          Copyright (c) 2022 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "c_resource2.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int save_record(
    hgobj gobj,
    const char *resource,
    json_t *record // not owned
);
PRIVATE int delete_record(
    hgobj gobj,
    const char *resource
);

PRIVATE int create_persistent_resources(hgobj gobj);
PRIVATE int load_persistent_resources(hgobj gobj);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "json_desc",        SDF_RD,             0,              "C struct json_desc_t with the schema of records. Empty is no schema"),
SDATA (ASN_BOOLEAN,     "persistent",       SDF_WR|SDF_PERSIST, 0,              "Resources are persistent"),
SDATA (ASN_OCTET_STR,   "service",          SDF_RD,             "",             "Service name for global store, for example 'mqtt'"),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             0,              "Database name. Not empty to be persistent. Path is store/{service}/yuno_role_plus_name()/{database}/"),

SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messagess",       "Trace messages"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    json_t *db_resources;
    char path_database[PATH_MAX];
    const char *pkey;

    json_desc_t *json_desc;
    const char *service;
    const char *database;
    BOOL persistent;
} PRIVATE_DATA;




            /******************************
             *      Framework Methods
             ******************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    priv->db_resources = json_object();

    /*
     *  SERVICE subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(subscriber) {
        gobj_subscribe_event(gobj, NULL, NULL, subscriber);
    }

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(json_desc,             gobj_read_pointer_attr)
    SET_PRIV(persistent,            gobj_read_bool_attr)
    SET_PRIV(service,               gobj_read_str_attr)
    SET_PRIV(database,              gobj_read_str_attr)

    // WARNING duplicated code
    if(priv->persistent) {
        char path_service[NAME_MAX];
        build_path3(path_service, sizeof(path_service),
            priv->service,
            gobj_yuno_role_plus_name(),
            priv->database
        );
        yuneta_store_dir(
            priv->path_database, sizeof(priv->path_database), "resources", path_service, TRUE
        );
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(persistent,             gobj_read_bool_attr)
        // WARNING duplicated code
        if(priv->persistent) {
            char path_service[NAME_MAX];
            build_path3(path_service, sizeof(path_service),
                priv->service,
                gobj_yuno_role_plus_name(),
                priv->database
            );
            yuneta_store_dir(
                priv->path_database, sizeof(priv->path_database), "resources", path_service, TRUE
            );
        }
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    JSON_DECREF(priv->db_resources);
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->persistent) {
// TODO        create_persistent_resources(gobj); // IDEMPOTENTE.
//         load_persistent_resources(gobj);
    }

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    return 0;
}

/***************************************************************************
 *      Framework Method create_resource
 ***************************************************************************/
PRIVATE json_t *mt_create_resource(hgobj gobj, const char *resource, json_t *kw, json_t *jn_options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*----------------------------------*
     *  Free content or with schema
     *----------------------------------*/
    json_t *record = 0;
    if(priv->json_desc) {
        record = create_json_record(priv->json_desc);
        json_object_update_existing_new(record, kw); // kw owned
    } else {
        record = kw; // kw owned
    }

    /*------------------------------------------*
     *      Create resource
     *------------------------------------------*/
    json_t *jn_resource = kw_get_dict(priv->db_resources, resource, json_object(), KW_CREATE);
    json_object_update(jn_resource, record);

    /*------------------------------------------*
     *      Save if persistent
     *------------------------------------------*/
    if(priv->persistent) {
        save_record(gobj, resource, record);
    }

    JSON_DECREF(jn_options);
    return record;
}

/***************************************************************************
 *      Framework Method update_resource
 ***************************************************************************/
PRIVATE int mt_update_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,  // NO USED
    json_t *kw,  // NOT owned
    json_t *jn_options  // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->persistent) {
        save_record(gobj, resource, kw);
    }

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return 0;
}

/***************************************************************************
 *      Framework Method delete_resource
 ***************************************************************************/
PRIVATE int mt_delete_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,  // owned, can be a string or dict with 'id'
    json_t *jn_options // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *jn_resource = kw_get_dict(priv->db_resources, resource, 0, 0);
    if(!jn_resource) {
        log_error(LOG_OPT_TRACE_STACK,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "No resource found",
            "service",      "%s", priv->service,
            "database",     "%s", priv->database,
            "resource",     "%s", resource,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return 0;
    }

    delete_record(gobj, resource);

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return 0;
}

/***************************************************************************
 *      Framework Method list_resource
 *  'ids' has prevalence over other fields.
 *  If 'ids' exists then other fields are ignored to search resources.
 ***************************************************************************/
PRIVATE void *mt_list_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,  // owned
    json_t *jn_options  // owned
)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *list = json_array();
    if(jn_filter) {
    }

    // TODO

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return list;
}

/***************************************************************************
 *      Framework Method get_resource
 ***************************************************************************/
PRIVATE json_t *mt_get_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,  // owned, string 'id' or dict with 'id' field are used to find the node
    json_t *jn_options  // owned
)
{
//     BOOL free_return_iter;
//     dl_list_t *resource2_iter = get_resource2_iter(
//         gobj,
//         resource2,
//         0,
//         &free_return_iter
//     );
//
//     if(!resource2_iter || dl_size(resource2_iter)==0) {
//         if(free_return_iter) {
//             rc_free_iter(resource2_iter, TRUE, 0);
//         }
//         KW_DECREF(str_or_kw);
//         JSON_DECREF(jn_options);
//         return 0;
//     }
//     json_int_t id = 0;
//     if(json_is_string(str_or_kw)) {
//         id = atoll(json_string_value(str_or_kw));
//     } else {
//         id = kw_get_int(str_or_kw, "id", 0, KW_REQUIRED|KW_WILD_NUMBER);
//     }
//     hsdata hs_resource2; rc_instance_t *i_hs;
//     i_hs = rc_first_instance(resource2_iter, (rc_resource2_t **)&hs_resource2);
//     while(i_hs) {
//         json_int_t id_ = sdata_read_uint64(hs_resource2, "id");
//         if(id == id_) {
//             if(free_return_iter) {
//                 rc_free_iter(resource2_iter, TRUE, 0);
//             }
//             KW_DECREF(str_or_kw);
//             JSON_DECREF(jn_options);
//             return hs_resource2;
//         }
//         i_hs = rc_next_instance(i_hs, (rc_resource2_t **)&hs_resource2);
//     }
//
//     if(free_return_iter) {
//         rc_free_iter(resource2_iter, TRUE, 0);
//     }
//
    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return 0;
}




            /***************************
             *      Commands
             ***************************/




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_record(
    hgobj gobj,
    const char *resource,
    json_t *record // not owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), ""); // TODO

    char path[PATH_MAX];
    build_path2(path, sizeof(path), priv->path_database, resource);

    int ret = json_dump_file(
        record,
        filename,
        JSON_INDENT(4)
    );
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_JSON_ERROR,
            "msg",          "%s", "Cannot save record",
            "path",         "%s", path,
            NULL
        );
        return -1;
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int delete_record(
    hgobj gobj,
    const char *resource
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), "");

    char path[PATH_MAX];
    build_path2(path, sizeof(path), priv->path_database, resource);

    int ret = unlink(path);
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "Cannot delete record file",
            "path",         "%s", path,
            NULL
        );
        return -1;
    }
    return 0;
}




            /***************************
             *      Private Methods
             ***************************/




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_IDLE,
    NULL
};

PRIVATE FSM fsm = {
    input_events,
    output_events,
    state_names,
    states,
};

/***************************************************************************
 *              GClass
 ***************************************************************************/
/*---------------------------------------------*
 *              Local methods table
 *---------------------------------------------*/
PRIVATE LMETHOD lmt[] = {
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_RESOURCE2_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0,
        0, //mt_command,
        0, //mt_inject_event,
        mt_create_resource,
        mt_list_resource,
        mt_update_resource,
        mt_delete_resource,
        0, //mt_future21,
        0, //mt_future22,
        mt_get_resource,
        0, //mt_state_changed,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_authz_checker,
        0, //mt_future39,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_future44,
        0, //mt_unlink_nodes,
        0, //mt_topic_jtree,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_list_instances,
        0, //mt_node_tree,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_resource2(void)
{
    return &_gclass;
}
