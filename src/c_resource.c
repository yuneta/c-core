/***********************************************************************
 *          C_RESOURCE.C
 *          Resource GClass.
 *
 *          Resource Controler

Resources will be out of ginsfsm. It owns to yuneta.
And, the parameters of commands will be defined with sdata!

Commands are managed with SData.
Then Resources commands acts like others command.


 *          Copyright (c) 2016,2018 Niyamaka.
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

PRIVATE json_int_t kw_get_purechild_parent_id(
    hgobj gobj,
    const char *resource,
    json_t *kw  // not owned
);
PRIVATE json_int_t get_grand_parent_id(hgobj gobj, hsdata hs, const char** parent_field);
PRIVATE dl_list_t *get_resource_iter(
    hgobj gobj,
    const char *resource,
    json_int_t parent_id,
    BOOL *free_return_iter
);

PRIVATE void build_n_n_childs_tablename(
    char *bf,
    int bfsize,
    const char *resource_parent,
    const char *resource_child
);
/*
 *  resources_list():
 *      Return [""], a list of strings with the names of resources:
 */
PRIVATE json_t * resources_list(hgobj gobj);

PRIVATE int create_persistent_resources(hgobj gobj);
PRIVATE int load_persistent_resources(hgobj gobj);
PRIVATE int load_persistent_childs(hgobj gobj, hsdata hs);



/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
/*
 *  HACK (id, parent_id, child_id) is the standard for paren->childs list.
 *  In the relational sql world, of course.
 */
PRIVATE sdata_desc_t tb_parent_child[] = {
/*-FIELD-type-----------name----------------flag------------------------schema----------header----------fillsp--description---------*/
SDATADF (ASN_COUNTER64, "id",               SDF_PERSIST|SDF_PKEY,       0,              "Id",           8,      0),
SDATADF (ASN_COUNTER64, "grand_parent_id",  SDF_PERSIST,                0,              "Grand Parent", 8,      0),
SDATADF (ASN_COUNTER64, "parent_id",        SDF_PERSIST,                0,              "Parent",       8,      0),
SDATADF (ASN_COUNTER64, "child_id",         SDF_PERSIST,                0,              "Child",        8,      0),
SDATA_END()
};


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
            JSON_INCREF(jn_properties);
            priv->pDb = priv->dba->dba_open(gobj, database, jn_properties);
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
PRIVATE hsdata mt_create_resource(hgobj gobj, const char *resource, json_t *kw)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);

    json_int_t parent_id = 0;
    if((resource_flag & SDF_PURECHILD)) {
        parent_id = kw_get_purechild_parent_id(
            gobj,
            resource,
            kw  // not owned
        );
        if(!parent_id) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "Creating PureChild WITHOUT parent_id!",
                "resource",     "%s", resource,
                NULL
            );
            KW_DECREF(kw);
            return (hsdata)0;
        }
    }
    BOOL free_return_iter; // Must be always FALSE, because parent_id is used.
    dl_list_t *resource_iter = get_resource_iter(
        gobj,
        resource,
        parent_id,
        &free_return_iter
    );
    if(!resource_iter) {
        KW_DECREF(kw);
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
        return (hsdata)0;
    }

    hsdata hs = sdata_create(schema, gobj, on_write_it_cb, 0, 0, resource);
    if(!hs)  {
        KW_DECREF(kw);
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

    /*
     *  Add childs of the creating resource.
     */
    const sdata_desc_t *it = schema;
    while(it->name) {
        if((it->flag & SDF_RESOURCE) && !(it->flag & SDF_PURECHILD)) {
            if(kw_has_key(kw, it->name)) {
                json_t *kw_list_ids = kw_get_dict_value(kw, it->name, 0, 0);
                json_t *kw_childs = kwids_json2kwids(kw_list_ids);
                dl_list_t *iter_childs = gobj_list_resource(
                    gobj,
                    it->resource,
                    kw_childs
                );
                if(iter_childs) {
                    hsdata hs_child; rc_instance_t *i_hs;
                    i_hs = rc_first_instance(iter_childs, (rc_resource_t **)&hs_child);
                    while(i_hs) {
                        gobj_add_child_resource_link(
                            gobj,
                            hs,
                            hs_child
                        );

                        /*
                         *  Next child
                         */
                        i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_child);
                    }
                    rc_free_iter(iter_childs, TRUE, 0);
                }
            }
        }
        it++;
    }

    KW_DECREF(kw);
    return hs;
}

/***************************************************************************
 *      Framework Method update_resource
 ***************************************************************************/
