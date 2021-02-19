/***********************************************************************
 *          C_STORE.C
 *          Store GClass.
 *
 *          Store service
 *
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 *
 *  Los almacences se encuentran en el directorio
 *      .../store
 *
 *  Hay 3 niveles de clasificación:
 *      - store     (almacen)       OBLIGATORIO
 *      - room      (sala)          OPCIONAL
 *      - shelf     (estanteria)    OPCIONAL
 *      - id        (referencia del documento)  OBLIGATORIO (autocreado)
 *
 *  Ejemplo: Agent service store:
 *
 *      - store = yunos             Este store ahora será local, en el futuro será remoto
 *      - room = binaries
 *      - room = configurations
 *
 *      - store = realms (combination of binaries & configurations)
 *          Los binarios y configs no son enlaces directos (referencias)
 *          sino copias integras, para hacer del servicio un sistema robusto,
 *          con la desventaja claro de una distribución y actualización mas costosa.
 *
 *  Los eventos del servicio son: EV_WRITE_RESOURCE, EV_READ_RESOURCE y EV_DELETE_RESOURCE.
 *  Write es la entrada del recurso al almacén.
 *  Read es la salida de una **copia** del recurso del almacén.
 *  Remove es la eliminacion del recurso del almacén.
 *
 ***********************************************************************/
#include <string.h>
#include <stdio.h>
#include "c_store.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_INTEGER,     "timeout",              0, 2*1000, "Timeout"),
SDATA (ASN_POINTER,     "user_data",            0, 0, "user data"),
SDATA (ASN_POINTER,     "user_data2",           0, 0, "more user data"),
SDATA (ASN_POINTER,     "subscriber",           0, 0, "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_USER = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"trace_user",        "Trace user description"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    int32_t timeout;

    hgobj timer;
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

    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);

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
    SET_PRIV(timeout,               gobj_read_int32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout,             gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout);
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);
    return 0;
}





            /***************************
             *      Commands
             ***************************/




            /***************************
             *      Local Methods
             ***************************/




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_update_resource(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_list_resource(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_delete_resource(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_WRITE_RESOURCE",   EVF_PUBLIC_EVENT,   0,   0, "Write resource"},
    {"EV_READ_RESOURCE",    EVF_PUBLIC_EVENT,   0,   0, "Read resource"},
    {"EV_DELETE_RESOURCE",  EVF_PUBLIC_EVENT,   0,   0, "Delete resource"},
    // bottom input
    {"EV_TIMEOUT",      0,  0,  0,  0},
    // internal
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_WRITE_RESOURCE_ACK",   EVF_PUBLIC_EVENT,   0, 0, "Write resource ack"},
    {"EV_READ_RESOURCE_ACK",    EVF_PUBLIC_EVENT,   0, 0, "Read resource ack"},
    {"EV_DELETE_RESOURCE_ACK",  EVF_PUBLIC_EVENT,   0, 0, "Delete resource ack"},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_WRITE_RESOURCE",       ac_update_resource,      0},
    {"EV_READ_RESOURCE",        ac_list_resource,       0},
    {"EV_DELETE_RESOURCE",      ac_delete_resource,     0},
    {"EV_TIMEOUT",              ac_timeout,             0},
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
    GCLASS_STORE_NAME,
    &fsm,
    {
        mt_create,
        mt_destroy,
        mt_start,
        mt_stop,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_resource_bin,
        0, //mt_resource_desc,
        0, //mt_play,
        0, //mt_pause,
        0, //mt_resource_data,
        0, //mt_child_added,
        0, //mt_child_removed,
        mt_stats,
        0, //mt_command,
        0, //mt_create2,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_authorization_parser,
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
        0, //mt_future60,
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
PUBLIC GCLASS *gclass_store(void)
{
    return &_gclass;
}
