/*
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

#include "en-global-config.h"
#include "en-lflow.h"
#include "en-lr-nat.h"
#include "en-lr-stateful.h"
#include "en-ls-stateful.h"
#include "en-multicast.h"
#include "en-northd.h"
#include "en-meters.h"
#include "en-sampling-app.h"
#include "en-group-ecmp-route.h"
#include "lflow-mgr.h"
#include "en-datapath-logical-switch.h"
#include "en-datapath-logical-router.h"

#include "lib/inc-proc-eng.h"
#include "northd.h"
#include "stopwatch.h"
#include "lib/stopwatch-names.h"
#include "timeval.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(en_lflow);

static void
lflow_get_input_data(struct engine_node *node,
                     struct lflow_input *lflow_input)
{
    struct northd_data *northd_data = engine_get_input_data("northd", node);
    struct bfd_sync_data *bfd_sync_data =
        engine_get_input_data("bfd_sync", node);
    struct routes_data *routes_data =
        engine_get_input_data("routes", node);
    struct group_ecmp_route_data *group_ecmp_route_data =
        engine_get_input_data("group_ecmp_route", node);
    struct route_policies_data *route_policies_data =
        engine_get_input_data("route_policies", node);
    struct port_group_data *pg_data =
        engine_get_input_data("port_group", node);
    struct sync_meters_data *sync_meters_data =
        engine_get_input_data("sync_meters", node);
    struct ed_type_lr_stateful *lr_stateful_data =
        engine_get_input_data("lr_stateful", node);
    struct ed_type_ls_stateful *ls_stateful_data =
        engine_get_input_data("ls_stateful", node);
    struct multicast_igmp_data *multicat_igmp_data =
        engine_get_input_data("multicast_igmp", node);

    lflow_input->sbrec_logical_flow_table =
        EN_OVSDB_GET(engine_get_input("SB_logical_flow", node));
    lflow_input->sbrec_logical_dp_group_table =
        EN_OVSDB_GET(engine_get_input("SB_logical_dp_group", node));
    lflow_input->sbrec_acl_id_table =
        EN_OVSDB_GET(engine_get_input("SB_acl_id", node));

    lflow_input->sbrec_mcast_group_by_name_dp =
           engine_ovsdb_node_get_index(
                          engine_get_input("SB_multicast_group", node),
                         "sbrec_mcast_group_by_name");

    lflow_input->ls_datapaths = &northd_data->ls_datapaths;
    lflow_input->lr_datapaths = &northd_data->lr_datapaths;
    lflow_input->ls_ports = &northd_data->ls_ports;
    lflow_input->lr_ports = &northd_data->lr_ports;
    lflow_input->ls_port_groups = &pg_data->ls_port_groups;
    lflow_input->lr_stateful_table = &lr_stateful_data->table;
    lflow_input->ls_stateful_table = &ls_stateful_data->table;
    lflow_input->meter_groups = &sync_meters_data->meter_groups;
    lflow_input->lb_datapaths_map = &northd_data->lb_datapaths_map;
    lflow_input->svc_monitor_map = &northd_data->svc_monitor_map;
    lflow_input->bfd_ports = &bfd_sync_data->bfd_ports;
    lflow_input->route_data = group_ecmp_route_data;
    lflow_input->route_tables = &routes_data->route_tables;
    lflow_input->route_policies = &route_policies_data->route_policies;
    lflow_input->igmp_groups = &multicat_igmp_data->igmp_groups;
    lflow_input->igmp_lflow_ref = multicat_igmp_data->lflow_ref;

    struct ed_type_global_config *global_config =
        engine_get_input_data("global_config", node);
    lflow_input->features = &global_config->features;
    lflow_input->ovn_internal_version_changed =
        global_config->ovn_internal_version_changed;
    lflow_input->svc_monitor_mac = global_config->svc_monitor_mac;

    struct ed_type_sampling_app_data *sampling_app_data =
        engine_get_input_data("sampling_app", node);
    lflow_input->sampling_apps = &sampling_app_data->apps;
}

enum engine_node_state
en_lflow_run(struct engine_node *node, void *data)
{
    struct lflow_input lflow_input;
    lflow_get_input_data(node, &lflow_input);

    stopwatch_start(BUILD_LFLOWS_STOPWATCH_NAME, time_msec());

    struct lflow_data *lflow_data = data;
    lflow_table_clear(lflow_data->lflow_table);
    lflow_data->handled_incrementally = false;
    lflow_reset_northd_refs(&lflow_input);
    lflow_ref_clear(lflow_input.igmp_lflow_ref);

    build_lflows(&lflow_input, lflow_data->lflow_table);
    stopwatch_stop(BUILD_LFLOWS_STOPWATCH_NAME, time_msec());

    return EN_UPDATED;
}

enum engine_input_handler_result
lflow_northd_handler(struct engine_node *node,
                     void *data)
{
    struct northd_data *northd_data = engine_get_input_data("northd", node);
    if (!northd_has_tracked_data(&northd_data->trk_data)) {
        return EN_UNHANDLED;
    }

    const struct engine_context *eng_ctx = engine_get_context();
    struct lflow_data *lflow_data = data;

    struct lflow_input lflow_input;
    lflow_get_input_data(node, &lflow_input);

    if (!lflow_handle_northd_port_changes(eng_ctx->ovnsb_idl_txn,
                                          &northd_data->trk_data.trk_lsps,
                                          &lflow_input,
                                          lflow_data->lflow_table)) {
        return EN_UNHANDLED;
    }

    if (!lflow_handle_northd_lb_changes(
            eng_ctx->ovnsb_idl_txn, &northd_data->trk_data.trk_lbs,
            &lflow_input, lflow_data->lflow_table)) {
        return EN_UNHANDLED;
    }

    return EN_HANDLED_UPDATED;
}

enum engine_input_handler_result
lflow_port_group_handler(struct engine_node *node, void *data OVS_UNUSED)
{
    struct port_group_data *pg_data =
        engine_get_input_data("port_group", node);

    /* If the set of switches per port group didn't change then there's no
     * need to reprocess lflows.  Otherwise, there might be a need to
     * add/delete port-group ACLs to/from switches. */
    if (pg_data->ls_port_groups_sets_changed) {
        return EN_UNHANDLED;
    }

    return EN_HANDLED_UPDATED;
}