PRIVATE int mt_update_resource(hgobj gobj, hsdata hs)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->pDb) {
        /*
         *  Non-write fields must be checked at user level.
         */
        const char *resource = sdata_resource(hs);
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
            return -1;
        }

        json_t *kw_filtro = json_object();
        json_object_set_new(kw_filtro, "id", json_integer(id));
        json_t *kw_resource = sdata2json(hs, SDF_PERSIST, 0);

        return priv->dba->dba_update_record(
            gobj,
            priv->pDb,
            resource,
            kw_filtro,
            kw_resource
        );
    }
    return 0;
}

/***************************************************************************
 *      Framework Method delete_resource
 ***************************************************************************/
PRIVATE int mt_delete_resource(hgobj gobj, hsdata hs)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *resource = sdata_resource(hs);
    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);

    /*
     *  Firstly remove childs of the deleting resource.
     */
    const sdata_desc_t *it = schema;
    while(it->name) {
        if((it->flag & SDF_RESOURCE) && !(it->flag & SDF_PURECHILD)) {
            dl_list_t *childs_iter = sdata_read_iter(hs, it->name);
            hsdata hs_child; rc_instance_t *i_hs;
            while((i_hs = rc_first_instance(childs_iter, (rc_resource_t **)&hs_child))) {
                gobj_delete_child_resource_link(gobj, hs, hs_child);
            }
        }
        it++;
    }

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

    return ret;
}

/***************************************************************************
 *      Framework Method add_child_resource_link
 ***************************************************************************/
PRIVATE int mt_add_child_resource_link(hgobj gobj, hsdata hs_parent, hsdata hs_child)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *parent_resource = sdata_resource(hs_parent);
    const char *child_resource = sdata_resource(hs_child);

    sdata_flag_t parent_resource_flag;
    const sdata_desc_t *parent_schema = resource_schema(priv->tb_resources, parent_resource, &parent_resource_flag);

    /*
     *  Search the parent's field with the childs iter.
     *  Must be a field with SDF_RESOURCE flag and
     *  his `resource` field matching the resource of hs_child.
     */
    dl_list_t *childs_iter = 0;
    const sdata_desc_t *parent_it = parent_schema;
    while(parent_it->name) {
        if((parent_it->flag & SDF_RESOURCE) && !(parent_it->flag & SDF_PURECHILD)) {
        //if((parent_it->flag & SDF_RESOURCE)) {
            if(parent_it->resource && strcasecmp(child_resource, parent_it->resource)==0) {
                childs_iter = sdata_read_iter(hs_parent, parent_it->name);
                break;
            }
        }
        parent_it++;
    }
    if(!childs_iter) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "childs_iter NOT FOUND",
            "parent_resource",  "%s", parent_resource,
            "child_resource",   "%s", child_resource,
            NULL
        );
        return -1;
    }
    if(rc_instance_index(childs_iter, hs_child, 0)) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "resource ALREADY in iter",
            "parent_resource",  "%s", parent_resource,
            "child_resource",   "%s", child_resource,
            NULL
        );
        return -1;
    }

    /*
     *  It's a n-n relation.
     *
     *  The n-n implies to have a external table with the "'parent_resource'_'child_resource'" name
     *  in addition to the "parent_resource" "child_resource" tables.
     *  Si el parent es child único (pure child),
     *  entonces la tabla tiene que ser: gran_parent + parent + child resource!
     */

    rc_instance_t *i_hs_child = rc_add_instance(childs_iter, hs_child, 0);

    /*
     *  Persistence
     */
    if(priv->pDb) {
        uint64_t parent_id = sdata_read_uint64(hs_parent, "id");
        uint64_t child_id = sdata_read_uint64(hs_child, "id");
        uint64_t grand_parent_id = 0;
        const char *parent_field;
        if((parent_resource_flag & SDF_PURECHILD)) {
            grand_parent_id = get_grand_parent_id(gobj, hs_parent, &parent_field);
        }
        json_t *kw_relation = json_pack("{s:I, s:I, s:I}",
            "grand_parent_id", (json_int_t)grand_parent_id,
            "parent_id", (json_int_t)parent_id,
            "child_id", (json_int_t)child_id
        );
        char childs_tablename[90];
        build_n_n_childs_tablename(childs_tablename, sizeof(childs_tablename), parent_resource, child_resource);

        uint64_t new_id = priv->dba->dba_create_record(gobj, priv->pDb, childs_tablename, kw_relation);
        if(new_id == -1 || new_id == 0) {
            rc_delete_instance(i_hs_child, 0);
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_INTERNAL_ERROR,
                "msg",              "%s", "Cannot create child resource link",
                "parent_resource",  "%s", parent_resource,
                "child_resource",   "%s", child_resource,
                "tablename",        "%s", childs_tablename,
                "return",           "%d", (int)new_id,
                NULL
            );
            return -1;
        }
        rc_instance_set_id(i_hs_child, new_id);
    }
    return 0;
}

