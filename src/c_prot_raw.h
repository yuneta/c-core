/****************************************************************************
 *          C_PROT_RAW.H
 *          Prot_raw GClass.
 *
 *          Raw protocol, no insert/deletion of headers
 *
 *          Copyright (c) 2017 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>

#ifdef __cplusplus
extern "C"{
#endif

/**rst**
.. _prot_raw-gclass:

**"Prot_raw"** :ref:`GClass`
================================

Raw protocol, no insert/deletion of headers

``GCLASS_PROT_RAW_NAME``
   Macro of the gclass string name, i.e **"Prot_raw"**.

``GCLASS_PROT_RAW``
   Macro of the :func:`gclass_prot_raw()` function.

**rst**/
PUBLIC GCLASS *gclass_prot_raw(void);

#define GCLASS_PROT_RAW_NAME "Prot_raw"
#define GCLASS_PROT_RAW gclass_prot_raw()


#ifdef __cplusplus
}
#endif