enum engine_input_handler_result
lflow_lr_stateful_handler(struct engine_node *node, void *data)
{
    struct ed_type_lr_stateful *lr_sful_data =
        engine_get_input_data("lr_stateful", node);

    if (!lr_stateful_has_tracked_data(&lr_sful_data->trk_data)) {
        return EN_UNHANDLED;
    }

    const struct engine_context *eng_ctx = engine_get_context();
    struct lflow_data *lflow_data = data;
    struct lflow_input lflow_input;

    lflow_get_input_data(node, &lflow_input);
    if (!lflow_handle_lr_stateful_changes(eng_ctx->ovnsb_idl_txn,
                                          &lr_sful_data->trk_data,
                                          &lflow_input,
                                          lflow_data->lflow_table)) {
        return EN_UNHANDLED;
    }

    return EN_HANDLED_UPDATED;
}

enum engine_input_handler_result
lflow_ls_stateful_handler(struct engine_node *node, void *data)
{
    struct ed_type_ls_stateful *ls_sful_data =
        engine_get_input_data("ls_stateful", node);

    if (!ls_stateful_has_tracked_data(&ls_sful_data->trk_data)) {
        return EN_UNHANDLED;
    }

    const struct engine_context *eng_ctx = engine_get_context();
    struct lflow_data *lflow_data = data;
    struct lflow_input lflow_input;

    lflow_get_input_data(node, &lflow_input);
    if (!lflow_handle_ls_stateful_changes(eng_ctx->ovnsb_idl_txn,
                                          &ls_sful_data->trk_data,
                                          &lflow_input,
                                          lflow_data->lflow_table)) {
        return EN_UNHANDLED;
    }

    return EN_HANDLED_UPDATED;
}

