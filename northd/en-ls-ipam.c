/*
 * Copyright (c) 2025, STACKIT GmbH & Co. KG
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
#include <stdbool.h>

#include "openvswitch/vlog.h"
#include "stopwatch.h"
#include "northd.h"

#include "en-ls-ipam.h"
#include "en-learned-route-sync.h"
#include "lib/stopwatch-names.h"
#include "openvswitch/hmap.h"

VLOG_DEFINE_THIS_MODULE(en_ls_ipam);

void *en_ls_ipam_init(struct engine_node *node OVS_UNUSED,
                               struct engine_arg *arg OVS_UNUSED)
{
    VLOG_INFO("LUCAS %s", __func__);
    return NULL;
}

void en_ls_ipam_cleanup(void *_data OVS_UNUSED)
{
    VLOG_INFO("LUCAS %s", __func__);
   // cleanup_macam();
}

enum engine_node_state
en_ls_ipam_run(struct engine_node *node OVS_UNUSED, void *_data OVS_UNUSED)
{
    VLOG_INFO("LUCAS %s", __func__);
    return EN_UPDATED;
}

enum engine_input_handler_result
ls_ipam_handler(struct engine_node *node, void * _data OVS_UNUSED)
{
    VLOG_INFO("LUCAS %s", __func__);
    struct northd_data *northd_data = engine_get_input_data("northd", node);
    if (!northd_has_tracked_data(&northd_data->trk_data)) {
        return EN_UNHANDLED;
    }

    if (!northd_has_ls_ipam_in_tracked_data(&northd_data->trk_data)/*&&
        !northd_has_lsps_in_tracked_data(&northd_data->trk_data) */ ) {
        return EN_HANDLED_UNCHANGED;
    }

    /*if (!northd_has_lsps_in_tracked_data(&northd_data->trk_data)) {
        return EN_UNHANDLED;
    }*/
    struct northd_tracked_data *nd_changes = &northd_data->trk_data;
    struct hmapx_node *hmapx_node;
   // hmap_insert(ls_ports, &op->key_node, hash_string(op->key, 0));
    struct  ovn_port *op;
    
    HMAPX_FOR_EACH (hmapx_node, &nd_changes->trk_lsps.deleted) {
        op = hmapx_node->data;
        for (size_t i = 0; i < op->n_lsp_addrs; i++) {
        //uint64_t mac64 = eth_addr_to_uint64(port->lsp_addrs[i].ea);
        //VLOG_INFO("LUCAS rem %s %lu", port->lsp_addrs->ea_s, mac64);
        //remove_mac_from_macam(&port->lsp_addrs[i].ea);
            remove_mac_from_macam(&op->lsp_addrs[i].ea);
        }
        for (size_t i = 0; i < op->n_ps_addrs; i++) {
            remove_mac_from_macam(&op->ps_addrs[i].ea);
        }
       // hmap_insert(&ls_ports, &op->key_node, hash_string(op->key, 0));
    }
/*    HMAPX_FOR_EACH (hmapx_node, &nd_changes->trk_lsps.updated) {
        op =  hmapx_node->data;
        hmap_insert(&ls_ports, &op->key_node, hash_string(op->key, 0));
    }*/
    HMAPX_FOR_EACH (hmapx_node, &nd_changes->ls_with_changed_ipam) {
        struct ovn_datapath *od = hmapx_node->data;
        init_ipam_info_for_datapath(od);

        VLOG_INFO("LUCAS %s", od->nbs->name);
        /*if (od->ipam_info.allocated_ipv4s &&
            od->ipam_info.ipv6_prefix_set) {
            return EN_HANDLED_UPDATED;
        } */
        if (!update_ipam_from_ls(od, &northd_data->ls_ports, false)) {
            return EN_HANDLED_UNCHANGED;
        }
    }
    return EN_HANDLED_UPDATED;
}