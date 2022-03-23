/***********************************************************************
 *          C_RESOURCE2.C
 *          Resource2 GClass.
 *
 *          Resource Controler using flat files
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
    const char *pkey,
    json_t *record // not owned
);
PRIVATE int delete_record(
    hgobj gobj,
    const char *resource,
    const char *pkey
);

PRIVATE int create_persistent_resources(hgobj gobj);
PRIVATE int load_persistent_resources(hgobj gobj);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "json_desc",        SDF_RD,             0,              "C struct json_desc_t with the schema of records. Empty is no schema"),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             0,              "Database name. Not empty to be persistent. Path is store/{service}/yuno_role_plus_name()/{database}/"),
SDATA (ASN_JSON,        "properties",       SDF_RD,             0,              "Json with database properties. Specific of each database."),
SDATA (ASN_OCTET_STR,   "service",          SDF_RD,             "",             "Service name for global store, for example 'mqtt'"),
SDATA (ASN_OCTET_STR,   "pkey",             SDF_RD,             "id",           "Primary key of records"),

SDATA (ASN_POINTER,     "kw_match",         0,                  kw_match_simple,"kw_match default function"),

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
    BOOL persistent;
    const char *pkey;

    json_desc_t *json_desc;
    kw_match_fn kw_match;
    const char *database;
    const char *service;
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
    SET_PRIV(kw_match,              gobj_read_pointer_attr)
    SET_PRIV(json_desc,             gobj_read_pointer_attr)
    SET_PRIV(database,              gobj_read_str_attr)
    SET_PRIV(service,               gobj_read_str_attr)
    SET_PRIV(pkey,                  gobj_read_str_attr)

    if(!empty_string(priv->database)) {
        char path_service[NAME_MAX];
        build_path3(path_service, sizeof(path_service),
            priv->service,
            gobj_yuno_role_plus_name(),
            priv->database
        );
        yuneta_store_dir(
            priv->path_database, sizeof(priv->path_database), "resources", path_service, TRUE
        );
        priv->persistent = TRUE;
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    //PRIVATE_DATA *priv = gobj_priv_data(gobj);

    //IF_EQ_SET_PRIV(timeout,             gobj_read_int32_attr)
    //END_EQ_SET_PRIV()
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
        create_persistent_resources(gobj); // IDEMPOTENTE.
        load_persistent_resources(gobj);
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
 *      Options: pkey, ignore_pkey
 *          (pkey="" and ignore_pkey=TRUE) for add to anonymous list
 ***************************************************************************/
PRIVATE json_t *mt_create_resource(hgobj gobj, const char *resource, json_t *kw, json_t *jn_options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *record = 0;
    if(priv->json_desc) {
        record = create_json_record(priv->json_desc);
        json_object_update_existing_new(record, kw); // kw owned
    } else {
        record = kw; // kw owned
    }

    const char *force_pkey = kw_get_str(jn_options, "pkey", 0, 0);
    const char *pkey = kw_get_str(record, force_pkey?force_pkey:priv->pkey, "", 0);
    if(empty_string(pkey)) {
        if(!kw_get_bool(jn_options, "ignore_pkey", 0, 0)) {
            log_error(LOG_OPT_TRACE_STACK,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "No pkey field in record (ignore_pkey false)",
                "service",      "%s", priv->service,
                "database",     "%s", priv->database,
                "resource",     "%s", resource,
                NULL
            );
            log_debug_json(0, record, "no pkey");
            JSON_DECREF(jn_options);
            JSON_DECREF(record);
            return 0;
        }
    }

    json_t *jn_resource = kw_get_dict(priv->db_resources, resource, json_object(), KW_CREATE);

    if(empty_string(pkey)) {
        json_object_update(jn_resource, record);
    } else {
        if(kw_has_key(jn_resource, pkey)) {
            if(!kw_get_bool(jn_options, "force", 0, 0)) {
                log_error(LOG_OPT_TRACE_STACK,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "pkey already exists",
                    "service",      "%s", priv->service,
                    "database",     "%s", priv->database,
                    "resource",     "%s", resource,
                    NULL
                );
                log_debug_json(0, record, "No pkey field in record (force false)");
                JSON_DECREF(jn_options);
                JSON_DECREF(record);
                return 0;
            }
        }
        json_object_set_new(jn_resource, pkey, record);
    }

    if(priv->persistent) {
        save_record(gobj, resource, pkey, record);
    }

    JSON_DECREF(jn_options);
    return record;
}

