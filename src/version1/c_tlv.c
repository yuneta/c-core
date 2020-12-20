/***********************************************************************
 *          C_ASN1_TLV.C
 *          BER (Basic Encoding Rules) TLV (Type, Length, Value)
 *          ASN.1 (Abstract Syntax Notation.One) used by snmp/ldap
 *
 *          Output events go to parent! (no broadcast)
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <stdint.h>
#include "c_tlv.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/
typedef enum {
    TLV_WAIT_TYPE,
    TLV_WAIT_LEN,
    TLV_WAIT_DATA
} TLV_STATE;

#define LONG_LENGTH     0x80
#define HIGH_BIT        0x80
#define EXTENSION_ID    0x1F
#define CONSTRUCTOR     0x20

    /**
     * Defines the BER extension "value" that is used to mark an extension type.
     */

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _ITL { // Input "Type Length"
    int type;
    int length;
    GBUFFER *gbuf;

    // TL mini-buffer for long length
    uint8_t ll[sizeof(long)];
    int ll_idx;
    int ll_len;

    // state of frame
    TLV_STATE tlv_state;

    BOOL complete;
} ITL;

typedef struct _UDP_SESSION {
    DL_ITEM_FIELDS

    char *session_name;  /* session name is peer "ip:port" */
    int inactivity_timeout;

    ITL itl;   /* itl for processing input snmp message */
} UDP_SESSION;


/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE void reset_itl(ITL *tlv);
PRIVATE UDP_SESSION *udp_session_new(hgobj gobj, const char *session_name);
PRIVATE void udp_session_delete(hgobj gobj, UDP_SESSION *upd_session);
PRIVATE UDP_SESSION * udp_session_find(hgobj gobj, const char *session_name);

PRIVATE int asn_parse_length(uint8_t *data, int data_len);
PRIVATE int consume_tlv(ITL *itl, GBUFFER *gbuf);
PRIVATE int tlv_expand(TLV *tlv);

PRIVATE TLV * tlv_find(TLV *tlv, int *indices, int indices_len);

PRIVATE int tlv_build_length(GBUFFER *gbuf, int length);
PRIVATE int tlv_shrink(GBUFFER *gbuf, TLV *tlv);

PRIVATE int tlv_build_asn1_signed32(TLV *tlv, uint8_t type, int integer);
PRIVATE int tlv_parse_asn1_signed32(TLV *tlv, int *value);

PRIVATE int tlv_build_asn1_unsigned32(TLV *tlv, uint8_t type, unsigned long value);
PRIVATE int tlv_parse_asn1_unsigned32(TLV *tlv, unsigned int *value);

PRIVATE int tlv_build_asn1_unsigned64(TLV *tlv, uint8_t type, size_t value);
//PRIVATE int tlv_parse_asn1_unsigned64(TLV *tlv, size_t *value);

PRIVATE int tlv_build_asn1_string(TLV *tlv, uint8_t type, const char *str, int len);
PRIVATE char *tlv_parse_asn1_string(TLV *tlv);

PRIVATE int tlv_build_asn1_oid(TLV *tlv, uint8_t type, const oid_t *oid, int len);
PRIVATE int tlv_parse_asn1_oid(TLV *tlv, oid_t *poid, int *oid_len);

PRIVATE int tlv_build_asn1_empty(TLV *tlv, uint8_t type);

