#ifndef NORTHD_IPAM_H
#define NORTHD_IPAM_H 1

#include <stdint.h>
#include <stdbool.h>

#include "openvswitch/types.h"
#include "lib/ovn-util.h"
struct ipam_info {
    uint32_t start_ipv4;
    size_t total_ipv4s;
    unsigned long *allocated_ipv4s; /* A bitmap of allocated IPv4s */
    bool ipv6_prefix_set;
    struct in6_addr ipv6_prefix;
    bool mac_only;
    const char *id;
};

struct smap;
struct ovn_datapath;
struct ovn_port;

void init_ipam_info(struct ipam_info *info, const struct smap *config,
                    const char *id);

void init_ipam_info_for_datapath(struct ovn_datapath *od);

void destroy_ipam_info(struct ipam_info *info);

void ipam_add_port_addresses(struct ovn_datapath *od, struct ovn_port *op);

bool ipam_insert_ip(struct ipam_info *info, uint32_t ip, bool dynamic);

void ipam_insert_ip_for_datapath(struct ovn_datapath *od, uint32_t ip, bool dynamic);

uint32_t ipam_get_unused_ip(struct ipam_info *info);

void ipam_insert_mac(struct eth_addr *ea, bool check);

uint64_t ipam_get_unused_mac(ovs_be32 ip);

void cleanup_macam(void);

void remove_mac_from_macam(struct eth_addr *ea);

struct eth_addr get_mac_prefix(void);

const char *set_mac_prefix(const char *hint);

void
ipam_insert_lsp_addresses(struct ovn_datapath *od,
                          struct lport_addresses *laddrs);

void update_ipam_from_lsp(struct ovn_datapath *, struct ovn_port *);

#endif /* NORTHD_IPAM_H */
