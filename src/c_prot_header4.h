/****************************************************************************
 *          C_PROT_HEADER4.H
 *          Prot_header4 GClass.
 *
 *          Protocol with a 4 bytes header.
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
.. _prot_header4-gclass:

**"Prot_header4"** :ref:`GClass`
================================

Protocol with a 4 bytes header.

``GCLASS_PROT_HEADER4_NAME``
   Macro of the gclass string name, i.e **"Prot_header4"**.

``GCLASS_PROT_HEADER4``
   Macro of the :func:`gclass_prot_header4()` function.

**rst**/
PUBLIC GCLASS *gclass_prot_header4(void);

#define GCLASS_PROT_HEADER4_NAME "Prot_header4"
#define GCLASS_PROT_HEADER4 gclass_prot_header4()


#ifdef __cplusplus
}
#endif