PRIVATE int tlv_read_type(TLV *tlv, int *indices, int indices_len, uint8_t *value);
PRIVATE int tlv_read_integer(TLV *tlv, int *indices, int indices_len, int *value);
PRIVATE char *tlv_read_string(TLV *tlv, int *indices, int indices_len);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_UNSIGNED,    "inactt",       SDF_RD,  5, "inactivity timeout in sec"),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_DEBUG = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"debug",        "Trace to debug"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj timer;

    dl_list_t dl_udp_session;
    uint32_t inactt;
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

    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);
    dl_init(&priv->dl_udp_session);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(inactt,    gobj_read_uint32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(inactt,    gobj_read_uint32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, 1*1000);
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_SESSION *udp_session;

    while((udp_session=dl_first(&priv->dl_udp_session))!=0) {
        udp_session_delete(gobj, udp_session);
    }
    clear_timeout(priv->timer);
    gobj_stop(priv->timer);
    return 0;
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void reset_itl(ITL *itl)
{
    itl->type = 0;
    itl->length = 0;
    GBUF_DECREF(itl->gbuf);

    memset(itl->ll, 0, sizeof(itl->ll));
    itl->ll_idx = 0;
    itl->ll_len = 0;

    // state of frame
    itl->tlv_state = TLV_WAIT_TYPE;

    itl->complete = FALSE;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE UDP_SESSION *udp_session_new(hgobj gobj, const char *session_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_SESSION *us;

    us = gbmem_malloc(sizeof(UDP_SESSION));
    if(!us) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_MEMORY_ERROR,
            "msg",              "%s", "no memory for sizeof(UDP_SESSION)",
            "sizeof(UDP_SESSION)",  "%d", sizeof(UDP_SESSION),
            NULL
        );
        return 0;
    }

    GBMEM_STR_DUP(us->session_name, session_name);
    us->inactivity_timeout = start_sectimer(priv->inactt);
    dl_add(&priv->dl_udp_session, us);
    reset_itl(&us->itl);

    json_t *kw_ex = json_pack("{s:s}",
        "session", session_name
    );
    gobj_send_event(gobj_parent(gobj), "EV_NEW_SESSION", kw_ex, gobj);

    return us;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void udp_session_delete(hgobj gobj, UDP_SESSION *udp_session)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *kw_ex = json_pack("{s:s}",
        "session", udp_session->session_name
    );
    gobj_send_event(gobj_parent(gobj), "EV_DEL_SESSION", kw_ex, gobj);

    dl_delete(&priv->dl_udp_session, udp_session, 0);

    GBMEM_FREE(udp_session->session_name);
    GBMEM_FREE(udp_session);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE UDP_SESSION * udp_session_find(hgobj gobj, const char *session_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    register UDP_SESSION *us = dl_first(&priv->dl_udp_session);
    while(us) {
        if(strcmp(us->session_name, session_name)==0) {
            return us;
        }
        us = dl_next(us);
    }
    return us;
}

/***************************************************************************
 *  asn_parse_length - return the long length.
 ***************************************************************************/
PRIVATE int asn_parse_length(uint8_t *data, int data_len)
{
    int length = 0;

    while (data_len--) {
        length <<= 8;
        length |= *data++;
    }
    return length;
}

/***************************************************************************
 *  Return -1 if any error
 ***************************************************************************/
PRIVATE int consume_tlv(ITL *itl, GBUFFER *gbuf)
{
    /*
     *  Processing type
     */
    if(itl->tlv_state == TLV_WAIT_TYPE) {
        uint8_t *type = gbuf_get(gbuf, 1);
        if(!type) { // No data?
            return -1;
        }
        itl->type = *type;
        itl->tlv_state = TLV_WAIT_LEN;
    }

    /*
     *  Processing length
     */
    if(itl->tlv_state == TLV_WAIT_LEN) {
        uint8_t *length = gbuf_get(gbuf, 1);
        if(!length) { // No more data!!
            return 0;
        }
        if(itl->ll_idx == 0) {
            /*
             *  First time in this state
             */
            if (*length & 0x80) {
                /*
                 * long length
                 */
                itl->ll_len = *length & ~0x80;
                itl->ll_idx = 0;
                if(itl->ll_len == 0 || itl->ll_len > sizeof(itl->ll)) {
                    // indefinite length or too long
                    return -1;
                }
            } else {
                /*
                 * short length
                 */
                itl->ll[itl->ll_idx++] = *length;
                itl->ll_len = 1;
            }
        }
        int need = itl->ll_len - itl->ll_idx;
        while(need>0) {
            uint8_t *byte =  gbuf_get(gbuf, 1);
            if(!byte) { // No more data!!
                return 0;
            }
            itl->ll[itl->ll_idx++] = *byte;
            need--;
        }
        itl->length = asn_parse_length(itl->ll, itl->ll_len);
        itl->tlv_state = TLV_WAIT_DATA;
    }

    /*
     *  Processing data
     */
    if(itl->tlv_state == TLV_WAIT_DATA) {
        if(!itl->gbuf) {
            /*
             *  Must be the first time here.
             */
            itl->gbuf = gbuf_create(
                itl->length,
                itl->length,
                0,
                0
            );
            if(!itl->gbuf) {
                log_error(0,
                    "gobj",         "%s", "TLV",
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_MEMORY_ERROR,
                    "msg",          "%s", "no memory for ITL data",
                    "len",          "%d", itl->length,
                    NULL
                );
                return -1;
            }
        }
        int available = gbuf_leftbytes(gbuf);
        int need = gbuf_freebytes(itl->gbuf);
        int ln = MIN(need, available);
        if(ln > 0) {
            uint8_t *data = gbuf_get(gbuf, ln);
            gbuf_append(itl->gbuf, data, ln);
        }
        if(gbuf_freebytes(itl->gbuf) == 0) {
            itl->complete = TRUE;
        }
    }
    return 0;
}

/***************************************************************************
 *  Convert gbuf in simple or complex type, recursive until exhaust the gbuf
 ***************************************************************************/
//static int level = 0;
PRIVATE int tlv_expand(TLV *tlv)
{
    TLV *child;
    ITL itl;

    //level++;

    memset(&itl, 0, sizeof(ITL));

    if(IS_CONSTRUCTOR(tlv->type)) {
        /*
         *  Expand Complex types
         */
        //printf("%*.*sX-Type %02X, ln %02X\n", level,level," ", tlv->type, tlv->length);
        while(1) {
            reset_itl(&itl);
            int ret = consume_tlv(&itl, tlv->gbuf);
            if(ret < 0) {
                return -1;
            }
            if(itl.complete) {
                child = tlv_create(itl.type, itl.length);
                if(!child) {
                    log_error(0,
                        "gobj",         "%s", "TLV",
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_MEMORY_ERROR,
                        "msg",          "%s", "tlv_create() FAILED",
                        NULL
                    );
                    return -1;
                }
                child->gbuf = itl.gbuf;   // give gbuff to tlv
                itl.gbuf = 0;
                dl_add(&tlv->dl_sequence, child);

                /*
                *  Reset for new frame
                */
                reset_itl(&itl);
            }
            int available = gbuf_leftbytes(tlv->gbuf);
            if(!available) {
                // no more data
                gbuf_decref(tlv->gbuf);
                tlv->gbuf = 0;
                break;
            }
        }
        /*
         *  Expand childs
         */
        child = dl_first(&tlv->dl_sequence);
        while(child) {
            int ret = tlv_expand(child);
            if(ret < 0) {
                // Some error
                return -1;
            }
            child = dl_next(child);
        }
    } else {
        /*
         *  Simple type
         */
        // Let the data in gbuf
        //printf("%*.*sS-Type %02X, ln %02X\n", level,level," ", tlv->type, tlv->length);
    }

    //level--;
    return 0;
}

/***************************************************************************
 *  Read the type of tlv
 ***************************************************************************/
PRIVATE TLV * tlv_find(TLV *tlv, int *indices, int indices_len)
{
    int ind = *indices;
    if(ind>0) {
        /*
         *  walk down
         */
        TLV *child = (TLV *)dl_nfind(&tlv->dl_sequence, ind);
        if(!child) {
            log_error(0,
                "gobj",         "%s", "TLV",
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SNMP_ERROR,
                "msg",          "%s", "int child NOT FOUND",
                NULL
            );
            return 0;
        }
        indices++;
        indices_len--;
        return tlv_find(child, indices, indices_len);
    } else {
        /*
         *  Found
         */
        return tlv;
    }
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_length(GBUFFER *gbuf, int length)
{
    int ln = 0;
    uint8_t byte;

    if (length <= 0x7f) {
        byte = (uint8_t)length;
        gbuf_append(gbuf, &byte, 1);
        ln = 1;
    } else if (length <= 0xff) {
        byte = (uint8_t)(0x01 | LONG_LENGTH);
        gbuf_append(gbuf, &byte, 1);
        byte = (uint8_t)(length & 0xff);
        gbuf_append(gbuf, &byte, 1);
        ln = 2;
    } else if(length <= 0xffff) {
        byte = (uint8_t)(0x02 | LONG_LENGTH);
        gbuf_append(gbuf, &byte, 1);
        byte = (uint8_t)((length >> 8) & 0xff);
        gbuf_append(gbuf, &byte, 1);
        byte = (uint8_t)(length & 0xff);
        gbuf_append(gbuf, &byte, 1);
        ln = 3;
    } else {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "length TOO LONG",
            "value",        "%d", length,
            NULL
        );
        return 0;
    }

    return ln;
}

/***************************************************************************
 *  Shrink all childs to his parent
 *  Return the length of the builtin gbuffer
 ***************************************************************************/
PRIVATE int tlv_shrink(GBUFFER *gbuf_parent, TLV *tlv)
{
    TLV *child;

    if(IS_CONSTRUCTOR(tlv->type)) {
        //printf("CONSTRUCTOR type %2X\n", tlv->type);
        /*
         *  Create gbuf
         */
        if(tlv->gbuf) {
            log_error(0,
                "gobj",         "%s", "TLV",
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SNMP_ERROR,
                "msg",          "%s", "gbuf MUST be NULL",
                NULL
            );
            GBUF_DECREF(tlv->gbuf);
        }
        tlv->gbuf = gbuf_create(256, SNMP_MAX_MSG_SIZE, 0, 0);
        if(!tlv->gbuf) {
            log_error(0,
                "gobj",         "%s", "TLV",
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SNMP_ERROR,
                "msg",          "%s", "gbuf_create() FAILED",
                NULL
            );
            return 0;
        }

        /*
         *  Shrink childs
         */
        tlv->length = 0;
        child = dl_first(&tlv->dl_sequence);
        while(child) {
            tlv->length += tlv_shrink(tlv->gbuf, child);
            child = dl_next(child);
        }
    }

    int ln = 1;
    gbuf_append(gbuf_parent, &tlv->type, 1);
    if(tlv->length != gbuf_leftbytes(tlv->gbuf)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "MISMATCH lengths",
            "tlv->length",  "%d", tlv->length,
            "gbuf->ln",     "%d", gbuf_leftbytes(tlv->gbuf),
            NULL
        );
    }
    ln += tlv_build_length(gbuf_parent, tlv->length);
    ln += gbuf_leftbytes(tlv->gbuf);

    gbuf_append_gbuf(gbuf_parent, tlv->gbuf);
    GBUF_DECREF(tlv->gbuf);

    return ln;
}

