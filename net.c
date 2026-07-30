#include "net.h"
#include "e1000.h"
#include "terminal.h"
#include <stddef.h>

static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }

#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPV4 0x0800
#define ARP_REQUEST    1
#define ARP_REPLY      2
#define IP_PROTO_ICMP  1
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

typedef struct __attribute__((packed)) {
    uint8_t  dest[6];
    uint8_t  src[6];
    uint16_t ethertype;
} eth_header_t;

typedef struct __attribute__((packed)) {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint32_t spa;
    uint8_t  tha[6];
    uint32_t tpa;
} arp_packet_t;

typedef struct __attribute__((packed)) {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ipv4_header_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_header_t;

static uint8_t  my_mac[6];
static uint32_t my_ip;

static uint16_t checksum16(const void* data, int len) {
    const uint16_t* p = (const uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len == 1) sum += *(const uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

void net_init(uint32_t ip_addr) {
    e1000_get_mac(my_mac);
    my_ip = ip_addr;
}

static void send_arp_reply(arp_packet_t* req, eth_header_t* req_eth) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    eth_header_t* eth = (eth_header_t*)frame;
    arp_packet_t* arp = (arp_packet_t*)(frame + sizeof(eth_header_t));

    for (int i = 0; i < 6; i++) { eth->dest[i] = req_eth->src[i]; eth->src[i] = my_mac[i]; }
    eth->ethertype = swap16(ETHERTYPE_ARP);

    arp->htype = swap16(1);
    arp->ptype = swap16(ETHERTYPE_IPV4);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = swap16(ARP_REPLY);
    for (int i = 0; i < 6; i++) arp->sha[i] = my_mac[i];
    arp->spa = my_ip;
    for (int i = 0; i < 6; i++) arp->tha[i] = req->sha[i];
    arp->tpa = req->spa;

    e1000_send(frame, sizeof(frame));
}

static void send_icmp_reply(eth_header_t* req_eth, ipv4_header_t* req_ip,
                             icmp_header_t* req_icmp, const uint8_t* payload, int payload_len) {
    uint8_t frame[1514];
    eth_header_t*  eth  = (eth_header_t*)frame;
    ipv4_header_t* ip   = (ipv4_header_t*)(frame + sizeof(eth_header_t));
    icmp_header_t* icmp = (icmp_header_t*)(frame + sizeof(eth_header_t) + sizeof(ipv4_header_t));
    uint8_t* data = frame + sizeof(eth_header_t) + sizeof(ipv4_header_t) + sizeof(icmp_header_t);

    if (payload_len > 1400) payload_len = 1400; /* biztonsagi korlat */

    for (int i = 0; i < 6; i++) { eth->dest[i] = req_eth->src[i]; eth->src[i] = my_mac[i]; }
    eth->ethertype = swap16(ETHERTYPE_IPV4);

    uint16_t total_len = (uint16_t)(sizeof(ipv4_header_t) + sizeof(icmp_header_t) + payload_len);
    ip->ver_ihl    = 0x45;
    ip->tos        = 0;
    ip->total_len  = swap16(total_len);
    ip->id         = 0;
    ip->flags_frag = 0;
    ip->ttl        = 64;
    ip->protocol   = IP_PROTO_ICMP;
    ip->checksum   = 0;
    ip->src_ip     = my_ip;
    ip->dst_ip     = req_ip->src_ip;
    ip->checksum   = checksum16(ip, sizeof(ipv4_header_t));

    icmp->type     = ICMP_ECHO_REPLY;
    icmp->code     = 0;
    icmp->id       = req_icmp->id;
    icmp->seq      = req_icmp->seq;
    icmp->checksum = 0;
    for (int i = 0; i < payload_len; i++) data[i] = payload[i];
    icmp->checksum = checksum16(icmp, (int)(sizeof(icmp_header_t) + (size_t)payload_len));

    e1000_send(frame, (uint16_t)(sizeof(eth_header_t) + total_len));
}

static void handle_frame(uint8_t* buf, uint16_t len) {
    if (len < sizeof(eth_header_t)) return;
    eth_header_t* eth = (eth_header_t*)buf;

    if (eth->ethertype == swap16(ETHERTYPE_ARP)) {
        if (len < sizeof(eth_header_t) + sizeof(arp_packet_t)) return;
        arp_packet_t* arp = (arp_packet_t*)(buf + sizeof(eth_header_t));
        if (arp->oper == swap16(ARP_REQUEST) && arp->tpa == my_ip)
            send_arp_reply(arp, eth);
    }
    else if (eth->ethertype == swap16(ETHERTYPE_IPV4)) {
        if (len < sizeof(eth_header_t) + sizeof(ipv4_header_t)) return;
        ipv4_header_t* ip = (ipv4_header_t*)(buf + sizeof(eth_header_t));
        int ihl_bytes = (ip->ver_ihl & 0xF) * 4;

        if (ip->dst_ip == my_ip && ip->protocol == IP_PROTO_ICMP) {
            icmp_header_t* icmp = (icmp_header_t*)((uint8_t*)ip + ihl_bytes);
            if (icmp->type == ICMP_ECHO_REQUEST) {
                int icmp_total   = swap16(ip->total_len) - ihl_bytes;
                int payload_len  = icmp_total - (int)sizeof(icmp_header_t);
                uint8_t* payload = (uint8_t*)icmp + sizeof(icmp_header_t);
                if (payload_len < 0) payload_len = 0;
                send_icmp_reply(eth, ip, icmp, payload, payload_len);
            }
        }
    }
}

void net_poll(void) {
    uint8_t* buf;
    uint16_t len;
    while (e1000_poll(&buf, &len))
        handle_frame(buf, len);
}