enum engine_input_handler_result
lflow_multicast_igmp_handler(struct engine_node *node, void *data)
{
    struct multicast_igmp_data *mcast_igmp_data =
        engine_get_input_data("multicast_igmp", node);

    const struct engine_context *eng_ctx = engine_get_context();
    struct lflow_data *lflow_data = data;
    struct lflow_input lflow_input;
    lflow_get_input_data(node, &lflow_input);

    if (!lflow_ref_resync_flows(mcast_igmp_data->lflow_ref,
                                lflow_data->lflow_table,
                                eng_ctx->ovnsb_idl_txn,
                                lflow_input.ls_datapaths,
                                lflow_input.lr_datapaths,
                                lflow_input.ovn_internal_version_changed,
                                lflow_input.sbrec_logical_flow_table,
                                lflow_input.sbrec_logical_dp_group_table)) {
        return EN_UNHANDLED;
    }

    build_igmp_lflows(&mcast_igmp_data->igmp_groups,
                      &lflow_input.ls_datapaths->datapaths,
                      lflow_data->lflow_table,
                      mcast_igmp_data->lflow_ref);

    if (!lflow_ref_sync_lflows(mcast_igmp_data->lflow_ref,
                               lflow_data->lflow_table,
                               eng_ctx->ovnsb_idl_txn,
                               lflow_input.ls_datapaths,
                               lflow_input.lr_datapaths,
                               lflow_input.ovn_internal_version_changed,
                               lflow_input.sbrec_logical_flow_table,
                               lflow_input.sbrec_logical_dp_group_table)) {
        return EN_UNHANDLED;
    }

    return EN_HANDLED_UPDATED;
}

enum engine_input_handler_result
lflow_group_ecmp_route_change_handler(struct engine_node *node,
                                      void *data OVS_UNUSED)
{
    struct group_ecmp_route_data *group_ecmp_route_data =
        engine_get_input_data("group_ecmp_route", node);

    /* If we do not have tracked data we need to recompute. */
    if (!group_ecmp_route_data->tracked) {
        return EN_UNHANDLED;
    }

    const struct engine_context *eng_ctx = engine_get_context();
    struct lflow_data *lflow_data = data;

    struct lflow_input lflow_input;
    lflow_get_input_data(node, &lflow_input);

    struct group_ecmp_datapath *route_node;
    struct hmapx_node *hmapx_node;

    /* We need to handle deletions before additions as they could potentially
     * overlap. */
    HMAPX_FOR_EACH (hmapx_node,
                    &group_ecmp_route_data->trk_data.deleted_datapath_routes) {
        route_node = hmapx_node->data;
        lflow_ref_unlink_lflows(route_node->lflow_ref);

        bool handled = lflow_ref_sync_lflows(
            route_node->lflow_ref, lflow_data->lflow_table,
            eng_ctx->ovnsb_idl_txn, lflow_input.ls_datapaths,
            lflow_input.lr_datapaths,
            lflow_input.ovn_internal_version_changed,
            lflow_input.sbrec_logical_flow_table,
            lflow_input.sbrec_logical_dp_group_table);
        if (!handled) {
            return EN_UNHANDLED;
        }
    }

    /* Now we handle created or updated route nodes. */
    struct hmapx *crupdated_datapath_routes =
        &group_ecmp_route_data->trk_data.crupdated_datapath_routes;
    HMAPX_FOR_EACH (hmapx_node, crupdated_datapath_routes) {
        route_node = hmapx_node->data;
        lflow_ref_unlink_lflows(route_node->lflow_ref);
        build_route_data_flows_for_lrouter(
            route_node->od, lflow_data->lflow_table,
            route_node, lflow_input.bfd_ports);

        bool handled = lflow_ref_sync_lflows(
            route_node->lflow_ref, lflow_data->lflow_table,
            eng_ctx->ovnsb_idl_txn, lflow_input.ls_datapaths,
            lflow_input.lr_datapaths,
            lflow_input.ovn_internal_version_changed,
            lflow_input.sbrec_logical_flow_table,
            lflow_input.sbrec_logical_dp_group_table);
        if (!handled) {
            return EN_UNHANDLED;
        }
    }

    return EN_HANDLED_UPDATED;
}