/*********************************************************************
 *  Build integer
 *  I use the algorithm of mini-snmpd
 *  It's better simple than net-snmp or openNMS
 *********************************************************************/
PRIVATE int tlv_build_asn1_signed32(TLV *tlv, uint8_t type, int integer)
{
    if(type != ASN_INTEGER) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv type MUST be ASN_INTEGER",
            NULL
        );
        return -1;
    }
    tlv->type = type;

    int length;

    if (integer < -8388608 || integer > 8388607) {
        length = 4;
    } else if (integer < -32768 || integer > 32767) {
        length = 3;
    } else if (integer < -128 || integer > 127) {
        length = 2;
    } else {
        length = 1;
    }
    tlv->length = length;

    GBUF_DECREF(tlv->gbuf);
    tlv->gbuf = gbuf_create(length, length, 0, 0);
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }

    while (length--) {
        uint8_t byte = ((unsigned int)integer >> (8 * length)) & 0xFF;
        gbuf_append(tlv->gbuf, &byte, 1);
    }

    return 0;
}

/*********************************************************************
 *  Parse integer
 *********************************************************************/
PRIVATE int tlv_parse_asn1_signed32(TLV *tlv, int *value)
{
    if(tlv->type != ASN_INTEGER) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "ans type MUST be ASN_INTEGER",
            NULL
        );
        return -1;
    }
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "leaf tlv WITHOUT gbuf",
            NULL
        );
        return -1;
    }
    if(tlv->length != gbuf_leftbytes(tlv->gbuf)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "lengths MISMATCH",
            NULL
        );
        return -1;
    }
    int length = tlv->length;
    char *bf = gbuf_cur_rd_pointer(tlv->gbuf);

    unsigned int tmp_value;
    /* Fetch the value as unsigned integer (copy sign bit into all bytes first) */
    memset(&tmp_value, (*bf & 0x80) ? 0xFF : 0x00, sizeof (tmp_value));
    while (length--) {
        tmp_value = (tmp_value << 8) | *bf;
        bf++;
    }
    *(int *)value = tmp_value;

    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_asn1_unsigned32(TLV *tlv, uint8_t type, unsigned long value)
{
    if(!(type==ASN_UNSIGNED ||
            type==ASN_TIMETICKS ||
            type==ASN_COUNTER)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "UNSIGNED type INVALID!",
            "type",         "%d", type,
            NULL
        );
        return -1;
    }

    tlv->type = type;

    int i;
    #define OCTETS 9    // With this size we can build until uint64_t
    uint8_t buf[OCTETS];

    /* split the value into octets */
    for (i = OCTETS - 1; i >= 0; i--) {
        buf[i] = value & 0xff;
        value >>= 8;
    }
    /* no leading 9 zeroes */
    for (i = 0; i < OCTETS - 1; i++)
        if (!(buf[i] == 0x00 && (buf[i + 1] & 0x80) == 0))
            break;
    int intsize = OCTETS - i;
    tlv->length = intsize;

    GBUF_DECREF(tlv->gbuf);

    tlv->gbuf = gbuf_create(intsize, intsize, 0, 0);
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }

    while (i < OCTETS) {
        gbuf_append(tlv->gbuf, &buf[i++], 1);
    }
    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_parse_asn1_unsigned32(TLV *tlv, unsigned int *value)
{
    if(!(tlv->type==ASN_UNSIGNED ||
            tlv->type==ASN_TIMETICKS ||
            tlv->type==ASN_COUNTER)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "UNSIGNED type INVALID!",
            "type",         "%d", tlv->type,
            NULL
        );
        return -1;
    }
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "leaf tlv WITHOUT gbuf",
            NULL
        );
        return -1;
    }
    if(tlv->length != gbuf_leftbytes(tlv->gbuf)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "lengths MISMATCH",
            NULL
        );
        return -1;
    }

    int length = tlv->length;
    char *bf = gbuf_cur_rd_pointer(tlv->gbuf);

    unsigned int tmp_value = 0;

    while (length--) {
        tmp_value = (tmp_value << 8) | *bf;
        bf++;
    }
    *(unsigned *)value = tmp_value;

    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_asn1_unsigned64(TLV *tlv, uint8_t type, size_t value)
{
    if(!(type==ASN_COUNTER64)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "UNSIGNED64 type INVALID!",
            "type",         "%d", type,
            NULL
        );
        return -1;
    }

    tlv->type = type;

    int i;
    #define OCTETS 9    // With this size we can build until uint64_t
    uint8_t buf[OCTETS];

    /* split the value into octets */
    for (i = OCTETS - 1; i >= 0; i--) {
        buf[i] = value & 0xff;
        value >>= 8;
    }
    /* no leading 9 zeroes */
    for (i = 0; i < OCTETS - 1; i++)
        if (!(buf[i] == 0x00 && (buf[i + 1] & 0x80) == 0))
            break;
    int intsize = OCTETS - i;
    tlv->length = intsize;

    GBUF_DECREF(tlv->gbuf);

    tlv->gbuf = gbuf_create(intsize, intsize, 0, 0);
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }

    while (i < OCTETS) {
        gbuf_append(tlv->gbuf, &buf[i++], 1);
    }
    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
