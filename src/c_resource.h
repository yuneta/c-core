/****************************************************************************
 *          C_RESOURCE.H
 *          Resource GClass.
 *
 *          Resource Controler using hsdata and sqlite
 *          DEPRECATED remove hsdata dependency by json
 *
 *          Copyright (c) 2016-2022 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "yuneta_environment.h"
#include "c_timer.h"
#include "c_yuno.h"
#include "msglog_yuneta.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
/*
 *  Resources implemented with SQlite database
 */
#define GCLASS_RESOURCE_NAME "Resource"
#define GCLASS_RESOURCE gclass_resource()


/***************************************************************
 *              Structures
 ***************************************************************/

/*
 *
 *  HACK All data conversion must be going through json.
 *  SData and others are peripherals, leaf nodes. Json is a through-node.
 *
 *  Here the edges are the conversion algorithms.
 *  In others are the communication links.
 *
 *  Is true "What is above, is below. What is below, is above"?
 *
 */
typedef void * (*dba_open_fn)(
    hgobj gobj,
    const char *resource, json_t
    *jn_properties  // Owned
);
typedef int (*dba_close_fn)(hgobj gobj, void *pDb);
typedef int (*dba_create_resource_fn)( // HACK this function MUST BE idempotent! (like CREATE TABLE IF NOT EXISTS)
    hgobj gobj,
    void *pDb,
    const char *resource,
    const char *key,    // Must be in kw_fields in order to know the type of key
    json_t *kw_fields
);
typedef int (*dba_drop_resource_fn)(
    hgobj gobj,
    void *pDb,
    const char *resource
);
typedef uint64_t (*dba_create_record_fn)(
    hgobj gobj,
    void *pDb,
    const char *resource,
    json_t *kw_record   // owned
);
typedef int (*dba_update_record_fn)(
    hgobj gobj,
    void *pDb,
    const char *resource,
    json_t *kw_filtro,  // owned
    json_t *kw_record   // owned
);
typedef int (*dba_delete_record_fn)(
    hgobj gobj,
    void *pDb,
    const char *resource,
    json_t *kw_filtro  // owned
);

typedef int (*dba_record_cb)( // Return 1 to append, 0 to ignore or -1 to break the load.
    hgobj gobj,
    const char* resource,
    void *user_data,
    json_t *kw_record   // must be owned
);

//
// Return a json list of dictionaries with the record information.
// If you don't want the returned json list, use dba_record_cb to process yoursef the found records.
//
// If not null then is called with each record. Return 1 to admit, 0 to reject or -1 to break the loading.
// You can input the record in your system in dba_record_cb(),
// filling yourself an iter for example, and return 0 to reject it.
// If dba_record_cb() return 1, the record is added to json list return.
// Finally, if no record has been added, the return is a empty list.
// You can pass your own jn_record_list,  or if jn_record_list is null then a new list will be created and returned.
//
typedef json_t * (*dba_load_table_fn)(
    hgobj gobj,
    void *pDb,
    const char* tablename,
    const char* resource,
    void *user_data,    // To use as parameter in dba_record_cb() callback.
    json_t *kw_filtro,  // owned. Filter the records if you don't want load the full table.
    dba_record_cb dba_filter,
    json_t *jn_record_list
);


typedef struct {
    dba_open_fn                         dba_open;
    dba_close_fn                        dba_close;
    dba_create_resource_fn              dba_create_resource; // HACK MUST BE idempotent, (CREATE TABLE IF NOT EXISTS)
    dba_drop_resource_fn                dba_drop_resource;
    dba_create_record_fn                dba_create_record;
    dba_update_record_fn                dba_update_record;
    dba_delete_record_fn                dba_delete_record;
    dba_load_table_fn                   dba_load_table;
} dba_persistent_t;

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_resource(void);

#ifdef __cplusplus
}
#endif
