/***********************************************************************
 *          C_SNMP.C
 *          Implementation of the SNMP protocol.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <stdint.h>
#include "c_snmp.h"

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
SDATA (ASN_OCTET_STR,   "url",      SDF_RD,  0, "snmp server url"),
SDATA (ASN_OCTET_STR,   "lHost",    SDF_RD,  0, "Listening ip, got from url"),
SDATA (ASN_OCTET_STR,   "lPort",    SDF_RD,  0, "Listening port, got from url"),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_DEBUG = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"debug",        "Trace debug"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj srv_udp;
    hgobj asn1_tlv;
} PRIVATE_DATA;




            /***************************
             *      Framework Methods
             ***************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    json_t *kw;

    const char *url = gobj_read_str_attr(gobj, "url");
    if(empty_string(url)) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "url NULL",
            NULL
        );
        return;
    }

    /*-----------------------------------------------*
     *  Create the stack of gobjs for process snmp:
     *      TLV
     *      UPD
     *-----------------------------------------------*/
    /*
     *  ASN1_TLV
     */
    priv->asn1_tlv = gobj_create("snmp", GCLASS_TLV, 0, gobj);

    /*
     *  SRV_UPD
     */
    kw = json_pack("{s:s}",
        "url", url
    );
    priv->srv_udp = gobj_create("snmp", GCLASS_UDP_S0, kw, priv->asn1_tlv);
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->asn1_tlv)
        gobj_start(priv->asn1_tlv);
    if(priv->srv_udp)
        gobj_start(priv->srv_udp);
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->asn1_tlv)
        gobj_stop(priv->asn1_tlv);
    if(priv->srv_udp)
        gobj_stop(priv->srv_udp);
    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
//    PRIVATE_DATA *priv = gobj_priv_data(gobj);

}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  handle GET
 ***************************************************************************/
PRIVATE int handle_snmp_get(hgobj gobj, int version, TLV *varbind)
{
    oid_t toid[MAX_OID_LEN+4];
    int toid_len = MAX_OID_LEN+4;

    // reset input
    memset(toid, 0, sizeof(toid)/sizeof(oid_t));
    toid_len = MAX_OID_LEN+4;

    if(snmp_get_varbind_oid(varbind, toid, &toid_len)<0) {
        return SNMP_ERR_GENERR;
    }

    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        char bfoid[256];
        snprint_oid(
            bfoid,
            sizeof(bfoid),
            toid,
            toid_len
        );
        //printf("====> GET %s\n", bfoid);
    }

    int index;
    sdata_desc_t *it;
    hgobj obj_found;
    hsdata hs_found;

    int ret = gobj_traverse_by_oid(
        gobj,
        toid,
        toid_len,
        &it,
        &index,
        &obj_found,
        &hs_found
    );
    if(ret<=0) {
        if(gobj_trace_level(gobj) & TRACE_DEBUG) {
            log_debug_printf("snmp", "<=== NOT FOUND");
        }
        if(version == SNMP_VERSION_1) {
            return SNMP_ERR_NOSUCHNAME;
        } else {
            snmp_set_varbind_value_empty(varbind, SNMP_NOSUCHOBJECT);
            return SNMP_ERR_NOERROR;
        }
    }

    if(!SDF_IS_SNMP_READABLE(it->flag)) {
        if(gobj_trace_level(gobj) & TRACE_DEBUG) {
            log_debug_printf("snmp", "<=== NOT READABLE %s", it->name);
        }
        if(version == SNMP_VERSION_1) {
            return SNMP_ERR_GENERR;
        } else {
            return SNMP_ERR_NOACCESS;
        }
    }

    void *ptr = sdata_it_pointer(hs_found, it, index);
    if(!ptr) {
        return SNMP_ERR_GENERR;
    }

    /*
     *  SNMP variable found. Can be a scalar type, or a table type.
     */
    if(!SDF_IS_SNMP(it->flag)) {
        return SNMP_ERR_GENERR;
    }
    if(!ASN_IS_SCHEMA(it->type)) {
        if(gobj_trace_level(gobj) & TRACE_DEBUG) {
            log_debug_printf("snmp", "<=== GOT %s from %s", it->name, gobj_full_name(obj_found));
            print_hsdata_item("<=== GOT", hs_found, it, ptr);
        }
        snmp_set_varbind_value_from_sdata(varbind, it, hs_found, ptr);
        return SNMP_ERR_NOERROR;
    }

    /*
     *  Table: search the cell
     */
    //it = (it->pointer_col_cb)(obj_found, row, name);

    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        log_debug_printf("snmp", "<=== GOT %s from %s", it->name, gobj_full_name(obj_found));
        print_hsdata_item("<=== GOT", hs_found, it, ptr);
    }
    snmp_set_varbind_value_from_sdata(varbind, it, hs_found, ptr);
    return SNMP_ERR_NOERROR;
}