// PRIVATE int tlv_parse_asn1_unsigned64(TLV *tlv, size_t *value)
// {
//     int length = tlv->length;
//     char *bf = gbuf_cur_rd_pointer(tlv->gbuf);
//
//     /*
//      *  check to see that we can actually decode the value
//      */
//     int max_size = sizeof(size_t) + 1;
//     if(length > max_size) {
//         log_error(0,
//             "gobj",         "%s", "TLV",
//             "function",     "%s", __FUNCTION__,
//             "msgset",       "%s", MSGSET_INTERNAL_ERROR,
//             "msg",          "%s", "Integer 64 TOO LARGE",
//             "type",         "%d", tlv->type,
//             NULL
//         );
//         return -1;
//     }
//
//     size_t tmp_value = 0;
//
//     while (length--) {
//         tmp_value = (tmp_value << 8) | *bf;
//         bf++;
//     }
//     *value = tmp_value;
//     return 0;
// }

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_asn1_string(TLV *tlv, uint8_t type, const char *str, int len)
{
    if(!(ASN_IS_STRING(type))) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "STRING type MISMATCH",
            "type",         "%d", tlv->type,
            NULL
        );
        return -1;
    }
    tlv->type = type;

    GBUF_DECREF(tlv->gbuf);

    tlv->gbuf = gbuf_create(len, len, 0, 0);
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }
    tlv->length = len;
    if(len > 0) {
        gbuf_append(tlv->gbuf, (char *)str, len);
    }
    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE char *tlv_parse_asn1_string(TLV *tlv)
{
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "leaf tlv WITHOUT gbuf",
            NULL
        );
        return 0;
    }
    if(tlv->length != gbuf_leftbytes(tlv->gbuf)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "lengths MISMATCH",
            NULL
        );
        return 0;
    }

    if(!(tlv->type == ASN_OCTET_STR||
            tlv->type == ASN_IPADDRESS||
            tlv->type == ASN_BIT_STR||
            tlv->type == ASN_OPAQUE)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "STRING type MISMATCH",
            "type",         "%d", tlv->type,
            NULL
        );
        return 0;
    }
    return gbuf_cur_rd_pointer(tlv->gbuf);
}

/*********************************************************************
 *  Create a varbind
 *********************************************************************/