/***************************************************************************
 *      Framework Method delete_child_resource_link
 ***************************************************************************/
PRIVATE int mt_delete_child_resource_link(hgobj gobj, hsdata hs_parent, hsdata hs_child)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *parent_resource = sdata_resource(hs_parent);
    const char *child_resource = sdata_resource(hs_child);

    sdata_flag_t resource_flag;
    const sdata_desc_t *parent_schema = resource_schema(priv->tb_resources, parent_resource, &resource_flag);

    /*
     *  Search the parent's field with the childs iter.
     *  Must be a field with SDF_RESOURCE flag and his `resource` field matching the resource of hs_child.
     */
    dl_list_t *childs_iter = 0;
    const sdata_desc_t *it = parent_schema;
    while(it->name) {
        if((it->flag & SDF_RESOURCE) && !(it->flag & SDF_PURECHILD)) {
        //if((it->flag & SDF_RESOURCE)) {
            if(it->resource && strcasecmp(child_resource, it->resource)==0) {
                childs_iter = sdata_read_iter(hs_parent, it->name);
                break;
            }
        }
        it++;
    }
    if(!childs_iter) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "childs_iter NOT FOUND",
            "parent_resource",  "%s", parent_resource,
            "child_resource",   "%s", child_resource,
            NULL
        );
        return -1;
    }

    rc_instance_t *i_hs_child = rc_instance_index(childs_iter, hs_child, 0);
    if(!i_hs_child) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "hs child NOT FOUND",
            "parent_resource",  "%s", parent_resource,
            "child_resource",   "%s", child_resource,
            NULL
        );
        return -1;
    }

    if(priv->pDb) {
        /*
         *  Delete sqlite sql
         */
        char childs_tablename[90];
        build_n_n_childs_tablename(childs_tablename, sizeof(childs_tablename), parent_resource, child_resource);
        uint64_t id = rc_instance_get_id(i_hs_child);
        if(id == -1 || id == 0) {
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_INTERNAL_ERROR,
                "msg",              "%s", "id REQUIRED to delete child resource link",
                "childs_tablename", "%s", childs_tablename,
                NULL
            );
            return -1;
        }
        json_t *kw_filtro = json_object();
        json_object_set_new(kw_filtro, "id", json_integer(id));
        priv->dba->dba_delete_record(
            gobj,
            priv->pDb,
            childs_tablename,
            kw_filtro
        );
    }

    rc_delete_instance(i_hs_child, sdata_destroy);

    return 0;
}

/***************************************************************************
 *      Framework Method list_resource
 *  'ids' has prevalence over other fields.
 *  If 'ids' exists then other fields are ignored to search resources.
 ***************************************************************************/
PRIVATE dl_list_t * mt_list_resource(hgobj gobj, const char *resource, json_t* jn_filter)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    // Return always an iter, although empty.
    dl_list_t *user_iter = rc_init_iter(0);

    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
    if(!schema) {
        // Error already logged
        KW_DECREF(jn_filter);
        return user_iter;
    }
    json_int_t parent_id = 0;
    if((resource_flag & SDF_PURECHILD)) {
        parent_id = kw_get_purechild_parent_id(
            gobj,
            resource,
            jn_filter  // not owned
        );
    }
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
            KW_INCREF(jn_filter);
            if(!priv->kw_match || sdata_match(
                    hs_resource,
                    SDF_PERSIST|SDF_VOLATIL,
                    0,
                    jn_filter,
                    priv->kw_match)
              ) {
                /*
                 *  Matched or no match fn
                 */
                if(!priv->kw_match) {
                    KW_DECREF(jn_filter);
                }
                /*
                 *  Now check n-n fields
                 */
                BOOL n_n_matched = TRUE;
                const sdata_desc_t *it = schema;
                while(it->name) {
                    if((it->flag & SDF_RESOURCE) && !(it->flag & SDF_PURECHILD)) {
                        if(kw_has_key(jn_filter, it->name)) {
                            json_t *kw_list_ids = kw_get_dict_value(jn_filter, it->name, 0, 0);
                            json_t *jn_ids = json_expand_integer_list(kw_list_ids);

                            dl_list_t *childs_iter = sdata_read_iter(hs_resource, it->name);
                            hsdata hs_child; rc_instance_t *i_hs;
                            i_hs = rc_first_instance(childs_iter, (rc_resource_t **)&hs_child);
                            while(i_hs) {
                                json_int_t id_ = sdata_read_uint64(hs_child, "id");
                                if(!int_in_jn_list(id_, jn_ids)) {
                                    n_n_matched = FALSE;
                                    break;
                                }
                                i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_child);
                            }

                            json_decref(jn_ids);
                        }
                    }
                    it++;
                }
                if(n_n_matched) {
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
    return user_iter;
}

