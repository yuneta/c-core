/***********************************************************************
 *          C_RESOURCE.C
 *          Resource GClass.
 *
 *          Resource Controler

Resources will be out of ginsfsm. It owns to yuneta.
And, the parameters of commands will be defined with sdata!

Commands are managed with SData.
Then Resources commands acts like others command.


 *          Copyright (c) 2016-2022 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "c_resource.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int print_required_attr_not_found(void *user_data, const char *attr);
PRIVATE int on_write_it_cb(void *user_data, const char *name);

PRIVATE dl_list_t *get_resource_iter(
    hgobj gobj,
    const char *resource,
    json_int_t parent_id,
    BOOL *free_return_iter
);

/*
 *  resources_list():
 *      Return [""], a list of strings with the names of resources:
 */
PRIVATE json_t * resources_list(hgobj gobj);

PRIVATE int create_persistent_resources(hgobj gobj);
PRIVATE int load_persistent_resources(hgobj gobj);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tb_resources",     SDF_REQUIRED,       0,              "Table with resource's schemas."),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             0,              "Database name. Not empty to be persistent."),
SDATA (ASN_JSON,        "properties",       SDF_RD,             0,              "Json with database properties. Specific of each database."),
SDATA (ASN_POINTER,     "dba",              0,                  0,              "Table with persistent dba functions. Not empty to be persistent."),
SDATA (ASN_OCTET_STR,   "service",          SDF_RD,             "",             "Service name for global store"),
SDATA (ASN_BOOLEAN,     "local_store",      SDF_RD,             0,              "True for local store (not clonable)"),

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
    TRACE_SQL = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"sql",        "Trace sql sentences"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    sdata_desc_t *tb_resources;
    dba_persistent_t *dba;
    kw_match_fn kw_match;
    void *pDb;

    hsdata hs_resources;
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

    priv->kw_match = gobj_read_pointer_attr(gobj, "kw_match");
    priv->tb_resources = gobj_read_pointer_attr(gobj, "tb_resources");
    priv->dba = gobj_read_pointer_attr(gobj, "dba");
    priv->hs_resources = sdata_create(priv->tb_resources, gobj, on_write_it_cb, 0, 0, 0);

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
    //SET_PRIV(timeout,               gobj_read_int32_attr)
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
    if(priv->hs_resources) {
        sdata_desc_t *it = priv->tb_resources;
        while(it->name) {
            dl_list_t *dl_iter = sdata_read_iter(priv->hs_resources, it->name);
            // TODO review pure childs
            rc_free_iter(dl_iter, FALSE, sdata_destroy);
            it++;
        }
        sdata_destroy(priv->hs_resources);
        priv->hs_resources = 0;
    }
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->dba) {
        const char *database_ = gobj_read_str_attr(gobj, "database");
        if(!empty_string(database_)) {

            BOOL local_store = gobj_read_bool_attr(gobj, "local_store");
            char database[NAME_MAX];
            if(local_store) {
                yuneta_realm_file(database, sizeof(database), "store", database_, TRUE);
            } else {
                const char *service = gobj_read_str_attr(gobj, "service");
                yuneta_store_file(database, sizeof(database), "resources", service, database_, TRUE);
            }

            json_t *jn_properties = gobj_read_json_attr(gobj, "properties");
            priv->pDb = priv->dba->dba_open(gobj, database, json_incref(jn_properties));
            if(priv->pDb) {
                create_persistent_resources(gobj); // IDEMPOTENTE.
                load_persistent_resources(gobj);
            }
        } else {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "database name EMPTY",
                NULL
            );
        }
    }

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->pDb) {
        priv->dba->dba_close(gobj, priv->pDb);
        priv->pDb = 0;
    }
    return 0;
}

/***************************************************************************
 *      Framework Method create_resource
 ***************************************************************************/
