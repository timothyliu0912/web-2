#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <string.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#define MAC_ADDRSTRLEN 2*6+5+1
struct ip_pair
{
    char src[50];
    char dst[50];
    int cnt;
};
int len = 0;
int ip_total = 0;
void print_time(struct pcap_pkthdr *header)
{
    struct tm *ltime;
    char timestr[21];
    ltime = localtime(&header->ts.tv_sec);
    strftime(timestr, sizeof timestr, "%Y-%m-%e:%H:%M:%S", ltime);
    printf("| Time: %s\n",timestr);
}
void print_macaddr(unsigned char *mac_addr)
{
    for(int i = 0; i < 6; i++)
    {
        printf("%02x",*(mac_addr+i));
        if(i!=5) printf(":");
    }
    printf("\n");
}
char* print_ipv6(void *i)
{
    static char str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, i, str, sizeof(str));
    printf("%s\n",str);
    return str;
}
char* print_ip(void *i)
{
    static char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, i, str, sizeof(str));
    printf("%s\n",str);
    return str;
}
void dump_tcp(u_int32_t length,const u_char *content)
{
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct tcphdr *tcp = (struct tcphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));
    u_int16_t sur_port = ntohs(tcp->th_sport);
    u_int16_t des_port = ntohs(tcp->th_dport);
    printf("| Sour. Port: %d\n| Dest. Port: %d\n",sur_port,des_port);
}
void dump_udp(u_int32_t length,const u_char *content)
{
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct udphdr *udp = (struct udphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));
    u_int16_t sur_port = ntohs(udp->uh_sport);
    u_int16_t des_port = ntohs(udp->uh_dport);
    printf("|- Sour. Port: %d\n|- Dest. Port: %d\n",sur_port,des_port);
}
void check_ip(char *source,char *dest,struct ip_pair arr[])
{
    int flag = 0;
    for(int i = 0; i < len; i++)
    {
        if( !strcmp(source,arr[i].src) && !strcmp(dest,arr[i].dst) )
        {
            arr[i].cnt ++;
            flag = 1;
            break;
        }
    }
    if(!flag)
    {
        strcpy(arr[len].src, source);
        strcpy(arr[len].dst, dest);
        arr[len].cnt = 1;
        len++;
    }
}
void dump_ip(u_int32_t length,const u_char *content,struct ip_pair arr[])
{   
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    u_char protocol = ip->ip_p;
    printf("|- Sour. IP Addr: ");
    char *src;
    src = print_ip(&ip->ip_src);
    printf("|- Dest. IP Addr: ");
    char *dst;
    dst = print_ip(&ip->ip_dst);
    check_ip(src,dst,arr);
    ip_total++;
    switch (protocol) 
    {
        case IPPROTO_UDP:
            printf("| UDP packet:\n");
            dump_udp(length, content);
            break;

        case IPPROTO_TCP:
            printf("| TCP packet:\n");
            dump_tcp(length, content);
            break;
    }
}
void dump_ipv6(u_int32_t length,const u_char *content,struct ip_pair arr[])
{
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    u_char protocol = ip->ip_p;
    printf("|- Sour. IP Addr: ");
    char *src;
    src = print_ipv6(&ip->ip_src);
    printf("|- Dest. IP Addr: ");
    char *dst;
    dst = print_ipv6(&ip->ip_src);
    check_ip(src,dst,arr);
    ip_total++;
    switch (protocol) 
    {
        case IPPROTO_UDP:
            printf("| UDP packet:\n");
            dump_udp(length, content);
            break;

        case IPPROTO_TCP:
            printf("| TCP packet:\n");
            dump_tcp(length, content);
            break;
    }
}
int main(int argc, const char * argv[]) 
{
    struct ip_pair arr[1000];
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handler = NULL;
    int num = 1;
    int flag = 0;
    int lenflag = 0;
    if(!strcmp(argv[1],"-r"))
    {
        handler = pcap_open_offline(argv[2], errbuf);
        if(!handler)
        {
            fprintf(stderr, "pcap_open_offline: %s\n", errbuf);
            exit(1);
        }
    }
    if(!strcmp(argv[1],"-c"))
    {
        if(!strcmp(argv[2],"-n"))
        {
            num = atoi(argv[3]);
            flag = 1;
        }
        else
            flag = 0;
        handler = pcap_open_live("en0",65535, 1, 1, errbuf);
        if(!handler) 
        {
            fprintf(stderr, "pcap_open_live: %s\n", errbuf);
            exit(1);
        }
    }
    if(!strcmp(argv[1],"-v"))
    {
        printf("Home work 3(HW3) 1.0.0\n");
        return 0;
    }
    while(num)
    {
        struct pcap_pkthdr *header = NULL;
        const u_char *content = NULL;
        int ret = pcap_next_ex(handler, &header, &content);
        if(ret == 1) 
        {
            u_int16_t type;
            unsigned short ethernet_type = 0;
            struct ether_header *ethernet = (struct ether_header *)content;
            printf("+-----------------------------------------------+\n");
            print_time(header);
            unsigned char *mac_addr = NULL;	
            mac_addr = (unsigned char *)ethernet->ether_shost;//獲取源mac
            printf("| Mac Sour. Addr: ");
            print_macaddr(mac_addr);
            mac_addr = (unsigned char *)ethernet->ether_dhost;//獲取目的mac
            printf("| Mac Dest. Addr: ");
            print_macaddr(mac_addr);
            printf("| Length: %d bytes\n", header->len);
            ethernet_type = ntohs(ethernet->ether_type);
            switch(ethernet_type)
            {
	            case ETHERTYPE_IP:
                printf("| IP:\n");
                dump_ip(header->caplen,content,arr);
                break;
                case ETHERTYPE_IPV6:
                printf("| IPv6:\n");
                dump_ipv6(header->caplen,content,arr);
                break;
            }
            printf("+-----------------------------------------------+\n\n");
        }
        else if(ret == -1) fprintf(stderr, "pcap_next_ex(): %s\n", pcap_geterr(handler));
        else if(ret == -2) break;
        if(flag) num--;
    }
    printf("IP Package: %d\n",ip_total);
    printf("--------------------\n");
    for(int i = 0; i < len; i++)
    {
        printf("Sour: %s\n",arr[i].src);
        printf("Dest: %s\n",arr[i].dst);
        printf("Cunt: %d\n",arr[i].cnt);
        printf("--------------------\n");
    }
    pcap_close(handler);
    return 0;
}