PUBLIC TLV *snmp_new_varbind(void)
{
    TLV *varbind = tlv_create(ASN_SEQUENCE|ASN_CONSTRUCTOR, 0);
    if(!varbind) {
        return 0;
    }

    TLV *tlv_oid;
    tlv_oid = tlv_create(ASN_OBJECT_ID, 0);
    if(!tlv_oid) {
        tlv_delete(varbind);
        return 0;
    }
    dl_add(&varbind->dl_sequence, tlv_oid);

    TLV *tlv_value;
    tlv_value = tlv_create(ASN_NULL, 0);
    if(!tlv_value) {
        tlv_delete(varbind);
        return 0;
    }
    dl_add(&varbind->dl_sequence, tlv_value);

    return varbind;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_asn1_oid(TLV *tlv, uint8_t type, const oid_t *toid, int len)
{
    if(!(type == ASN_OBJECT_ID)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "OID type MISMATCH",
            "type",         "%d", tlv->type,
            NULL
        );
        return -1;
    }
    tlv->type = type;

    GBUF_DECREF(tlv->gbuf);

    int length;
    int i;

    length = 1;
    for (i = 2; i < len; i++) {
        if (toid[i] >= (1 << 28)) {
            length += 5;
        } else if (toid[i] >= (1 << 21)) {
            length += 4;
        } else if (toid[i] >= (1 << 14)) {
            length += 3;
        } else if (toid[i] >= (1 << 7)) {
            length += 2;
        } else {
            length += 1;
        }
    }
    if (length > 0xFFFF) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "OID overflow",
            "type",         "%d", tlv->type,
            NULL
        );
        return -1;
    }

    tlv->gbuf = gbuf_create(length, length, 0, 0);
    if(!tlv->gbuf) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }
    tlv->length = length;

    uint8_t byte;
    byte = toid[0] * 40 + toid[1];
    gbuf_append(tlv->gbuf, &byte, 1);

    for (i = 2; i < len; i++) {
        if (toid[i] >= (1 << 28)) {
            length = 5;
        } else if (toid[i] >= (1 << 21)) {
            length = 4;
        } else if (toid[i] >= (1 << 14)) {
            length = 3;
        } else if (toid[i] >= (1 << 7)) {
            length = 2;
        } else {
            length = 1;
        }
        while (length--) {
            if (length) {
                byte = ((toid[i] >> (7 * length)) & 0x7F) | 0x80;
                gbuf_append(tlv->gbuf, &byte, 1);
            } else {
                byte = (toid[i] >> (7 * length)) & 0x7F;
                gbuf_append(tlv->gbuf, &byte, 1);
            }
        }
    }

    return 0;
}

/*********************************************************************
 *  Save in `poid` the oid of tlv. TLV must be a varbind.
 *  `oid_len` is IN/OUT.
 *      In Input is size of `poid` (number of array's indices).
 *      In Output is the readed oid length saved in `poid`.
 *  Return 0 if succesfully or -1 if error.
 *********************************************************************/
PRIVATE int tlv_parse_asn1_oid(TLV *tlv, oid_t *poid, int *oid_len)
{
    if(tlv->length != gbuf_leftbytes(tlv->gbuf)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "MISMATCH lengths",
            "tlv->length",  "%d", tlv->length,
            "gbuf->ln",     "%d", gbuf_leftbytes(tlv->gbuf),
            NULL
        );
        *oid_len = 0;
        return -1;
    }
    int max_out_len = *oid_len;
    if(max_out_len < 2) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "oid_t array MUST BE > 2",
            NULL
        );
        *oid_len = 0;
        return -1;
    }
    int length = tlv->length;
    uint8_t *bf = gbuf_cur_rd_pointer(tlv->gbuf);
    int idsOff = 0;

    /*
     *  decode the first byte
     */
    {
        --length;
        uint8_t b = *bf; bf++;
        poid[idsOff++] = b / 40;
        poid[idsOff++] = b % 40;
    }

    /*
     *  decode the rest of the identifiers
     */
    while (length > 0) {
        oid_t oid = 0;
        BOOL done = FALSE;
        do {
            --length;
            uint8_t b = *bf; bf++;
            oid = (oid << 7) | (int) (b & 0x7f);
            if ((b & HIGH_BIT) == 0)
                done = TRUE;
        } while (!done);

        if(idsOff >= max_out_len) {
            log_error(0,
                "gobj",         "%s", "TLV",
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SNMP_ERROR,
                "msg",          "%s", "oid_t array TOO SHORT",
                NULL
            );
            *oid_len = idsOff;
            return -1;
        }
        poid[idsOff++] = oid;

    }
    *oid_len = idsOff;
    return 0;
}

/*********************************************************************
 *
 *********************************************************************/
PRIVATE int tlv_build_asn1_empty(TLV *tlv, uint8_t type)
{
    if(!(type == SNMP_NOSUCHOBJECT ||
            type == SNMP_NOSUCHINSTANCE ||
            type == SNMP_ENDOFMIBVIEW ||
            type == ASN_OPAQUE||
            type == ASN_NULL)) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv type MUST be ASN_NULL or similar",
            NULL
        );
        return -1;
    }
    tlv->type = type;
    tlv->length = 0;

    GBUF_DECREF(tlv->gbuf);
    tlv->gbuf = gbuf_create(0,0,0,0);

    return 0;
}




            /***************************
             *      Helpers
             ***************************/




/***************************************************************************
 *  Read the type of tlv
 ***************************************************************************/
PRIVATE int tlv_read_type(TLV *tlv, int *indices, int indices_len, uint8_t *value)
{
    TLV *child = tlv_find(tlv, indices, indices_len);
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv child NOT FOUND",
            NULL
        );
        return -1;
    } else {
        /*
         *  Found
         */
        *value = child->type;
        return 0;
    }
}

/***************************************************************************
 *  Read an int variable
 ***************************************************************************/
