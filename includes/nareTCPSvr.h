/* Copyright 2024 Felipe Markson dos Santos Monteiro <fmarkson@outlook.com> */
/*
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef NARE_SVR_H
#define NARE_SVR_H

#include "nare.h"

typedef struct NareTCPSvr NareTCPSvr;

typedef struct NareTCPSvrClient NareTCPSvrClient;
NareTCPSvr* NareTCPSvr_open(Nare* nare, int port);
void NareTCPSvr_close(NareTCPSvr* svr);
#endif