PRIVATE json_t *mt_create_resource(hgobj gobj, const char *resource, json_t *kw, json_t *jn_options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);

    BOOL free_return_iter; // Must be always FALSE, because parent_id is used.
    dl_list_t *resource_iter = get_resource_iter(
        gobj,
        resource,
        0, // parent_id TODO
        &free_return_iter
    );
    if(!resource_iter) {
        KW_DECREF(kw);
        JSON_DECREF(jn_options);
        return (hsdata)0;
    }
    if(free_return_iter) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "free_return_iter MUST be FALSE!",
            "resource",     "%s", resource,
            NULL
        );
        KW_DECREF(kw);
        JSON_DECREF(jn_options);
        return (hsdata)0;
    }

    hsdata hs = sdata_create(schema, gobj, on_write_it_cb, 0, 0, resource);
    if(!hs)  {
        KW_DECREF(kw);
        JSON_DECREF(jn_options);
        return (hsdata)0;
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
        JSON_DECREF(jn_options);
        return (hsdata)0;
    }

    if(priv->pDb) {
        /*
         *  Creating a new persistent resource, convert hsdata to json.
         */
        json_t *kw_resource = sdata2json(hs, SDF_PERSIST, 0);
        uint64_t id = sdata_read_uint64(hs, "id");
        uint64_t new_id = priv->dba->dba_create_record(gobj, priv->pDb, resource, kw_resource);
        if(new_id == -1 || new_id == 0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "dba_create_record() FAILED",
                "resource",     "%s", resource,
                NULL
            );
            sdata_destroy(hs);
            KW_DECREF(kw);
            JSON_DECREF(jn_options);
            return (hsdata)0;
        }
        if(!id) {
            // id set by bbdd
            sdata_write_uint64(hs, "id", new_id);
        } else {
            if(id != new_id) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                    "msg",          "%s", "new_id NOT MATCH",
                    "resource",     "%s", resource,
                    "id",           "%d", id,
                    "new_id",       "%d", new_id,
                    NULL
                );
            }
        }
    }

    /*
     *  Add to iter
     */
    rc_add_instance(resource_iter, hs, 0);

    /*
     *  Add id if not set
     */
    uint64_t id = sdata_read_uint64(hs, "id");
    if(id==0) {
        id = resource_iter->__last_id__;
        sdata_write_uint64(hs, "id", id);
    }

    KW_DECREF(kw);
    JSON_DECREF(jn_options);
    return hs;
}

/***************************************************************************
 *      Framework Method update_resource
 ***************************************************************************/
PRIVATE int mt_save_resource(
    hgobj gobj,
    const char *resource,
    json_t *hs_,
    json_t *jn_options // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    hsdata hs = (hsdata)hs_;  // HACK old compatibility

    if(priv->pDb) {
        /*
         *  Non-write fields must be checked at user level.
         */
        uint64_t id = sdata_read_uint64(hs, "id");
        if(!id) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "id REQUIRED to update resource",
                "resource",     "%s", resource,
                NULL
            );
            JSON_DECREF(jn_options);
            return 0;
        }

        json_t *kw_filtro = json_object();
        json_object_set_new(kw_filtro, "id", json_integer(id));
        json_t *kw_resource = sdata2json(hs, SDF_PERSIST, 0);

        priv->dba->dba_update_record(
            gobj,
            priv->pDb,
            resource,
            kw_filtro,
            kw_resource
        );
    }
    JSON_DECREF(jn_options);
    return 0;
}

/***************************************************************************
 *      Framework Method delete_resource
 ***************************************************************************/
PRIVATE int mt_delete_resource(
    hgobj gobj,
    const char *resource,
    json_t *hs_,
    json_t *jn_options // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    hsdata hs = (hsdata)hs_; // HACK old compatibility

    int ret = 0;

    if(priv->pDb) {
        uint64_t id = sdata_read_uint64(hs, "id");
        if(!id) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "id REQUIRED to delete resource",
                "resource",     "%s", resource,
                NULL
            );
            JSON_DECREF(jn_options);
            return -1;
        }
        json_t *kw_filtro = json_object();
        json_object_set_new(kw_filtro, "id", json_integer(id));
        ret = priv->dba->dba_delete_record(
            gobj,
            priv->pDb,
            resource,
            kw_filtro
        );
    }
    ret += rc_delete_resource(hs, sdata_destroy); // Must the last thing to do

    JSON_DECREF(jn_options);
    return ret;
}