/***************************************************************************
 *      Framework Method get_resource
 ***************************************************************************/
PRIVATE hsdata mt_get_resource(hgobj gobj, const char *resource, json_int_t parent_id, json_int_t id)
{
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
        return 0;
    }

    hsdata hs_resource; rc_instance_t *i_hs;
    i_hs = rc_first_instance(resource_iter, (rc_resource_t **)&hs_resource);
    while(i_hs) {
        json_int_t id_ = sdata_read_uint64(hs_resource, "id");
        if(id == id_) {
            if(free_return_iter) {
                rc_free_iter(resource_iter, TRUE, 0);
            }
            return hs_resource;
        }
        i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_resource);
    }

    if(free_return_iter) {
        rc_free_iter(resource_iter, TRUE, 0);
    }

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
PRIVATE void * resource_webix_schema(hgobj gobj, void *data)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *resource = data;
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
 *  In a pure child schema, find the field with the parent id.
 ***************************************************************************/
PRIVATE const sdata_desc_t *get_parent_it_field(hgobj gobj, const sdata_desc_t *schema)
{
    const sdata_desc_t *it = schema;
    if(schema) {
        while(it->name) {
            if((it->flag & SDF_PARENTID)) {
                return it;
            }
            it++;
        }
    }
    log_error(0,
        "gobj",         "%s", gobj_full_name(gobj),
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_SERVICE_ERROR,
        "msg",          "%s", "Parent id field not found",
        NULL
    );
    return 0;
}

/***************************************************************************
 *  Get the parent_id of resource in kw
 ***************************************************************************/
PRIVATE json_int_t kw_get_purechild_parent_id(
    hgobj gobj,
    const char *resource,
    json_t *kw) // not owned
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
    if(!schema) {
        // Error already logged
        return 0;
    }
    if(!(resource_flag & SDF_PURECHILD)) {
        return 0;
    }

    const sdata_desc_t *parent_it = get_parent_it_field(gobj, schema);
    if(!parent_it) {
        // Error already logged
        return 0;
    }

    json_int_t parent_id = kw_get_int(kw, parent_it->name, 0, 0);

    return parent_id;
}

/***************************************************************************
 *  Es grand_parent relativo al child del pure-child.
 *  Para el pure-child es el parent.
 *  En las tablas n-n se declara la relación parent_id-child_d cualquiera,
 *  pero como en este caso el parent_id es un pure-child,
 *  se ha añadido el campo gran_parent_id, que representa al parent de parent_id
 ***************************************************************************/