/***************************************************************************
 *      Framework Method update_resource
 *      jn_options: create, pkey, ignore_pkey
 *          (pkey="" and ignore_pkey=TRUE) for add to anonymous list
 *      jn_filter: pkey
 ***************************************************************************/
PRIVATE json_t *mt_update_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,  // owned, can be a string or dict with 'id'
    json_t *kw,  // owned, 'id' field are used to find the node if jn_filter is null
    json_t *jn_options  // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL create = kw_get_bool(jn_options, "create", 0, 0);
    const char *force_pkey = kw_get_str(jn_options, "pkey", 0, 0);
    const char *pkey;
    if(json_is_string(jn_filter)) {
        pkey = json_string_value(jn_filter);
    } else {
        pkey = kw_get_str(jn_filter, force_pkey?force_pkey:priv->pkey, "", 0);
    }

    if(empty_string(pkey)) {
        pkey = kw_get_str(kw, force_pkey?force_pkey:priv->pkey, "", 0);
        if(empty_string(pkey)) {
            if(!kw_get_bool(jn_options, "ignore_pkey", 0, 0)) {
                log_error(LOG_OPT_TRACE_STACK,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "No pkey field in record (ignore_pkey false)",
                    "service",      "%s", priv->service,
                    "database",     "%s", priv->database,
                    "resource",     "%s", resource,
                    NULL
                );
                log_debug_json(0, kw, "no pkey");
                JSON_DECREF(jn_filter);
                JSON_DECREF(kw);
                JSON_DECREF(jn_options);
                return 0;
            }
        }
    }

    json_t *jn_resource = kw_get_dict(
        priv->db_resources, resource, create?json_object():0, create?KW_CREATE:0
    );
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
        JSON_DECREF(kw);
        JSON_DECREF(jn_options);
        return 0;
    }

    json_t *record;
    if(empty_string(pkey)) {
        record = jn_resource;
    } else {
        record = kw_get_dict(jn_resource, pkey, create?json_object():0, create?KW_CREATE:0);
    }
    if(!record) {
        log_error(LOG_OPT_TRACE_STACK,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Resource record not found",
            "service",      "%s", priv->service,
            "database",     "%s", priv->database,
            "resource",     "%s", resource,
            "pkey",         "%s", pkey?pkey:"",
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(kw);
        JSON_DECREF(jn_options);
        return 0;
    }

    if(priv->json_desc) {
        json_t *new_record = create_json_record(priv->json_desc);
        json_object_update_existing_new(new_record, kw); // kw owned
        json_object_update_existing_new(record, new_record); // new_record owned
    } else {
        json_object_update_existing_new(record, kw); // record owned
    }

    if(priv->persistent) {
        save_record(gobj, resource, pkey, record);
    }

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return record;
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

    const char *force_pkey = kw_get_str(jn_options, "pkey", 0, 0);
    const char *pkey;
    if(json_is_string(jn_filter)) {
        pkey = json_string_value(jn_filter);
    } else {
        pkey = kw_get_str(jn_filter, force_pkey?force_pkey:priv->pkey, "", 0);
    }

    if(empty_string(pkey)) {
        if(!kw_get_bool(jn_options, "ignore_pkey", 0, 0)) {
            log_error(LOG_OPT_TRACE_STACK,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "No pkey field in record (ignore_pkey false)",
                "service",      "%s", priv->service,
                "database",     "%s", priv->database,
                "resource",     "%s", resource,
                NULL
            );
            log_debug_json(0, jn_filter, "no pkey");
            JSON_DECREF(jn_filter);
            JSON_DECREF(jn_options);
            return -1;
        }
    }

    json_t *jn_resource = kw_get_dict(priv->db_resources, resource, json_object(), 0);
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

    json_t *record;
    if(empty_string(pkey)) {
        record = jn_resource;
    } else {
        record = kw_get_dict(jn_resource, pkey, 0, 0);
    }
    if(!record) {
        log_error(LOG_OPT_TRACE_STACK,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Resource record not found",
            "service",      "%s", priv->service,
            "database",     "%s", priv->database,
            "resource",     "%s", resource,
            "pkey",         "%s", pkey?pkey:"",
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return 0;
    }

    delete_record(gobj, resource, pkey);

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
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

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
    BOOL free_return_iter;
    dl_list_t *resource2_iter = get_resource2_iter(
        gobj,
        resource2,
        0,
        &free_return_iter
    );

    if(!resource2_iter || dl_size(resource2_iter)==0) {
        if(free_return_iter) {
            rc_free_iter(resource2_iter, TRUE, 0);
        }
        KW_DECREF(str_or_kw);
        JSON_DECREF(jn_options);
        return 0;
    }
    json_int_t id = 0;
    if(json_is_string(str_or_kw)) {
        id = atoll(json_string_value(str_or_kw));
    } else {
        id = kw_get_int(str_or_kw, "id", 0, KW_REQUIRED|KW_WILD_NUMBER);
    }
    hsdata hs_resource2; rc_instance_t *i_hs;
    i_hs = rc_first_instance(resource2_iter, (rc_resource2_t **)&hs_resource2);
    while(i_hs) {
        json_int_t id_ = sdata_read_uint64(hs_resource2, "id");
        if(id == id_) {
            if(free_return_iter) {
                rc_free_iter(resource2_iter, TRUE, 0);
            }
            KW_DECREF(str_or_kw);
            JSON_DECREF(jn_options);
            return hs_resource2;
        }
        i_hs = rc_next_instance(i_hs, (rc_resource2_t **)&hs_resource2);
    }

    if(free_return_iter) {
        rc_free_iter(resource2_iter, TRUE, 0);
    }

    KW_DECREF(str_or_kw);
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
PRIVATE json_t *load_commands(
    const char *GpsId
)
{
    char filename[NAME_MAX];
    get_command_filename(filename, sizeof(filename), GpsId);

    size_t flags = 0;
    json_error_t error;
    return json_load_file(filename, flags, &error);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_record(
    hgobj gobj,
    const char *resource,
    const char *pkey,
    json_t *record // not owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), "");

    char path[PATH_MAX];
    build_path3(path, sizeof(path), priv->path_database, resource, pkey);

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
    const char *resource,
    const char *pkey
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), "");

    char path[PATH_MAX];
    build_path3(path, sizeof(path), priv->path_database, resource, pkey);

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




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int print_required_attr_not_found(void *user_data, const char *attr)
{
    hsdata hs = user_data;
    log_error(0,
        "gobj",         "%s", "Resource2 GCLASS",
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_PARAMETER_ERROR,
        "msg",          "%s", "Required resource2 attribute NOT FOUND",
        "resource2",     "%s", sdata_resource2(hs),
        "attr",         "%s", attr,
        NULL
    );
    return 0;
}

/***************************************************************************
 *  ATTR: callback for sdata
 ***************************************************************************/
PRIVATE int on_write_it_cb(void *user_data, const char *name)
{
    hgobj gobj = user_data;
    if(gobj) {
        // TODO publish_event?
    }

    return 0;
}

/***************************************************************************
 *  Get the iter of `resource2`
 ***************************************************************************/
PRIVATE dl_list_t *get_resource2_iter(
    hgobj gobj,
    const char* resource2,
    json_int_t parent_id,
    BOOL *free_return_iter)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    *free_return_iter = FALSE;

    sdata_flag_t resource2_flag;
    const sdata_desc_t *schema = resource2_schema(priv->tb_resources, resource2, &resource2_flag);
    if(!schema) {
        // Error already logged
        return 0;
    }

    /*
     *  Root table
     */
    return sdata_read_iter(priv->db_resources, resource2);
}

/***************************************************************************
 *  Return [""], a list of strings with the names of resources:
 ***************************************************************************/
PRIVATE json_t * resources_list(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const sdata_desc_t *it = priv->tb_resources;
    json_t *jn_list = json_array();
    if(!it) {
        return jn_list;
    }
    while(it->name) {
        if(it->flag & SDF_RESOURCE2) {
            json_array_append_new(jn_list, json_string(it->name));
        }
        it++;
    }
    return jn_list;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int create_persistent_resources(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t * jn_resources = resources_list(gobj);
    size_t index;
    json_t *jn_resource2;
    json_array_foreach(jn_resources, index, jn_resource2) {
        const char *resource2 = json_string_value(jn_resource2);
        sdata_flag_t resource2_flag;
        const sdata_desc_t *items = resource2_schema(priv->tb_resources, resource2, &resource2_flag);
        if(!items) {
            // Error already logged
            continue;
        }
        const char *key;
        json_t *kw_fields = schema2json(items, &key);
        priv->dba->dba_create_resource2(gobj, priv->pDb, resource2, key, kw_fields);
    }
    JSON_DECREF(jn_resources);
    return 0;
}

/***************************************************************************
 *  Callback to load root resources or purechilds
 ***************************************************************************/
PRIVATE int dba_load_record_cb(
    hgobj gobj,
    const char *resource2,
    void *user_data,
    json_t *kw  // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    dl_list_t *resource2_iter = user_data;

    sdata_flag_t resource2_flag;
    const sdata_desc_t *schema = resource2_schema(priv->tb_resources, resource2, &resource2_flag);

    hsdata hs = sdata_create(schema, gobj, on_write_it_cb, 0, 0, resource2);
    if(!hs)  {
        KW_DECREF(kw);
        return 0; // Don't append record
    }

    /*
     *  Convert json in hsdata
     */
    json2sdata(hs, kw, SDF_PERSIST|SDF_VOLATIL, 0, 0);

    /*
     *  Check required fields
     */
    if(!sdata_check_required_attrs(hs, print_required_attr_not_found, hs)) {
        sdata_destroy(hs);
        KW_DECREF(kw);
        return 0; // Don't append record
    }

    rc_add_instance(resource2_iter, hs, 0);

    KW_DECREF(kw);
    // Return 1 to append, 0 to ignore, -1 to break the load.
    return 0; // Don't append record
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int load_persistent_resources(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*--------------------------------*
     *      Load root tables
     *--------------------------------*/
    json_t * jn_resources = resources_list(gobj);
    size_t index;
    json_t *jn_resource2;
    json_array_foreach(jn_resources, index, jn_resource2) {
        const char *resource2 = json_string_value(jn_resource2);
        sdata_flag_t resource2_flag;
        const sdata_desc_t *schema = resource2_schema(priv->tb_resources, resource2, &resource2_flag);
        if(!schema) {
            // Error already logged
            continue;
        }
        /*
         *  root resource2 iter
         */
        dl_list_t *resource2_iter = sdata_read_iter(priv->db_resources, resource2);

        json_t *kw_filtro = json_object(); //  WARNING load all records!

        json_t *jn_list = priv->dba->dba_load_table(
            gobj,
            priv->pDb,
            resource2,
            resource2,
            resource2_iter,
            kw_filtro,
            dba_load_record_cb,
            0
        );
        JSON_DECREF(jn_list); // we load records in callback, ignore return
    }

    JSON_DECREF(jn_resources);

    return 0;
}




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
    {"resource2_webix_schema",   resource2_webix_schema,      0},
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
        mt_get_resource2,
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