/***************************************************************************
 *  handle SET
 ***************************************************************************/
PRIVATE int handle_snmp_set(hgobj gobj, int version, TLV *varbind)
{
    oid_t toid[MAX_OID_LEN+4];
    int toid_len = MAX_OID_LEN+4;

    // reset input
    memset(toid, 0, sizeof(toid)/sizeof(oid_t));
    toid_len = MAX_OID_LEN+4;

    if(snmp_get_varbind_oid(varbind, toid, &toid_len)<0) {
        return SNMP_ERR_GENERR;
    }

    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        char bfoid[256];
        snprint_oid(
            bfoid,
            sizeof(bfoid),
            toid,
            toid_len
        );
        log_debug_printf("snmp", "====> SET %s", bfoid);
    }

    int index;
    sdata_desc_t *it;
    hgobj obj_found;
    hsdata hs_found;

    int ret = gobj_traverse_by_oid(
        gobj,
        toid,
        toid_len,
        &it,
        &index,
        &obj_found,
        &hs_found
    );
    if(ret<=0) {
        if(version == SNMP_VERSION_1) {
            return SNMP_ERR_NOSUCHNAME;
        } else {
            snmp_set_varbind_value_empty(varbind, SNMP_NOSUCHOBJECT);
            return SNMP_ERR_NOERROR;
        }
    }
    if(!SDF_IS_SNMP_WRITABLE(it->flag)) {
        if(version == SNMP_VERSION_1) {
            return SNMP_ERR_GENERR;
        } else {
            return SNMP_ERR_NOACCESS;
        }
    }

    TLV *tlv_value = snmp_get_varbind_value(varbind);
    if(!tlv_value) {
        return SNMP_ERR_GENERR;
    }

    if(!ASN_COMPATIBLE_TYPES(tlv_value->type, it->type)) {
        return SNMP_ERR_GENERR;
    }

    void *ptr = sdata_it_pointer(hs_found, it, index);

    /*
     *  Write in sdata
     */
    if(snmp_write_sdata_from_varbind(it, hs_found, ptr, tlv_value)<0) {
        return SNMP_ERR_GENERR;
    }

    /*
     *  Re-read from sdata to build the response, also checking the write.
     */
    return handle_snmp_get(gobj, version, varbind);
}

/***************************************************************************
 *  handle GETNEXT
 ***************************************************************************/
PRIVATE int handle_snmp_getnext(hgobj gobj, int version, TLV *varbind)
{
    oid_t toid[MAX_OID_LEN+4];
    int toid_len = MAX_OID_LEN+4;

    // reset input
    memset(toid, 0, sizeof(toid)/sizeof(oid_t));
    toid_len = MAX_OID_LEN+4;

    if(snmp_get_varbind_oid(varbind, toid, &toid_len)<0) {
        return SNMP_ERR_GENERR;
    }

    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        char bfoid[256];
        snprint_oid(
            bfoid,
            sizeof(bfoid),
            toid,
            toid_len
        );
        log_debug_printf("snmp", "====> GETNEXT %s", bfoid);
    }

    int index;
    sdata_desc_t *it;
    hgobj obj_found;
    hsdata hs_found;

    int ret = gobj_traverse_by_oid(
        gobj,
        toid,
        toid_len,
        &it,
        &index,
        &obj_found,
        &hs_found
    );
    if(ret<0) {
        /*
         *  OID not found, return the first oid in the yuno tree
         *  if the prefix match with us.
         */
        obj_found = gobj_yuno();
        toid_len = MAX_OID_LEN+4; // Reset input value
        gobj_oid(obj_found, toid, &toid_len);
        if(toid_len == 0) {
            if(version == SNMP_VERSION_1) {
                return SNMP_ERR_NOSUCHNAME;
            } else {
                snmp_set_varbind_value_empty(varbind, SNMP_ENDOFMIBVIEW);
                return SNMP_ERR_NOERROR;
            }
        }
    }

    /*
     *  OID found, return the next oid.
     */
    ret = gobj_next_oid(
        obj_found,
        toid,
        &toid_len
    );
    if(ret <= 0) {
        if(version == SNMP_VERSION_1) {
            return SNMP_ERR_NOSUCHNAME;
        } else {
            snmp_set_varbind_value_empty(varbind, SNMP_ENDOFMIBVIEW);
            return SNMP_ERR_NOERROR;
        }
    }
    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        char bfoid[256];
        snprint_oid(
            bfoid,
            sizeof(bfoid),
            toid,
            toid_len
        );
        log_debug_printf("snmp", "<==== GETNEXT %s", bfoid);
    }

    /*
     *  Set varbind oid
     */
    snmp_set_varbind_oid(varbind, toid, toid_len);

    /*
     *  read from sdata the new next oid
     */
    return handle_snmp_get(gobj, version, varbind);
}

