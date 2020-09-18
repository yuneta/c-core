/****************************************************************************
 *              YUNETA.H
 *              Includes
 *              Copyright (c) 2014-2015 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/

#ifndef _YUNETA_H
#define _YUNETA_H 1

#ifdef __cplusplus
extern "C"{
#endif

/*
 *  External dependencies
 */
#include <ginsfsm.h>


/*
 *  core
 */
#include "yuneta_version.h"
#include "msglog_yuneta.h"
#include "c_yuno.h"         // the grandmother
#include "dbsimple.h"

/*
 *  Services
 */
#include "c_node.h"
#include "c_resource.h"
#include "c_ievent_srv.h"
#include "c_ievent_cli.h"
// #include "c_store.h"
// #include "c_snmp.h"
// #include "c_dynagent.h"

/*
 *  Gadgets
 */
#include "c_qiogate.h"
#include "c_iogate.h"
#include "c_channel.h"
#include "c_counter.h"
// #include "c_task.h"
#include "c_dynrule.h"
#include "c_timetransition.h"
#include "c_rstats.h"

/*
 *  Protocols
 */
#include "c_connex.h"
#include "c_websocket.h"
#include "c_prot_header4.h"
#include "c_prot_raw.h"
#include "c_prot_http.h"
// #include "c_serverlink.h"
// #include "c_tlv.h"

/*
 *  Mixin uv-gobj
 */
#include "c_gss_udp_s0.h"   // gossamer
#include "c_tcp0.h"
#include "c_tcp_s0.h"
#include "c_udp_s0.h"
#include "c_udp0.h"
#include "c_timer.h"
#include "c_fs.h"

/*
 *  Decoders
 */
#include "ghttp_parser.h"

/*
 * Entry point
 */
#include "entry_point.h"


#ifdef __cplusplus
}
#endif


#endif