PRIVATE json_int_t get_grand_parent_id(hgobj gobj, hsdata hs, const char **parent_field)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    const char *resource = sdata_resource(hs);
    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
    if(!(resource_flag & SDF_PURECHILD)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SERVICE_ERROR,
            "msg",          "%s", "Non-pure child has not grand parent",
            "resource",     "%s", resource,
            NULL
        );
        if(parent_field) {
            *parent_field = 0;
        }
        return 0;
    }

    const sdata_desc_t *parent_it = get_parent_it_field(gobj, schema);
    if(!parent_it) {
        // Error already logged
        if(parent_field) {
            *parent_field = 0;
        }
        return 0;
    }
    if(parent_field) {
        *parent_field = parent_it->name;
    }

    json_int_t grand_parent_id = sdata_read_uint64(hs, parent_it->name);
    return grand_parent_id;
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
     *  Pueden darse dos situaciones: el resource es root, y priv->hs_resources(resource) tiene su iter
     *  o es child puro y el iter lo tiene en el register del padre
     */
    if(!(resource_flag & SDF_PURECHILD)) {
        /*
         *  Root table
         */
        return sdata_read_iter(priv->hs_resources, resource);
    }

    /*
     *  Child puro
     */
    const sdata_desc_t *parent_it = get_parent_it_field(gobj, schema);
    if(!parent_it) {
        // Error already logged
        return 0;
    }
    if(!parent_it->resource) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Parent item without resource field",
            "parent_it",    "%s", parent_it->name,
            NULL
        );
        return 0;
    }

    /*
     * Only one parent
     */
    if(parent_id) {
        hsdata hs_parent = gobj_get_resource(gobj, parent_it->resource, 0, parent_id);
        if(!hs_parent) {
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_PARAMETER_ERROR,
                "msg",              "%s", "Parent not found",
                "parent_resource",  "%s", parent_it->resource,
                "parent_field",     "%s", parent_it->name,
                "parent_id",        "%d", (int)parent_id,
                "resource",         "%s", resource,
                NULL
            );
            return 0;
        }
        return sdata_read_iter(hs_parent, resource);
    }

    /*
     * All parents
     */
    dl_list_t *user_iter = rc_init_iter(0);
    *free_return_iter = TRUE;

    dl_list_t *parents_iter = gobj_list_resource(gobj, parent_it->resource, 0);

    hsdata hs_parent; rc_instance_t *i_hs;
    i_hs = rc_first_instance(parents_iter, (rc_resource_t **)&hs_parent);
    while(i_hs) {
        dl_list_t *iter_ = sdata_read_iter(hs_parent, resource);
        rc_add_iter(user_iter, iter_);
        i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_parent);
    }
    rc_free_iter(parents_iter, TRUE, 0);

    return user_iter;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void build_n_n_childs_tablename(
    char *bf,
    int bfsize,
    const char *parent_resource,
    const char *child_resource)
{
    // Private table, id from rowid, random, not set by the user, only by code.
    snprintf(bf, bfsize, "__%s_%s__", parent_resource, child_resource);
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

        /*
         *  Create non-pure child tables (n-n relation).
         */
        const sdata_desc_t *it = items;
        while(it->name) {
            if((it->flag & SDF_RESOURCE) && !(it->flag & SDF_PURECHILD)) {
                if(empty_string(it->resource)) {
                    it++;
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_SERVICE_ERROR,
                        "msg",          "%s", "Child field without `resource`",
                        "resource",     "%s", resource,
                        "field",        "%s", it->name,
                        NULL
                    );
                    continue;
                }
                char childs_tablename[80];
                build_n_n_childs_tablename(childs_tablename, sizeof(childs_tablename), resource, it->resource);

                kw_fields = schema2json(tb_parent_child, &key);
                priv->dba->dba_create_resource(gobj, priv->pDb, childs_tablename, key, kw_fields);
            }
            it++;
        }
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
 *  Callback to load child resources (non-purechild)
 ***************************************************************************/
PRIVATE int dba_load_child_record_cb(
    hgobj gobj,
    const char *resource,
    void *user_data,
    json_t *kw  // owned
)
{
    dl_list_t *resource_iter = user_data;

    json_int_t child_id = kw_get_int(kw, "child_id", 0, KW_REQUIRED);
    hsdata hs_child = gobj_get_resource(gobj, resource, 0, child_id);
    if(!hs_child) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "child NOT FOUND",
            "resource",         "%s", resource,
            "id",               "%d", (int)child_id,
            NULL
        );
        KW_DECREF(kw);
        return 0; // Don't append record
    }

    if(rc_instance_index(resource_iter, hs_child, 0)) {
        // 16-May-2020 ocurre con rc_treedb, ignora
//         log_error(0,
//             "gobj",             "%s", gobj_full_name(gobj),
//             "function",         "%s", __FUNCTION__,
//             "msgset",           "%s", MSGSET_INTERNAL_ERROR,
//             "msg",              "%s", "resource ALREADY in iter",
//             "resource",         "%s", resource,
//             "child_id",         "%d", (int)child_id,
//             NULL
//         );
        KW_DECREF(kw);
        return 0; // Don't append record
    }

    rc_instance_t *i_rc = rc_add_instance(resource_iter, hs_child, 0);
    uint64_t id = kw_get_int(kw, "id", 0, KW_REQUIRED);
    if(id == 0) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_INTERNAL_ERROR,
            "msg",              "%s", "record with id=0",
            "resource",         "%s", resource,
            NULL
        );
    }
    rc_instance_set_id(i_rc, id);

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
        if(resource_flag & SDF_PURECHILD) {
            // Pure child data will be load within each parent resource instance.
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

    /*------------------------*
     *      Load childs
     *------------------------*/
    json_array_foreach(jn_resources, index, jn_resource) {
        const char *resource = json_string_value(jn_resource);
        sdata_flag_t resource_flag;
        const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);
        if(!schema) {
            // Error already logged
            continue;
        }
        if(resource_flag & SDF_PURECHILD) {
            // Pure child data will be load within each parent resource instance.
            continue;
        }
        /*
         *  root resource iter
         */
        dl_list_t *resource_iter = sdata_read_iter(priv->hs_resources, resource);

        /*
         *  Poll all root resource's instances to search childs iters.
         */
        hsdata hs_parent; rc_instance_t *i_hs;
        i_hs = rc_first_instance(resource_iter, (rc_resource_t **)&hs_parent);
        while(i_hs) {
            load_persistent_childs(gobj, hs_parent);
            i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_parent);
        }
    }
    JSON_DECREF(jn_resources);

    return 0;
}