/***************************************************************************
 *      Framework Method list_resource
 *  'ids' has prevalence over other fields.
 *  If 'ids' exists then other fields are ignored to search resources.
 ***************************************************************************/
PRIVATE void *mt_list_resource(
    hgobj gobj,
    const char *resource,
    json_t* jn_filter,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    // Return always an iter, although empty.
    dl_list_t *user_iter = rc_init_iter(0);

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
    if(!schema) {
        // Error already logged
        KW_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return user_iter;
    }
    json_int_t parent_id = 0;
    BOOL free_return_iter;
    dl_list_t *resource_iter = get_resource_iter(
        gobj,
        resource,
        parent_id,
        &free_return_iter
    );

    if(!resource_iter || dl_size(resource_iter)==0) {
        if(free_return_iter) {
            rc_free_iter(resource_iter, TRUE, 0);
        }
        KW_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return user_iter;
    }

    /*
     *  Extrae ids
     */
    json_int_t *ids_list = 0;
    if(kw_has_key(jn_filter, "ids")) {
        json_t *jn_ids = kw_get_dict_value(jn_filter, "ids", 0, 0);
        if(jn_ids) {
            json_t *jn_expanded = json_expand_integer_list(jn_ids);
            ids_list = jsonlist2c(jn_expanded);
            json_decref(jn_expanded);
        }
    } else if(jn_filter) {
        // Filtra de jn_filter solo las keys de hsdata
        const char **keys = sdata_keys(schema, -1, 0);
        if(keys) {
            json_t *jn_new_filter = kw_duplicate_with_only_keys(jn_filter, keys);
            gbmem_free(keys);
            KW_DECREF(jn_filter);
            jn_filter = jn_new_filter;
        }
    }

    hsdata hs_resource; rc_instance_t *i_hs;
    i_hs = rc_first_instance(resource_iter, (rc_resource_t **)&hs_resource);
    while(i_hs) {
        json_int_t id = sdata_read_uint64(hs_resource, "id");
        if(ids_list) {
            /*
             *  Si se busca por ids entonces SOLO se busca por ids.
             *  Es así para facilitar las busquedas exactas para update y delete.
             */
            if(int_in_list(id, ids_list)) {
                rc_add_instance(user_iter, hs_resource, 0);
            }
        } else {
            if(!priv->kw_match) {
                rc_add_instance(user_iter, hs_resource, 0);

            } else {
                if(sdata_match(
                    hs_resource,
                    SDF_PERSIST|SDF_VOLATIL,
                    0,
                    json_incref(jn_filter),
                    priv->kw_match)
                ) {
                    rc_add_instance(user_iter, hs_resource, 0);
                }
            }
        }
        i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_resource);
    }

    if(ids_list) {
        gbmem_free(ids_list);
    }
    if(free_return_iter) {
        rc_free_iter(resource_iter, TRUE, 0);
    }

    KW_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return user_iter;
}

/***************************************************************************
 *      Framework Method get_resource
 ***************************************************************************/
PRIVATE json_t *mt_get_resource(
    hgobj gobj,
    const char *resource,
    json_t *jn_filter,
    json_t *jn_options
)
{
    BOOL free_return_iter;
    dl_list_t *resource_iter = get_resource_iter(
        gobj,
        resource,
        0,
        &free_return_iter
    );

    if(!resource_iter || dl_size(resource_iter)==0) {
        if(free_return_iter) {
            rc_free_iter(resource_iter, TRUE, 0);
        }
        KW_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return 0;
    }
    json_int_t id = 0;
    if(json_is_string(jn_filter)) {
        id = atoll(json_string_value(jn_filter));
    } else {
        id = kw_get_int(jn_filter, "id", 0, KW_REQUIRED|KW_WILD_NUMBER);
    }
    hsdata hs_resource; rc_instance_t *i_hs;
    i_hs = rc_first_instance(resource_iter, (rc_resource_t **)&hs_resource);
    while(i_hs) {
        json_int_t id_ = sdata_read_uint64(hs_resource, "id");
        if(id == id_) {
            if(free_return_iter) {
                rc_free_iter(resource_iter, TRUE, 0);
            }
            KW_DECREF(jn_filter);
            return hs_resource;
        }
        i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_resource);
    }

    if(free_return_iter) {
        rc_free_iter(resource_iter, TRUE, 0);
    }

    KW_DECREF(jn_filter);
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
 *  Local method:
 *
 *  json_t *resource_webix_schema(const char *resource):
 *
 *      Return json webix schema of resource:
 *
 *      columns: [
 *          {
 *              id:"colname",
 *              header: "ColName",
 *              fillspace:10
 *          },
 *          ...
 *      ]
 *
 ***************************************************************************/
