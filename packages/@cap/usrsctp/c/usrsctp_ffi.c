#include "usrsctp.h"
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

static int cp_usrsctp_noop_output(void* addr, void* buf, size_t len, uint8_t tos, uint8_t set_df) {
    (void)addr;
    (void)buf;
    (void)len;
    (void)tos;
    (void)set_df;
    return 0;
}

int32_t cp_usrsctp_init(uint16_t udp_port) {
    usrsctp_init(udp_port, cp_usrsctp_noop_output, NULL);
    return 0;
}

int32_t cp_usrsctp_finish(void) {
    for (int i = 0; i < 200; i++) {
        if (usrsctp_finish() == 0) {
            return 0;
        }
        usleep(10000);
    }
    return -1;
}

void* cp_usrsctp_socket_create(int32_t domain, int32_t sock_type) {
    int dom = (domain == 0) ? AF_CONN : (int)domain;
    int stype = (sock_type == 0) ? SOCK_STREAM : (int)sock_type;
    return (void*)usrsctp_socket(dom, stype, IPPROTO_SCTP, NULL, NULL, 0, NULL);
}

void cp_usrsctp_socket_close(void* sock) {
    if (sock != NULL) {
        usrsctp_close((struct socket*)sock);
    }
}

int32_t cp_usrsctp_socket_create_and_close(void) {
    struct socket* so = usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP, NULL, NULL, 0, NULL);
    if (so == NULL) {
        return -1;
    }
    usrsctp_close(so);
    return 0;
}