/***************************************************************************
 *  Load childs (pure childs and/or n-n childs)
 ***************************************************************************/
PRIVATE int load_persistent_childs(hgobj gobj, hsdata hs)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    json_int_t parent_id = sdata_read_uint64(hs, "id");

    const char *resource = sdata_resource(hs);
    sdata_flag_t resource_flag;
    const sdata_desc_t *schema = resource_schema(priv->tb_resources, resource, &resource_flag);

    const sdata_desc_t *it = schema;
    while(it->name) {
        if((it->flag & SDF_RESOURCE)) {
            if(empty_string(it->resource)) {
                it++;
                continue;
            }
            if((it->flag & SDF_PURECHILD)) {
                /*
                 *  pure childs (n-1 relation)
                 */
                const sdata_desc_t *child_schema = resource_schema(priv->tb_resources, it->resource, 0);
                if(!child_schema) {
                    break;
                }

                BOOL free_return_iter;
                dl_list_t *child_iter = get_resource_iter(
                    gobj,
                    it->resource,
                    parent_id,
                    &free_return_iter
                );

                const sdata_desc_t *parent_it = get_parent_it_field(gobj, child_schema);

                json_t *kw_filtro = json_object();
                json_object_set_new(kw_filtro, parent_it->name, json_integer(parent_id));

                json_t *jn_list = priv->dba->dba_load_table(
                    gobj,
                    priv->pDb,
                    it->resource,
                    it->resource,
                    child_iter,
                    kw_filtro,
                    dba_load_record_cb,
                    0
                );
                JSON_DECREF(jn_list); // we load records in callback, ignore return

                /*
                 *  Poll all pure-child resource's instances to search childs iters.
                 */
                hsdata hs_parent; rc_instance_t *i_hs;
                i_hs = rc_first_instance(child_iter, (rc_resource_t **)&hs_parent);
                while(i_hs) {
                    load_persistent_childs(gobj, hs_parent);
                    i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs_parent);
                }
                if(free_return_iter) {
                    rc_free_iter(child_iter, TRUE, 0);
                }
            } else {
                /*
                 *  non-pure childs (n-n relation)
                 */
                json_int_t grand_parent_id = 0;
                if((resource_flag & SDF_PURECHILD)) {
                    grand_parent_id = get_grand_parent_id(gobj, hs, 0);
                }
                char childs_tablename[80];
                build_n_n_childs_tablename(childs_tablename, sizeof(childs_tablename),
                    resource,       // parent resource
                    it->resource    // child resource
                );
                dl_list_t *childs_iter = sdata_read_iter(hs, it->name);
                json_t *kw_filtro = json_object();
                json_object_set_new(kw_filtro, "parent_id", json_integer(parent_id));
                json_object_set_new(kw_filtro, "grand_parent_id", json_integer(grand_parent_id));

                json_t *jn_list = priv->dba->dba_load_table(
                    gobj,
                    priv->pDb,
                    childs_tablename,
                    it->resource,
                    childs_iter,
                    kw_filtro,
                    dba_load_child_record_cb,
                    0
                );
                JSON_DECREF(jn_list); // we load records in callback, ignore return
            }
        }
        it++;
    }
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
        mt_update_resource,
        mt_delete_resource,
        mt_add_child_resource_link,
        mt_delete_child_resource_link,
        mt_get_resource,
        0, //mt_future24,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_permission_on,
        0, //mt_permission_off,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_future38,
        0, //mt_future39,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_link_nodes2,
        0, //mt_unlink_nodes,
        0, //mt_unlink_nodes2,
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
        0, //mt_node_instances,
        0, //mt_save_node,
        0, //mt_future61,
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

// /***************************************************************************
//  *  Debug
//  ***************************************************************************/
// PUBLIC void resources_log(const char *prefix, hgobj gobj)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     sdata_desc_t *it = priv->tb_resources;
//     while(it->name) {
//         if(it->flag & SDF_RESOURCE) {
//             dl_list_t *resource_iter = sdata_read_iter(priv->hs_resources, it->name);
//             log_debug_sd_iter(prefix, 0, resource_iter);
//         }
//         it++;
//     }
// }

