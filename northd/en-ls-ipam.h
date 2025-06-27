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
#ifndef EN_LS_IPAM_H
#define EN_LS_IPAM_H 1

#include "lib/inc-proc-eng.h"
#include "openvswitch/hmap.h"
#include "openvswitch/list.h"
#include "northd/northd.h"
#include <netinet/in.h>

void *en_ls_ipam_init(struct engine_node *, struct engine_arg *);
void en_ls_ipam_cleanup(void *data);
enum engine_node_state en_ls_ipam_run(struct engine_node *, void *data);
enum engine_input_handler_result ls_ipam_handler(struct engine_node *, void *);

#endif /* EN_LS_IPAM_H */