PRIVATE int tlv_read_integer(TLV *tlv, int *indices, int indices_len, int *value)
{
    TLV *child = tlv_find(tlv, indices, indices_len);
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv child NOT FOUND",
            NULL
        );
        return -1;
    } else {
        /*
         *  Found
         */
        return tlv_parse_asn1_signed32(child, value);
    }
}

/***************************************************************************
 *  Read an string variable
 ***************************************************************************/
PRIVATE char *tlv_read_string(TLV *tlv, int *indices, int indices_len)
{
    TLV *child = tlv_find(tlv, indices, indices_len);
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv child NOT FOUND",
            NULL
        );
        return 0;
    } else {
        /*
         *  Found
         */
        return tlv_parse_asn1_string(child);
    }
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_process_udp(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);
    const char *peername = gbuf_getlabel(gbuf);

    UDP_SESSION *us = udp_session_find(gobj, peername);
    if(!us) {
        us = udp_session_new(gobj, peername);
        if(!us) {
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_INTERNAL_ERROR,
                "msg",              "%s", "udp_session_new() FAILED",
                NULL
            );
            gbuf_decref(gbuf);
            JSON_DECREF(kw);
            return -1;
        }
    }
    reset_itl(&us->itl);
    us->inactivity_timeout = start_sectimer(priv->inactt);
    while(1) {
        int ret = consume_tlv(&us->itl, gbuf);
        if(ret < 0) {
            /*
             *  some error, discard frame and session
             */
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SNMP_ERROR,
                "msg",          "%s", "ASN1-ITL protocol error",
                NULL
            );
            gbuf_decref(gbuf);
            udp_session_delete(gobj, us);
            break;
        }
        if(us->itl.complete) {
            /*
             *  Send tlv to parent
             */
            TLV *tlv = tlv_create(us->itl.type, us->itl.length);
            if(!tlv) {
                log_error(0,
                    "gobj",             "%s", gobj_full_name(gobj),
                    "function",         "%s", __FUNCTION__,
                    "msgset",           "%s", MSGSET_MEMORY_ERROR,
                    "msg",              "%s", "tlv_create() FAILED",
                    NULL
                );
                gbuf_decref(gbuf);
                udp_session_delete(gobj, us);
                break;
            }
            tlv->gbuf = us->itl.gbuf;   // give gbuff to tlv
            us->itl.gbuf = 0;

            ret = tlv_expand(tlv);
            if (ret < 0) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_SNMP_ERROR,
                    "msg",          "%s", "tlv_expand() FAILED",
                    NULL
                );
                gbuf_decref(gbuf);
                udp_session_delete(gobj, us);
                break;
            }

            json_t *kw_ex = json_pack("{s:s, s:I}",
                "session_name", us->session_name,
                "tlv", (json_int_t)tlv
            );
            gobj_send_event(gobj_parent(gobj), "EV_RX_TLV", kw_ex, gobj);

            /*
             *  Reset for new frame
             */
            reset_itl(&us->itl);
        }
        int available = gbuf_leftbytes(gbuf);
        if(!available) {
            // no more data
            gbuf_decref(gbuf);
            break;
        }
    }
    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  priv->timer or GCLASS_UDP_S0 has stopped
     */
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *  Check the sessions: delete all sessions with inactivity time elapsed.
     */
    UDP_SESSION *us = dl_first(&priv->dl_udp_session);
    while(us) {
        if(test_sectimer(us->inactivity_timeout)) {
            UDP_SESSION *next = dl_next(us);
            udp_session_delete(gobj, us);
            us = next;
        }
        us = dl_next(us);
    }

    JSON_DECREF(kw);
    return 1;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_RX_DATA",      0},
    {"EV_STOPPED",      0},
    {"EV_TIMEOUT",      0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_NEW_SESSION", 0},
    {"EV_DEL_SESSION", 0},
    {"EV_RX_TLV", 0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_RX_DATA",      ac_process_udp,         0},
    {"EV_TIMEOUT",      ac_timeout,             0},
    {"EV_STOPPED",      ac_stopped,             0},
    {"EV_TX_READY",     0,                      0},
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
    GCLASS_TLV_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_destroy,
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
        0, //mt_stats,
        0, //mt_command,
        0, //mt_create2,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link,
        0, //mt_delete_child_resource_link,
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
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_tlv(void)
{
    return &_gclass;
}


/***************************************************************************
 *
 ***************************************************************************/
PUBLIC TLV * tlv_create(uint8_t type, int length)
{
    TLV *tlv = gbmem_malloc(sizeof(TLV));
    if(!tlv) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for sizeof(TLV)",
            "sizeof(TLV)",  "%d", sizeof(TLV),
            NULL
        );
        return 0;
    }
    tlv->type = type;
    tlv->length = length;

    return tlv;
}

/***************************************************************************
 *  Remove a TLV and all his childs
 ***************************************************************************/
PUBLIC void tlv_delete(TLV *tlv)
{
    TLV *child;

    while((child = dl_first(&tlv->dl_sequence))) {
        dl_delete(&tlv->dl_sequence, child, 0);
        tlv_delete(child);
    }
    GBUF_DECREF(tlv->gbuf);
    gbmem_free((void *)tlv);
}

/***************************************************************************
 *  Duplicate a tlv
 ***************************************************************************/
PUBLIC TLV * tlv_dup(TLV *tlv)
{
    TLV *dup = tlv_create(tlv->type, tlv->length);
    return dup;
}

/***************************************************************************
 *  Get snmp version of frame
 ***************************************************************************/
