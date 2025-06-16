/*
 * Copyright (c) 2025, Red Hat, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef EN_LS_IPAM_H
#define EN_LS_IPAM_H 1

#include <config.h>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

/* OVS includes */
#include "include/openvswitch/hmap.h"
#include "lib/bitmap.h"
#include "lib/socket-util.h"
#include "lib/uuidset.h"
#include "openvswitch/util.h"
#include "openvswitch/vlog.h"
#include "stopwatch.h"

/* OVN includes */
#include "en-lb-data.h"
#include "en-ls-stateful.h"
#include "en-port-group.h"
#include "lib/inc-proc-eng.h"
#include "lib/lb.h"
#include "lib/ovn-nb-idl.h"
#include "lib/ovn-sb-idl.h"
#include "lib/ovn-util.h"
#include "lib/stopwatch-names.h"
#include "lflow-mgr.h"
#include "northd.h"

struct ls_ipam_tracked_data {
    /* Updated logical switch with data. */
    struct hmapx crupdated; /* Stores 'struct ls_stateful_record'. */
};

void *en_ls_ipam_init(struct engine_node *, struct engine_arg *);
void en_ls_ipam_cleanup(void *data);
void en_ls_ipam_clear_tracked_data(void *data);
enum engine_node_state en_ls_ipam_run(struct engine_node *, void *data);

enum engine_input_handler_result
ls_ipam_northd_handler(struct engine_node *, void *data);

#endif /* EN_LS_IPAM_H */