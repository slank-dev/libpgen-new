


// #include <pgen2/core/protocol/ethernet.h>
// #include <pgen2/core/protocol/ip.h>
// #include <arpa/inet.h>
#include <pgen2/core/protocol/udp.h>
#include <pgen2/exception.h>


namespace pgen {
    


struct udp_struct {
    uint16_t src;
    uint16_t dst;
    uint16_t len;
    uint16_t check;
};



void udp_header::clear() {
    src = 53;
    dst = 53;
    len = 0;
    check  = 0;
}



void udp_header::summary(bool moreinfo) const {
    printf("UDP [%d -> %d] \n", src, dst);

    if (moreinfo) {
        printf(" - source         : %d \n", src);
        printf(" - destination    : %d \n", dst);
        printf(" - total length   : %u \n", len);
        printf(" - checksum       : 0x%02x \n", check);
    }
}


void udp_header::write(void* buffer, size_t buffer_len) const {
    if (buffer_len < min_length) {
        throw pgen::exception("pgen::udp_header::read: buflen is too small");
    }

    struct udp_struct* p = reinterpret_cast<udp_struct*>(buffer);
    p->src   = htons(src);
    p->dst   = htons(dst);
    p->len   = htons(len);
    p->check = htons(check);
}



void udp_header::read(const void* buffer, size_t buffer_len) {
    if (buffer_len < min_length) {
        throw pgen::exception("pgen::udp_header::read: buflen is too small");
    }
     
    const struct udp_struct* p = 
        reinterpret_cast<const udp_struct*>(buffer);
    src   = ntohs(p->src);
    dst   = ntohs(p->dst);
    len   = ntohs(p->len);
    check = ntohs(p->check);
}



size_t udp_header::length() const {
    return min_length;
}


// TODO not tested and debuging
uint16_t udp_header::calc_checksum(const ipv4_header& iph,
        const void* data, size_t datalen) const {
    
    struct pseudo_header {
        uint8_t ip_src[4];
        uint8_t ip_dst[4];
        uint8_t  zero;
        uint8_t  ip_proto;
        uint16_t ip_len;
    };
    uint8_t buffer[sizeof(pseudo_header)+length()+datalen];
    uint8_t* p = buffer;
    memset(&buffer, 0, sizeof buffer);

    struct pseudo_header* ph =
        reinterpret_cast<pseudo_header*>(p);
    iph.src.copytoarray(ph->ip_src);
    iph.dst.copytoarray(ph->ip_dst);
    ph->zero = 0x00;
    ph->ip_proto = iph.protocol;
    ph->ip_len   = htons(iph.tot_len-iph.length());
    p += sizeof(pseudo_header);
    
    write(p, min_length);
    p += length();

    memcpy(p, data, datalen);
    p += datalen;

    if (sizeof(buffer) != static_cast<size_t>(p-buffer)) {
        throw pgen::exception(
                "pgen::udp_header::calc_checksum: error");
    }

    return pgen::checksum(buffer, sizeof(buffer));
}



// ipv4::ipv4() {
//     clear();
//     init_headers();
// }
//
//
// ipv4::ipv4(const void* buffer, size_t bufferlen) : ipv4() {
//     analyze(buffer, bufferlen);
// }
//
//
// ipv4::ipv4(const pgen::ipv4& rhs) : packet(rhs) {
//     ETH = rhs.ETH;
//     IP  = rhs.IP;
//     init_headers();
// }
//
//
//
//
// void ipv4::clear() {
//     ETH.clear();
//     ETH.type = ethernet::type::ip;
//     IP.clear();
// }
//
//
//
// void ipv4::init_headers() {
//     headers = {&ETH, &IP};
// }




} /* namespace pgen */
