#ifndef EN_LFLOW_H
#define EN_LFLOW_H 1

#include <config.h>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "lib/inc-proc-eng.h"
#include "lflow-mgr.h"

struct lflow_table;
struct sb_lflows;

struct lflow_data {
    struct lflow_table *lflow_table;
    /* When en-lflow incrementally processes lflow_refs, it rebuilds
     * the flows and adds the lflow_refs to this vector so that the
     * en-lflow-sync node can resync the flows with the southbound
     * database
     */
    struct vector lflow_refs;
    /* Not all incremental changes in en-lflow affect the lflow_refs
     * vector above. So we use this as a way of knowing for certain
     * if en-lflow was able to handle all changes incrementally in
     * nodes that take en-lflow as an input.
     */
    bool handled_incrementally;
};

enum engine_node_state en_lflow_run(struct engine_node *node, void *data);
void *en_lflow_init(struct engine_node *node, struct engine_arg *arg);
void en_lflow_cleanup(void *data);
void en_lflow_clear_tracked_data(void *tracked_data);
enum engine_input_handler_result lflow_northd_handler(struct engine_node *,
                                                      void *data);
enum engine_input_handler_result lflow_port_group_handler(struct engine_node *,
                                                          void *data);
enum engine_input_handler_result
lflow_lr_stateful_handler(struct engine_node *, void *data);
enum engine_input_handler_result
lflow_ls_stateful_handler(struct engine_node *node, void *data);
enum engine_input_handler_result
lflow_multicast_igmp_handler(struct engine_node *node, void *data);
enum engine_input_handler_result
lflow_group_ecmp_route_change_handler(struct engine_node *node, void *data);

struct sb_lflows;
struct lflow_sync_data {
    struct sb_lflows sb_lflows;
};

enum engine_node_state en_lflow_sync_run(struct engine_node *, void *data);
void *en_lflow_sync_init(struct engine_node *, struct engine_arg *);
void en_lflow_sync_cleanup(void *data);
enum engine_input_handler_result lflow_sync_lflow_handler(struct engine_node *,
                                                          void *data);

#endif /* EN_LFLOW_H */