/***************************************************************************
 *  expand getbulk
 ***************************************************************************/
PRIVATE int expand_getbulk(hgobj gobj, TLV *tlv)
{
    int j;
    int non_repeaters=0;
    int max_repetitions=0;

    if(gobj_trace_level(gobj) & TRACE_DEBUG) {
        log_debug_printf("snmp", "====> GETBULK");
    }
    snmp_get_non_repeaters(tlv, &non_repeaters);
    snmp_get_max_repetitions(tlv, &max_repetitions);

    TLV *varbind_list = snmp_get_varbind_list(tlv);

    /*
     *  Change to GETNEXT pdu type
     */
    snmp_set_pdu_type(tlv, SNMP_MSG_GETNEXT);

    /*
     *  Copy the original varbind list
     */
    dl_list_t original_dl_sequence;
    dl_move(&original_dl_sequence, &varbind_list->dl_sequence);

    /*
     *  Check limits
     */
    int max_varbind_list = dl_size(&original_dl_sequence);
    non_repeaters = MAX(0, non_repeaters);
    non_repeaters = MIN(non_repeaters, max_varbind_list);
    max_repetitions = MAX(0, max_repetitions);

    /*------------------------------------*
     *  Build a new varbind list
     *------------------------------------*/
    /*
     *  Move directly the non_repeaters items.
     */
    while(non_repeaters>0) {
        TLV *varbind = dl_first(&original_dl_sequence);
        dl_delete(&original_dl_sequence, varbind, 0);
        dl_add(&varbind_list->dl_sequence, varbind);
        non_repeaters--;
    }

    /*
     *  The repeaters are handled like with the GETNEXT request, except that:
     *
     * - the access is interleaved (i.e. first repetition of all varbinds,
     *   then second repetition of all varbinds, then third,...)
     * - the repetitions are aborted as soon as there is no successor found
     *   for all of the varbinds
     * - other than with getnext, the last variable in the MIB is named if
     *   the variable queried is not after the end of the MIB
     */
    for (j = 0; j < max_repetitions; j++) {
        int found_repeater = 0;
        TLV *varbind = dl_first(&original_dl_sequence);
        while(varbind) {
            /*
             *  Get the original oid
             */
            oid_t toid[MAX_OID_LEN+4];
            int toid_len = MAX_OID_LEN+4;

            // reset input
            memset(toid, 0, sizeof(toid)/sizeof(oid_t));
            toid_len = MAX_OID_LEN+4;

            if(snmp_get_varbind_oid(varbind, toid, &toid_len)<0) {
                return SNMP_ERR_GENERR;
            }

            /*
             *  Find the gobj
             */
            int index;
            sdata_desc_t *it;
            hgobj obj_found;
            hsdata hs_found;

            int ret = gobj_traverse_by_oid(
                gobj,
                toid,
                toid_len,
                &it,
                &index,
                &obj_found,
                &hs_found
            );
            if(ret < 0) {
                /*
                 *  Not found, get the yuno root
                 */
                hgobj obj_found = gobj_yuno();
                gobj_oid(obj_found, toid, &toid_len);
            }

            /*
             *  Save as new oid
             */
            TLV *new_varbind = snmp_new_varbind();
            dl_add(&varbind_list->dl_sequence, new_varbind);
            snmp_set_varbind_oid(new_varbind, toid, toid_len);

            /*
             *  Check if there is a GetNext
             */
            ret = gobj_next_oid(
                obj_found,
                toid,
                &toid_len
            );
            if(ret > 0) {
                /*
                 *  Save in the original oid for the next cycle
                 */
                found_repeater++;
                snmp_set_varbind_oid(varbind, toid, toid_len);
            }

            /*
             *  Next item
             */
            varbind = dl_next(varbind);
        }

        if (found_repeater == 0) {
            break;
        }
    }

    /*
     *  Delete the original varbind_list
     */
    TLV *child;
    while((child = dl_first(&original_dl_sequence))) {
        dl_delete(&original_dl_sequence, child, 0);
        tlv_delete(child);
    }
    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_process_tlv(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    const char *session_name = kw_get_str(kw, "session_name", 0, 0);
    TLV *tlv = (TLV *)kw_get_int(kw, "tlv", 0, FALSE);
    int error_status=0;
    int error_index=0;

    /*--------------------------*
     *  Check version
     *--------------------------*/
    int version;
    if(snmp_get_version(tlv, &version)<0) {
        goto end; // Ignore the request
    }
    if(!(version == SNMP_VERSION_1 || version == SNMP_VERSION_2c)) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "WRONG snmp VERSION",
            "version",      "%s", version,
            NULL
        );
        error_status = SNMP_ERR_GENERR;
        error_index = 0;
        goto end; // Ignore the request. Don't respond error, like net-snmp behaviour.
    }

    /*----------------------*
     *  Check community
     *----------------------*/
    char *community = snmp_get_community(tlv);
    if(!community) {
        goto end; // Ignore the request
    }
    if(strcmp(community, "public")!=0) { //TODO config community
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "WRONG snmp COMMUNITY",
            "community",    "%s", community,
            NULL
        );
        if(version == SNMP_VERSION_2c)
            error_status = SNMP_ERR_NOACCESS;
        else
            error_status = SNMP_ERR_GENERR;
        error_index = 0;
        goto end; // Ignore the request. Don't respond error, like net-snmp behaviour.
    }

    /*-----------------------*
     *  Check PDU type
     *-----------------------*/
    uint8_t pdu_type;
    if(snmp_get_pdu_type(tlv, &pdu_type)<0) {
        error_status = SNMP_ERR_GENERR;
        error_index = 0;
        goto end; // Ignore the request
    }

    /*-----------------------------------------------*
     *  IF GETBULK pdu, expand to get commands
     *-----------------------------------------------*/
    if(pdu_type == SNMP_MSG_GETBULK) {
        if(expand_getbulk(gobj, tlv)<0) {
            error_status = SNMP_ERR_GENERR;
            error_index = 0;
            goto end; // Ignore the request
        }
        pdu_type = SNMP_MSG_GETNEXT;
    }

    /*----------------------------*
     *  Process varbind list
     *----------------------------*/
    int varbind_list_size = snmp_get_varbind_list_size(tlv);
    for(int i=0; i<varbind_list_size; i++) {
        TLV *varbind = snmp_get_varbind(tlv, i+1);
        if(!varbind) {
            error_status = SNMP_ERR_GENERR;
            error_index = i+1;
            goto response;
        }

        switch(pdu_type) {
        case SNMP_MSG_GET:
            error_status = handle_snmp_get(
                gobj,
                version,
                varbind
            );
            if(error_status) {
                error_index = i+1;
                goto response;
            }
            break;

        case SNMP_MSG_GETNEXT:
            error_status = handle_snmp_getnext(
                gobj,
                version,
                varbind
            );
            if(error_status) {
                error_index = i+1;
                goto response;
            }
            break;

        case SNMP_MSG_SET:
            error_status = handle_snmp_set(
                gobj,
                version,
                varbind
            );
            if(error_status) {
                error_index = i+1;
                goto response;
            }
            break;

        default:
            error_status = SNMP_ERR_GENERR;
            error_index = 0;
            goto end; // Ignore the request
        }
    }