// TODO Fuera load_db/save_db de agent
// /***************************************************************************
//  *
//  ***************************************************************************/
// PRIVATE json_t *cmd_load_db(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     const char *source = kw_get_str(kw, "source", 0, 0);
//     if(!source) {
//         return msg_iev_build_webix(
//             gobj,
//             -101,
//             json_local_sprintf("What source?"),
//             0,
//             0,
//             kw  // owned
//         );
//     }
//
//     char path[NAME_MAX];
//     if(access(source, 0)==0) {
//         snprintf(path, sizeof(path), "%s", source);
//     } else {
//         yuneta_store_file(path, sizeof(path), "backups", "", source, TRUE);
//     }
//     if(access(path, 0)!=0) {
//         return msg_iev_build_webix(
//             gobj,
//             -102,
//             json_local_sprintf("Cannot open '%s' source.", path),
//             0,
//             0,
//             kw  // owned
//         );
//     }
//     size_t flags = 0;
//     json_error_t error;
//     json_t *jn_config = json_load_file(path, flags, &error);
//     if(!jn_config) {
//         log_error(0,
//             "gobj",         "%s", gobj_full_name(gobj),
//             "function",     "%s", __FUNCTION__,
//             "msgset",       "%s", MSGSET_JSON_ERROR,
//             "msg",          "%s", "JSON data INVALID",
//             "line",         "%d", error.line,
//             "column",       "%d", error.column,
//             "position",     "%d", error.position,
//             "json",         "%s", error.text,
//             "path",         "%s", path,
//             NULL
//         );
//         return msg_iev_build_webix(
//             gobj,
//             -103,
//             json_local_sprintf("Cannot load db. Bad json format in '%s' source.", path),
//             0,
//             0,
//             kw  // owned
//         );
//     }
//
//     json_t *jn_resources = kw_get_dict_value(jn_config, "resources", 0, 0);
//     json_t *jn_service = kw_get_dict_value(jn_resources, gobj_short_name(priv->resource), 0, 0);
//     size_t n_records = 0;
//     sdata_desc_t *it = tb_resources;
//     if(jn_service) {
//         while(it->name) {
//             if((it->flag & SDF_PURECHILD)) {
//                 it++;
//                 continue;
//             }
//             const char *resource = it->name;
//             json_t *jn_resource = kw_get_dict_value(jn_service, it->name, 0, 0);
//             int index;
//             json_t *jn_record;
//             json_array_foreach(jn_resource, index, jn_record) {
//                 /*
//                  *  Add to database
//                  */
//                 json_incref(jn_record);
//                 if(gobj_create_resource(priv->resource, resource, jn_record)) {
//                     n_records ++;
//                 }
//             }
//             it++;
//         }
//     }
//     it = tb_resources;
//     if(jn_service) {
//         while(it->name) {
//             if(!(it->flag & SDF_PURECHILD)) {
//                 it++;
//                 continue;
//             }
//             const char *resource = it->name;
//             json_t *jn_resource = kw_get_dict_value(jn_service, it->name, 0, 0);
//             int index;
//             json_t *jn_record;
//             json_array_foreach(jn_resource, index, jn_record) {
//                 /*
//                  *  Add to database
//                  */
//                 json_incref(jn_record);
//                 if(gobj_create_resource(priv->resource, resource, jn_record)) {
//                     n_records ++;
//                 }
//             }
//             it++;
//         }
//     }
//
//     JSON_DECREF(jn_config);
//
//     return msg_iev_build_webix(
//         gobj,
//         0,
//         json_local_sprintf("Loaded %d records.", n_records),
//         0,
//         0,
//         kw  // owned
//     );
//     return 0;
// }
//
// /***************************************************************************
//  *
//  ***************************************************************************/
// PRIVATE BOOL some_pure_pure_child(const sdata_desc_t *schema)
// {
//     const sdata_desc_t *it = schema;
//     while(it->name) {
//         if((it->flag & SDF_PURECHILD)) { ?
//             return TRUE;
//         }
//         it++;
//     }
//     return FALSE;
// }
//
// /***************************************************************************
//  *
//  ***************************************************************************/
// PRIVATE json_t *cmd_save_db(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     json_int_t realm_id = kw_get_int(kw, "realm_id", 0, 0);
//
//     /*----------------------------------*
//      *      Build destination file
//      *----------------------------------*/
//     char fecha[32];
//     char source_[NAME_MAX];
//     const char *destination = kw_get_str(kw, "destination", 0, 0);
//     if(empty_string(destination)) {
//         /*
//         *  Mask "DD/MM/CCYY-hh:mm:ss-w-ddd"
//         */
//         formatnowdatetime(fecha, sizeof(fecha), "DD-MM-CCYY");
//         snprintf(source_, sizeof(source_), "%s.%s.json",
//             gobj_yuno_role_plus_name(),
//             fecha);
//         destination = source_;
//     }
//     char path[NAME_MAX];
//     yuneta_store_file(path, sizeof(path), "backups", "", destination, TRUE);
//
//     /*----------------------------------*
//      *      Prepare json result
//      *----------------------------------*/
//     json_t *jn_save = json_object();
//     json_t *jn_services = json_object();
//     json_object_set_new(jn_save, "resources", jn_services);
//     json_t *jn_service = json_object();
//     json_object_set_new(jn_services, gobj_short_name(priv->resource), jn_service);
//
//     /*-------------------------------*
//      *  Save root resources
//      *-------------------------------*/
//     json_t * jn_resources = resources_list(priv->resource);
//     size_t index;
//     json_t *jn_resource;
//     json_array_foreach(jn_resources, index, jn_resource) {
//         const char *resource = json_string_value(jn_resource);
//         sdata_flag_t resource_flag;
//         xxx_resource_schema(priv->resource, resource, &resource_flag);
//         if((resource_flag & SDF_PURECHILD)) {
//             continue;
//         }
//
//         dl_list_t *iter = gobj_list_resource(priv->resource, resource, 0);
//         if(iter) {
//             json_t *jn_data = sdata_iter2json(iter, SDF_PERSIST|SDF_RESOURCE, SDF_PURECHILD);
//             if(jn_data) {
//                 json_object_set_new(jn_service, resource, jn_data);
//             }
//             rc_free_iter(iter, TRUE, 0);
//         }
//     }
//
//     /*-------------------------------*
//      *  Save pure-child resources
//      *-------------------------------*/
//     json_array_foreach(jn_resources, index, jn_resource) {
//         const char *resource = json_string_value(jn_resource);
//         sdata_flag_t resource_flag;
//         const sdata_desc_t *schema = xxx_resource_schema(priv->resource, resource, &resource_flag);
//         if(!schema || !some_pure_pure_child(schema)) {
//             continue;
//         }
//
//         /*---------------------------------------*
//          *  Poll root resource with pure childs
//          *---------------------------------------*/
//         dl_list_t *iter = gobj_list_resource(priv->resource, resource, 0);
//         hsdata hs; rc_instance_t *i_hs;
//         i_hs = rc_first_instance(iter, (rc_resource_t **)&hs);
//         while(i_hs) {
//             const sdata_desc_t *it = schema;
//             while(it->name) {
//                 if((it->flag & SDF_RESOURCE) && (it->flag & SDF_PURECHILD)) {
//                     const char *purechild_resource = it->resource;
//                     dl_list_t *purechild_iter = sdata_read_iter(hs, it->name);
//
//                     hsdata hs_child; rc_instance_t *i_hs_child;
//                     i_hs_child = rc_first_instance(purechild_iter, (rc_resource_t **)&hs_child);
//                     while(i_hs_child) {
//                         /*---------------------------------------------------*
//                          *  WARNING this code make this function
//                          *  as a not generic function.
//                          *  Esto se podría generalizar con kw que se
//                          *  conviertan en hs_match.
//                          *---------------------------------------------------*/
//                         if(realm_id>0 && strcasecmp(purechild_resource, "yunos")==0) {
//                             json_int_t realm_id_ = sdata_read_int64(hs_child, "realm_id");
//                             if(realm_id_ != realm_id) {
//                                 i_hs_child = rc_next_instance(i_hs_child, (rc_resource_t **)&hs_child);
//                                 continue;
//                             }
//                         }
//
//                         json_t *jn_r = sdata2json(hs_child, SDF_PERSIST|SDF_RESOURCE, SDF_PURECHILD);
//                         if(jn_r) {
//                             json_t *jn_yunos = json_object_get(jn_service, purechild_resource);
//                             if(!jn_yunos) {
//                                 jn_yunos = json_array();
//                                 json_array_append_new(jn_yunos, jn_r);
//                                 json_object_set_new(jn_service, purechild_resource, jn_yunos);
//                             } else {
//                                 json_array_append_new(jn_yunos, jn_r);
//                             }
//                         }
//                         i_hs_child = rc_next_instance(i_hs_child, (rc_resource_t **)&hs_child);
//                     }
//                 }
//                 it++;
//             }
//             i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hs);
//         }
//
//         rc_free_iter(iter, TRUE, 0);
//     }
//
//     JSON_DECREF(jn_resources);
//
//     /*-------------------------------*
//      *  To file
//      *-------------------------------*/
//     json_dump_file(jn_save, path, JSON_ENCODE_ANY | JSON_INDENT(4));
//     JSON_DECREF(jn_save);
//
//     return msg_iev_build_webix(
//         gobj,
//         0,
//         json_local_sprintf("Configuration saved in '%s' destination.", path),
//         0,
//         0,
//         kw  // owned
//     );
// }