void *en_lflow_init(struct engine_node *node OVS_UNUSED,
                     struct engine_arg *arg OVS_UNUSED)
{
    struct lflow_data *data = xmalloc(sizeof *data);
    data->lflow_table = lflow_table_alloc();
    lflow_table_init(data->lflow_table);
    return data;
}

void en_lflow_cleanup(void *data_)
{
    struct lflow_data *data = data_;
    lflow_table_destroy(data->lflow_table);
}

void en_lflow_clear_tracked_data(void *data_)
{
    struct lflow_data *data = data_;
    data->handled_incrementally = true;
}

static bool
datapath_is_valid(const struct sbrec_datapath_binding *dp,
                  const struct ovn_synced_logical_switch_map *synced_lses,
                  const struct ovn_synced_logical_router_map *synced_lrs)
{
    enum ovn_datapath_type dp_type = ovn_datapath_type_from_string(dp->type);
    if (dp_type == DP_MAX) {
        return false;
    }
    struct uuid nb_dp_key;
    if (!smap_get_uuid(&dp->external_ids, "nb_uuid", &nb_dp_key)) {
        return false;
    }
    if (dp_type == DP_SWITCH) {
        if (ovn_synced_logical_switch_find(synced_lses, &nb_dp_key)) {
            return true;
        } else {
            return false;
        }
    } else if (dp_type == DP_ROUTER) {
        if (ovn_synced_logical_router_find(synced_lrs, &nb_dp_key)) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

static void
sb_lflows_sync_for_datapaths(
    const struct ovn_synced_logical_switch_map *switches,
    const struct ovn_synced_logical_router_map *routers,
    struct sb_lflows *sb_lflows)
{
    struct sb_lflow *sb_lflow;
    HMAP_FOR_EACH_SAFE (sb_lflow, hmap_node, &sb_lflows->valid) {
        struct sbrec_datapath_binding *dp = sb_lflow->flow->logical_datapath;
        if (dp) {
            if (!datapath_is_valid(dp, switches, routers)) {
                hmap_remove(&sb_lflows->valid, &sb_lflow->hmap_node);
                hmap_insert(&sb_lflows->to_delete, &sb_lflow->hmap_node,
                            hmap_node_hash(&sb_lflow->hmap_node));
            }
        } else if (sb_lflow->flow->logical_dp_group) {
            const struct sbrec_logical_dp_group *dp_group;
            dp_group = sb_lflow->flow->logical_dp_group;
            for (size_t i = 0; i < dp_group->n_datapaths; i++) {
                dp = dp_group->datapaths[i];
                if (datapath_is_valid(dp, switches, routers)) {
                    break;
                }
                dp = NULL;
            }

        }
        if (!dp) {
            hmap_remove(&sb_lflows->valid, &sb_lflow->hmap_node);
            hmap_insert(&sb_lflows->to_delete, &sb_lflow->hmap_node,
                        hmap_node_hash(&sb_lflow->hmap_node));
        }
    }
}

static void
lflow_sync_data_init(struct lflow_sync_data *lflow_sync,
                     const struct sbrec_logical_flow_table *sb_lflow_table)
{
    hmap_init(&lflow_sync->sb_lflows.valid);
    hmap_init(&lflow_sync->sb_lflows.to_delete);

    if (!sb_lflow_table) {
        return;
    }

    const struct sbrec_logical_flow *lflow;
    SBREC_LOGICAL_FLOW_TABLE_FOR_EACH (lflow, sb_lflow_table) {
        struct sb_lflow *sb_lflow = xzalloc(sizeof *sb_lflow);
        sb_lflow->flow = lflow;
        sb_lflow->delete_me = true;
        hmap_insert(&lflow_sync->sb_lflows.valid, &sb_lflow->hmap_node,
                    uuid_hash(&lflow->header_.uuid));
    }
}

static void
lflow_sync_data_destroy(struct lflow_sync_data *lflow_sync)
{
    struct sb_lflow *sb_lflow;
    HMAP_FOR_EACH_SAFE (sb_lflow, hmap_node, &lflow_sync->sb_lflows.valid) {
        hmap_remove(&lflow_sync->sb_lflows.valid, &sb_lflow->hmap_node);
        free(sb_lflow);
    }
    HMAP_FOR_EACH_SAFE (sb_lflow, hmap_node,
                        &lflow_sync->sb_lflows.to_delete) {
        hmap_remove(&lflow_sync->sb_lflows.to_delete, &sb_lflow->hmap_node);
        free(sb_lflow);
    }
    hmap_destroy(&lflow_sync->sb_lflows.valid);
    hmap_destroy(&lflow_sync->sb_lflows.to_delete);
}

enum engine_node_state
en_lflow_sync_run(struct engine_node *node, void *data)
{
    const struct sbrec_logical_flow_table *sb_lflow_table =
        EN_OVSDB_GET(engine_get_input("SB_logical_flow", node));
    const struct ovn_synced_logical_switch_map *synced_lses =
        engine_get_input_data("datapath_synced_logical_switch", node);
    const struct ovn_synced_logical_router_map *synced_lrs =
        engine_get_input_data("datapath_synced_logical_router", node);
    /* XXX The lflow table is currently not treated as const because it
     * contains mutable logical datapath groups. A future commit will
     * separate the dp groups from the lflow_table so that this can be
     * treated as const.
     */
    struct lflow_data *lflow_data =
        engine_get_input_data("lflow", node);
    const struct northd_data *northd =
        engine_get_input_data("northd", node);
    struct ed_type_global_config *global_config =
        engine_get_input_data("global_config", node);
    const struct sbrec_logical_dp_group_table *sb_dp_group_table =
        EN_OVSDB_GET(engine_get_input("SB_logical_dp_group", node));
    const struct engine_context *eng_ctx = engine_get_context();

    struct lflow_sync_data *lflow_sync = data;
    lflow_sync_data_destroy(lflow_sync);
    lflow_sync_data_init(lflow_sync, sb_lflow_table);

    stopwatch_start(LFLOWS_TO_SB_STOPWATCH_NAME, time_msec());

    sb_lflows_sync_for_datapaths(synced_lses, synced_lrs,
                                 &lflow_sync->sb_lflows);
    lflow_table_sync_to_sb(lflow_data->lflow_table, eng_ctx->ovnsb_idl_txn,
                           &northd->ls_datapaths,
                           &northd->lr_datapaths,
                           global_config->ovn_internal_version_changed,
                           &lflow_sync->sb_lflows, sb_dp_group_table);
    lflow_table_sync_finish(&lflow_sync->sb_lflows);

    struct sb_lflow *sb_lflow;
    HMAP_FOR_EACH (sb_lflow, hmap_node, &lflow_sync->sb_lflows.to_delete) {
        sbrec_logical_flow_delete(sb_lflow->flow);
    }

    stopwatch_stop(LFLOWS_TO_SB_STOPWATCH_NAME, time_msec());

    return EN_UPDATED;
}

void *
en_lflow_sync_init(struct engine_node *node OVS_UNUSED,
                        struct engine_arg *arg OVS_UNUSED)
{
    struct lflow_sync_data *lflow_sync = xzalloc(sizeof *lflow_sync);
    lflow_sync_data_init(lflow_sync, NULL);
    return lflow_sync;
}

void
en_lflow_sync_cleanup(void *data)
{
    struct lflow_sync_data *lflow_sync = data;
    lflow_sync_data_destroy(lflow_sync);
}

enum engine_input_handler_result
lflow_sync_lflow_handler(struct engine_node *node, void *data OVS_UNUSED)
{
    const struct lflow_data *lflow_data = engine_get_input_data("lflow", node);

    /* The en-lflow node's handlers sync flows based on lflow_refs. If they
     * were all able to handle the changes incrementally, then there's no need
     * for en-lflow-sync to perform a sync of the entire lflow table.
     */
    if (lflow_data->handled_incrementally) {
        return EN_HANDLED_UPDATED;
    } else {
        return EN_UNHANDLED;
    }
}
