// SPDX-License-Identifier: GPL-2.0

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* TC 返回值 */
#define TC_ACT_OK       0
#define TC_ACT_SHOT     2
#define TC_ACT_UNSPEC   -1
//网络统计结构体-与用户空间共享
struct net_stats{
    __u64 rcv_bytes;
    __u64 rcv_packets;
    __u64 snd_bytes;
    __u64 snd_packets;
};
struct{
    __uint(type,BPF_MAP_TYPE_HASH);
    __uint(max_entries,64);
    __type(key,__u32);
    __type(value,struct net_stats);
}net_stats_map SEC(".maps");
static __always_inline void update_stats(__u32 ifindex,__u32 len,bool is_rx){
    struct net_stats* stats;
    struct net_stats new_stats={};
    stats=bpf_map_lookup_elem(&net_stats_map,&ifindex);

    if(!stats){
        if(is_rx){
            new_stats.rcv_bytes=len;
            new_stats.rcv_packets=1;
        }else{
            new_stats.snd_bytes=len;
            new_stats.snd_packets=1;
        }
        bpf_map_update_elem(&net_stats_map,&ifindex,&new_stats,BPF_ANY);
    }else{
        if(is_rx){
            __sync_fetch_and_add(&stats->rcv_bytes,len);
            __sync_fetch_and_add(&stats->rcv_packets,1);
        }else{
            __sync_fetch_and_add(&stats->snd_bytes,len);
            __sync_fetch_and_add(&stats->snd_packets,1);
        }
    }
    
}
SEC("tc/ingress")
int tc_ingress(struct __sk_buff* skb){
    __u32 ifindex=skb->ifindex;
    __u32 len=skb->len;
    if(ifindex==0||len==0)
       return TC_ACT_OK;
    update_stats(ifindex,len,true);
    return TC_ACT_OK;
}
SEC("tc/egress")
int tc_egress(struct __sk_buff* skb){
    __u32 ifindex = skb->ifindex;
    __u32 len = skb->len;

    /* 过滤无效数据 */
    if (ifindex == 0 || len == 0)
        return TC_ACT_OK;

    update_stats(ifindex, len, false);

    /* TC_ACT_OK: 继续正常处理，不影响数据包 */
    return TC_ACT_OK;
}

char LICENSE[] SEC("license") = "GPL";