response:
    {
        GBUFFER *gbuf_response = build_snmp_response(tlv, error_status, error_index);
        gbuf_setlabel(gbuf_response, session_name);
        json_t *kw_ex = json_pack("{s:I}",
            "gbuffer", (json_int_t)(size_t)gbuf_response
        );
        gobj_send_event(priv->srv_udp, "EV_TX_DATA", kw_ex, gobj);
    }

end:
    tlv_delete(tlv);
    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_new_session(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     const char *session_name = kw_get_str(kw, "session_name", 0);

    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_del_session(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     const char *session_name = kw_get_str(kw, "session_name", 0);

    JSON_DECREF(kw);
    return 1;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_NEW_SESSION", 0},
    {"EV_DEL_SESSION", 0},
    {"EV_RX_TLV", 0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_RX_TLV",           ac_process_tlv,          0},
    {"EV_NEW_SESSION",      ac_new_session,          0},
    {"EV_DEL_SESSION",      ac_del_session,          0},
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
    GCLASS_SNMP_NAME,
    &fsm,
    {
        mt_create,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_writing,
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
        0, //mt_stats,
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
        0, //mt_authz_allow,
        0, //mt_authz_deny,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_future38,
        0, //mt_authzs,
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
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_snmp(void)
{
    return &_gclass;
}
