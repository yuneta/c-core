/****************************************************************************
 *          C_SNMP.H
 *          Implementation of the SNMP protocol.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_tlv.h"
#include "c_udp_s0.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_snmp(void);

#define GCLASS_SNMP_NAME "Snmp"
#define GCLASS_SNMP gclass_snmp()

#ifdef __cplusplus
}
#endif