PUBLIC json_t *resource_webix_schema(
    hgobj gobj,
    const char *lmethod,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    char *resource = (char *)kw;
    const sdata_desc_t *it = resource_schema(priv->tb_resources, resource, 0);
    json_t *jn_list = json_array();
    if(!it) {
        return jn_list;
    }
    while(it->name) {
        sdata_flag_t flag =  it->flag;
        if(flag & SDF_NOTACCESS) {
            it++;
            continue;
        }
        json_t *jn_dict = json_object();
        json_object_set_new(jn_dict, "id", json_string(it->name));
        json_object_set_new(jn_dict, "header", json_string(it->header));
        json_object_set_new(jn_dict, "fillspace", json_integer(it->fillsp));

        json_array_append_new(jn_list, jn_dict);
        it++;
    }
    return jn_list;
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
        "gobj",         "%s", "Resource GCLASS",
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_PARAMETER_ERROR,
        "msg",          "%s", "Required resource attribute NOT FOUND",
        "resource",     "%s", sdata_resource(hs),
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
 *  Get the iter of `resource`
 ***************************************************************************/
PRIVATE dl_list_t *get_resource_iter(
    hgobj gobj,
    const char* resource,
    json_int_t parent_id,
    BOOL *free_return_iter)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    *free_return_iter = FALSE;

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
    if(!schema) {
        // Error already logged
        return 0;
    }

    /*
     *  Root table
     */
    return sdata_read_iter(priv->hs_resources, resource);
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
        if(it->flag & SDF_RESOURCE) {
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
    json_t *jn_resource;
    json_array_foreach(jn_resources, index, jn_resource) {
        const char *resource = json_string_value(jn_resource);
        sdata_flag_t resource_flag;
        const sdata_desc_t *items = resource_schema(priv->tb_resources, resource, &resource_flag);
        if(!items) {
            // Error already logged
            continue;
        }
        const char *key;
        json_t *kw_fields = schema2json(items, &key);
        priv->dba->dba_create_resource(gobj, priv->pDb, resource, key, kw_fields);
    }
    JSON_DECREF(jn_resources);
    return 0;
}

/***************************************************************************
 *  Callback to load root resources or purechilds
 ***************************************************************************/
PRIVATE int dba_load_record_cb(
    hgobj gobj,
    const char *resource,
    void *user_data,
    json_t *kw  // owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    dl_list_t *resource_iter = user_data;

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);

    hsdata hs = sdata_create(schema, gobj, on_write_it_cb, 0, 0, resource);
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

    rc_add_instance(resource_iter, hs, 0);

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
    json_t *jn_resource;
    json_array_foreach(jn_resources, index, jn_resource) {
        const char *resource = json_string_value(jn_resource);
        sdata_flag_t resource_flag;
        const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
        if(!schema) {
            // Error already logged
            continue;
        }
        /*
         *  root resource iter
         */
        dl_list_t *resource_iter = sdata_read_iter(priv->hs_resources, resource);

        json_t *kw_filtro = json_object(); //  WARNING load all records!

        json_t *jn_list = priv->dba->dba_load_table(
            gobj,
            priv->pDb,
            resource,
            resource,
            resource_iter,
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
    {"resource_webix_schema",   resource_webix_schema,      0},
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_RESOURCE_NAME,
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
        mt_save_resource,
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
PUBLIC GCLASS *gclass_resource(void)
{
    return &_gclass;
}