PUBLIC int snmp_get_version(TLV *tlv, int *version)
{
    /*
     *  Check snmp frame
     */
    if(tlv->type != (ASN_SEQUENCE | ASN_CONSTRUCTOR)) { // 0x30
        //  Snmp must begin with 0x30
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "SNMP ASN1 type not a SEQUENCE",
            NULL
        );
        return -1;
    }
    /*
     *  Get version
     */
    int inds_version[] = {1, 0};
    if(tlv_read_integer(
            tlv,
            inds_version,
            INDICES_LENGTH(inds_version),
            version)<0) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_read_integer(version) FAILED",
            NULL
        );
        return -1;
    }
    return 0;
}

/***************************************************************************
 *  Get snmp community
 ***************************************************************************/
PUBLIC char *snmp_get_community(TLV *tlv)
{
    int inds_community[] = {2, 0};
    char *community = tlv_read_string(
        tlv,
        inds_community,
        INDICES_LENGTH(inds_community)
    );
    if(!community) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_read_string(community) FAILED",
            NULL
        );
        return 0;
    }
    return community;
}

/***************************************************************************
 *  Get pdu type
 ***************************************************************************/
PUBLIC int snmp_get_pdu_type(TLV *tlv, uint8_t *pdu_type)
{
    int inds_pdu_type[] = {3, 0};
    if(tlv_read_type(
            tlv,
            inds_pdu_type,
            INDICES_LENGTH(inds_pdu_type),
            pdu_type)<0) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_read_type(pdu_type) FAILED",
            NULL
        );
        return -1;
    }
    return 0;
}

/***************************************************************************
 *  Return varbind list tlv.
 ***************************************************************************/
PUBLIC TLV *snmp_get_varbind_list(TLV *tlv)
{
    int inds[] = {3, 4, 0};
    TLV *child = tlv_find(tlv, inds, INDICES_LENGTH(inds));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind_list) FAILED",
            NULL
        );
        return 0;
    }
    return child;
}

/***************************************************************************
 *  Return number of varbind items.
 ***************************************************************************/
PUBLIC int snmp_get_varbind_list_size(TLV *tlv)
{
    int inds[] = {3, 4, 0};
    TLV *child = tlv_find(tlv, inds, INDICES_LENGTH(inds));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind_list) FAILED",
            NULL
        );
        return 0;
    }
    return dl_size(&child->dl_sequence);
}

/***************************************************************************
 *  Return a varbind tlv
 ***************************************************************************/
PUBLIC TLV *snmp_get_varbind(TLV *tlv, int ind)
{
    int inds[] = {3, 4, ind, 0};
    TLV *child = tlv_find(tlv, inds, INDICES_LENGTH(inds));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind) FAILED",
            NULL
        );
        return 0;
    }

    return child;
}

/***************************************************************************
 *  Save in `poid` the oid of tlv. TLV must be a varbind.
 *  `oid_len` is IN/OUT.
 *      In Input is size of `poid` (number of array's indices).
 *      In Output is the readed oid length saved in `poid`.
 *  Return 0 if succesfully or -1 if error.
 ***************************************************************************/
PUBLIC int snmp_get_varbind_oid(TLV *tlv_varbind, oid_t *poid, int *oid_len)
{
    int inds_oid[] = {1, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_oid, INDICES_LENGTH(inds_oid));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind oid) FAILED",
            NULL
        );
        return -1;
    }
    memset(poid, 0, sizeof(oid_t) * (*oid_len));
    return tlv_parse_asn1_oid(child, poid, oid_len);
}

/***************************************************************************
 *  Get the tlv value of a varbind
 ***************************************************************************/
PUBLIC TLV *snmp_get_varbind_value(TLV *tlv_varbind)
{
    int inds_value[] = {2, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_value, INDICES_LENGTH(inds_value));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind value) FAILED",
            NULL
        );
        return 0;
    }
    return child;
}

/***************************************************************************
 *  Get non-repeaters
 ***************************************************************************/
PUBLIC int snmp_get_non_repeaters(TLV *tlv, int *value)
{
    int inds_error_status[] = {3, 2, 0};
    TLV *child = tlv_find(tlv, inds_error_status, INDICES_LENGTH(inds_error_status));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(non-repeaters) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_parse_asn1_signed32(child, value);
}

/***************************************************************************
 *  Get max-repetitions
 ***************************************************************************/
PUBLIC int snmp_get_max_repetitions(TLV *tlv, int *value)
{
    int inds_error_index[] = {3, 3, 0};
    TLV *child = tlv_find(tlv, inds_error_index, INDICES_LENGTH(inds_error_index));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(max-repetitions) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_parse_asn1_signed32(child, value);
}

/***************************************************************************
 *  Set pdu type
 ***************************************************************************/
PUBLIC int snmp_set_pdu_type(TLV *tlv, uint8_t pdu_type)
{
    int inds_pdu_type[] = {3, 0};
    TLV *child = tlv_find(tlv, inds_pdu_type, INDICES_LENGTH(inds_pdu_type));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(pdu_type) FAILED",
            NULL
        );
        return -1;
    }
    child->type = pdu_type;
    return 0;
}

/***************************************************************************
 *  Set error status
 ***************************************************************************/
PUBLIC int snmp_set_error_status(TLV *tlv, int status)
{
    int inds_error_status[] = {3, 2, 0};
    TLV *child = tlv_find(tlv, inds_error_status, INDICES_LENGTH(inds_error_status));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(error status) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_build_asn1_signed32(child, ASN_INTEGER, status);
}

/***************************************************************************
 *  Set error index
 ***************************************************************************/
PUBLIC int snmp_set_error_index(TLV *tlv, int index)
{
    int inds_error_index[] = {3, 3, 0};
    TLV *child = tlv_find(tlv, inds_error_index, INDICES_LENGTH(inds_error_index));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(error index) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_build_asn1_signed32(child, ASN_INTEGER, index);
}

