/*
 * Copyright (c) 2024, Red Hat, Inc.
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

VLOG_DEFINE_THIS_MODULE(en_ls_ipam);

struct ls_ipam_input {
    struct ovn_datapaths *ls_datapaths;
};

static struct ovn_datapaths *
ls_ipam_get_input_data(struct engine_node *node)
{
    const struct northd_data *northd_data =
        engine_get_input_data("northd", node);

    return &northd_data->ls_datapaths;
}
void *
en_ls_ipam_init(struct engine_node *node OVS_UNUSED,
                struct engine_arg *arg OVS_UNUSED) {
    struct ls_ipam_input *data = xmalloc(sizeof *data);
    return data;
}

void
en_ls_ipam_cleanup(void *data_)
{
    struct ls_ipam_input *data = data_;
    free(data);
}
void
en_ls_ipam_clear_tracked_data(void *data)
{

}

enum engine_node_state
en_ls_ipam_run(struct engine_node *node OVS_UNUSED, void *data OVS_UNUSED) {
     /*struct ovn_datapaths *input_data = ls_ipam_get_input_data(node);
      full recompute
     */
    return EN_UPDATED;
}

enum engine_input_handler_result
ls_ipam_northd_handler(struct engine_node *node, void *data) {
    VLOG_INFO("LUCAS ls_stateful_northd_handler");
    struct northd_data *northd_data = engine_get_input_data("northd", node);
    if (!northd_has_tracked_data(&northd_data->trk_data)) {
        VLOG_INFO("LUCAS ls_stateful_northd_handler EN_UNHANDLED");
        return EN_UNHANDLED;
    }

    if (!northd_has_ls_ipam_in_tracked_data(&northd_data->trk_data)) {
        VLOG_INFO("LUCAS ls_stateful_northd_handler EN_HANDLED_UNCHANGED");
        return EN_HANDLED_UNCHANGED;
    }

    struct northd_tracked_data *nd_changes = &northd_data->trk_data;
    struct ovn_datapaths * input_data = ls_ipam_get_input_data(node);
    struct hmapx_node *hmapx_node;

    HMAPX_FOR_EACH (hmapx_node, &nd_changes->ls_with_changed_ipam) {
        const struct ovn_datapath *od = hmapx_node->data;
        //build_ipam()

    }

    VLOG_INFO("LUCAS ls_stateful_northd_handler EN_HANDLED_UNCHANGED");
    return EN_HANDLED_UNCHANGED;
}
