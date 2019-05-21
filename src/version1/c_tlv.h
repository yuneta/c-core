/****************************************************************************
 *          C_TLV.H
 *          BER (Basic Encoding Rules) TLV (Type, Length, Value)
 *          ASN.1 (Abstract Syntax Notation.One) used by snmp/ldap
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_TLV_H
#define _C_TLV_H 1

#include <stdint.h>
#include <ginsfsm.h>
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Structures
 *********************************************************************/
typedef struct _TLV {
    DL_ITEM_FIELDS

    uint8_t type;
    int length;
    /*
     *  when type is a sequence, the content of data is expand to dl_sequence.
     *  Cases:
     *      - primary type:
     *          * data:         with CONTENT
     *          * dl_sequence: EMPTY
     *      - complex type:
     *          * data:         NULL
     *          * dl_sequence: with CONTENT
     */
    dl_list_t dl_sequence;

    GBUFFER *gbuf;   /* Data without expansion.
                      * When expanded content goes to data or dl_sequence.
                      */
} TLV;

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_tlv(void);

/*********************************************************************
 *      Prototypes
 *********************************************************************/
#define INDICES_LENGTH(x)  (sizeof(x)/sizeof(int))

PUBLIC TLV * tlv_create(uint8_t type, int length);
PUBLIC void tlv_delete(TLV *tlv);
PUBLIC TLV * tlv_dup(TLV *tlv);

/*
 *  Read functions
 */
PUBLIC int snmp_get_version(TLV *tlv, int *version);
PUBLIC char *snmp_get_community(TLV *tlv);
PUBLIC int snmp_get_pdu_type(TLV *tlv, uint8_t *pdu_type);

PUBLIC TLV *snmp_get_varbind_list(TLV *tlv);
PUBLIC int snmp_get_varbind_list_size(TLV *tlv);
PUBLIC TLV *snmp_get_varbind(TLV *tlv, int ind);
PUBLIC int snmp_get_varbind_oid(TLV *tlv_varbind, oid_t *poid, int *oid_len);
PUBLIC TLV *snmp_get_varbind_value(TLV *tlv_varbind);

PUBLIC int snmp_get_non_repeaters(TLV *tlv, int *value);
PUBLIC int snmp_get_max_repetitions(TLV *tlv, int *value);

/*
 *  Write functions
 */
PUBLIC int snmp_set_pdu_type(TLV *tlv, uint8_t pdu_type);
PUBLIC int snmp_set_error_status(TLV *tlv, int status);
PUBLIC int snmp_set_error_index(TLV *tlv, int index);
PUBLIC GBUFFER *build_snmp_response(
    TLV *tlv,
    int error_status,
    int error_index
);

PUBLIC TLV *snmp_new_varbind(void);

// Set the object id of a varbind
PUBLIC int snmp_set_varbind_oid(TLV *tlv_varbind, const oid_t *oid, int oid_len);

// Set the values of a varbind:
PUBLIC int snmp_set_varbind_value_oid(TLV *tlv_varbind, const oid_t *oid, int oid_len);
PUBLIC int snmp_set_varbind_value_empty(TLV *tlv_varbind, int type);
PUBLIC int snmp_set_varbind_value_from_sdata(
    TLV *tlv_varbind,
    sdata_desc_t *it,
    hsdata hs,
    void *ptr
);
PUBLIC int snmp_write_sdata_from_varbind(
    sdata_desc_t *it,
    hsdata hs,
    void *ptr,
    TLV *tlv_value
);


#define GCLASS_TLV_NAME "TLV"
#define GCLASS_TLV gclass_tlv()

#ifdef __cplusplus
}
#endif


#endif