/***************************************************************************
 *  Build snmp response
 ***************************************************************************/
PUBLIC GBUFFER * build_snmp_response(
    TLV *tlv,
    int error_status,
    int error_index)
{
    snmp_set_pdu_type(tlv, SNMP_MSG_RESPONSE);
    snmp_set_error_status(tlv, error_status);
    snmp_set_error_index(tlv, error_index);

    GBUFFER *gbuf_response = gbuf_create(SNMP_MAX_MSG_SIZE, SNMP_MAX_MSG_SIZE, 0, 0);
    tlv_shrink(gbuf_response, tlv);
    return gbuf_response;
}

/***************************************************************************
 *  Set the object id of a varbind
 ***************************************************************************/
PUBLIC int snmp_set_varbind_oid(TLV *tlv_varbind, const oid_t *oid, int oid_len)
{
    int inds_oid[] = {1, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_oid, INDICES_LENGTH(inds_oid));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind oid) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_build_asn1_oid(child, ASN_OBJECT_ID, oid, oid_len);
}

/***************************************************************************
 *  Set the values of a varbind
 ***************************************************************************/
PUBLIC int snmp_set_varbind_value_oid(TLV *tlv_varbind, const oid_t *oid, int oid_len)
{
    int inds_value[] = {2, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_value, INDICES_LENGTH(inds_value));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind value) FAILED",
            NULL
        );
        return -1;
    }
    return tlv_build_asn1_oid(child, ASN_OBJECT_ID, oid, oid_len);
}

/***************************************************************************
 *  Write an empty value
 ***************************************************************************/
PUBLIC int snmp_set_varbind_value_empty(TLV *tlv_varbind, int type)
{
    int inds_value[] = {2, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_value, INDICES_LENGTH(inds_value));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind value) FAILED",
            NULL
        );
        return -1;
    }

    return tlv_build_asn1_empty(child, type);
}

/***************************************************************************
 *  Write a varbind value from sdata
 ***************************************************************************/
PUBLIC int snmp_set_varbind_value_from_sdata(
    TLV *tlv_varbind,
    sdata_desc_t *it,
    hsdata hs,
    void *ptr)
{
    int inds_value[] = {2, 0}; // relative to varbind!
    TLV *child = tlv_find(tlv_varbind, inds_value, INDICES_LENGTH(inds_value));
    if(!child) {
        log_error(0,
            "gobj",         "%s", "TLV",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SNMP_ERROR,
            "msg",          "%s", "tlv_find(varbind value) FAILED",
            NULL
        );
        return -1;
    }

    SData_Value_t value = sdata_read_by_type(hs, it, ptr);

    if(ASN_IS_BOOLEAN(it->type)) {
        tlv_build_asn1_signed32(
            child,
            ASN_INTEGER,
            value.b
        );
    } else if(ASN_IS_STRING(it->type)) {
        tlv_build_asn1_string(
            child,
            it->type,
            value.s,
            value.s?strlen(value.s):0
        );
    } else if(ASN_IS_JSON(it->type)) {
        char *s = json_dumps(value.j, JSON_COMPACT);
        if(s) {
            tlv_build_asn1_string(
                child,
                ASN_OCTET_STR,
                s,
                s?strlen(s):0
            );
            gbmem_free(s);
        }
    } else if(ASN_IS_UNSIGNED32(it->type)) {
        tlv_build_asn1_unsigned32(
            child,
            it->type,
            value.i32
        );
    } else if(ASN_IS_SIGNED32(it->type)) {
        tlv_build_asn1_signed32(
            child,
            it->type,
            value.u32
        );
    } else if(ASN_IS_UNSIGNED64(it->type)) {
        tlv_build_asn1_unsigned64(
            child,
            it->type,
            value.u64
        );
    } else {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "sdata_item: ASN it->type, not case FOUND",
            "it->type",         "%d", it->type,
            NULL
        );
        return 0;
    }

    return 0;
}

/***************************************************************************
 *  Write sdata from varbind
 ***************************************************************************/
PUBLIC int snmp_write_sdata_from_varbind(
    sdata_desc_t *it,
    hsdata hs,
    void *ptr,
    TLV *tlv_value)
{
    int type = tlv_value->type;

    if(type == ASN_BOOLEAN) {
        int value;
        if(tlv_parse_asn1_signed32(tlv_value, &value)<0) {
            return -1;
        }
        SData_Value_t v;
        v.b = value?1:0;
        return sdata_write_by_type(hs, it, ptr, v);

    } else if(ASN_IS_STRING(type)) {
        char *str = tlv_parse_asn1_string(tlv_value);
        if(!str) {
            return -1;
        }
        SData_Value_t v;
        v.s = str;
        return sdata_write_by_type(hs, it, ptr, v);

    } else if(ASN_IS_UNSIGNED32(type)) {
        unsigned int value;
        if(tlv_parse_asn1_unsigned32(tlv_value, &value)<0) {
            return -1;
        }
        SData_Value_t v;
        v.u32 = value;
        return sdata_write_by_type(hs, it, ptr, v);

    } else if(ASN_IS_SIGNED32(type)) {
        int value;
        if(tlv_parse_asn1_signed32(tlv_value, &value)<0) {
            return -1;
        }
        SData_Value_t v;
        v.i32 = value;
        return sdata_write_by_type(hs, it, ptr, v);

    //} else if(ASN_IS_NUMBER64(type)) { no case
    } else {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "sdata_item: ASN type, not case FOUND",
            "name",         "%s", it->name,
            "tlv type",     "%d", type,
            "it type",      "%d", it->type,
            NULL
        );
        return -1;
    }

    return 0;
}

