// High-performance multi-threaded real TCP RFC 6455 WebSocket Echo load generator
// Compiles cleanly with: gcc -O3 -pthread ws_bench.c -o ws_bench_bin
// Sends Masked Text frames of configurable payload size (default 128B) over persistent
// WebSocket connections, verifies unmasked Echo frames, and reports JSON metrics.

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_LAT_SAMPLES 65536

typedef struct {
    const char *host;
    int port;
    size_t payload_size;
    double duration_sec;
    double warmup_sec;
    uint64_t ok_count;
    uint64_t err_count;
    uint64_t total_bytes;
    uint32_t *lat_us;
    size_t lat_count;
    uint64_t sample_stride;
} ws_worker_arg_t;

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int send_all(int fd, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

static int recv_exact(int fd, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, buf + got, len - got, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        got += (size_t)n;
    }
    return 0;
}

static int connect_and_handshake(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    char req[512];
    int req_len = snprintf(
        req, sizeof(req),
        "GET /ws HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        host, port
    );
    if (send_all(fd, (const uint8_t *)req, (size_t)req_len) < 0) {
        close(fd);
        return -1;
    }

    char resp[2048];
    size_t used = 0;
    char *hdr_end = NULL;
    while (used < sizeof(resp) - 1) {
        ssize_t n = recv(fd, resp + used, sizeof(resp) - 1 - used, 0);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        used += (size_t)n;
        resp[used] = '\0';
        hdr_end = strstr(resp, "\r\n\r\n");
        if (hdr_end) break;
    }
    if (!hdr_end || strstr(resp, "101") == NULL || strstr(resp, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") == NULL) {
        close(fd);
        return -1;
    }
    return fd;
}

// Reads one unmasked server WebSocket frame and validates opcode + payload length.
static ssize_t read_ws_echo_frame(int fd, uint8_t *payload_buf, size_t max_payload, size_t expected_len) {
    uint8_t hdr[2];
    if (recv_exact(fd, hdr, 2) < 0) return -1;

    uint8_t opcode = hdr[0] & 0x0F;
    uint8_t masked = (hdr[1] & 0x80) != 0;
    uint64_t plen = hdr[1] & 0x7F;
    if (plen == 126) {
        uint8_t ext[2];
        if (recv_exact(fd, ext, 2) < 0) return -1;
        plen = ((uint64_t)ext[0] << 8) | (uint64_t)ext[1];
    } else if (plen == 127) {
        uint8_t ext[8];
        if (recv_exact(fd, ext, 8) < 0) return -1;
        plen = 0;
        for (int i = 0; i < 8; i++) plen = (plen << 8) | ext[i];
    }
    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (recv_exact(fd, mask, 4) < 0) return -1;
    }
    if (plen > max_payload) return -1;
    if (plen > 0) {
        if (recv_exact(fd, payload_buf, (size_t)plen) < 0) return -1;
        if (masked) {
            for (size_t i = 0; i < (size_t)plen; i++) payload_buf[i] ^= mask[i & 3];
        }
    }
    if ((opcode != 0x1 && opcode != 0x2) || (size_t)plen != expected_len) return -1;
    return (ssize_t)plen;
}

static void *ws_worker_fn(void *arg) {
    ws_worker_arg_t *w = (ws_worker_arg_t *)arg;
    size_t plen = w->payload_size;

    uint8_t *plain = malloc(plen);
    uint8_t *tx_frame = malloc(plen + 16);
    uint8_t *rx_payload = malloc(plen + 16);

    for (size_t i = 0; i < plen; i++) {
        plain[i] = (uint8_t)('A' + (i % 26));
    }

    const uint8_t mask[4] = {0x37, 0xFA, 0x21, 0x3D};
    size_t hdr_len = 0;
    tx_frame[0] = 0x81; // FIN + Text
    if (plen < 126) {
        tx_frame[1] = (uint8_t)(0x80 | plen);
        hdr_len = 2;
    } else {
        tx_frame[1] = (uint8_t)(0x80 | 126);
        tx_frame[2] = (uint8_t)((plen >> 8) & 0xFF);
        tx_frame[3] = (uint8_t)(plen & 0xFF);
        hdr_len = 4;
    }
    memcpy(tx_frame + hdr_len, mask, 4);
    hdr_len += 4;
    for (size_t i = 0; i < plen; i++) {
        tx_frame[hdr_len + i] = plain[i] ^ mask[i & 3];
    }
    size_t tx_len = hdr_len + plen;

    int fd = connect_and_handshake(w->host, w->port);

    uint64_t t_start = now_ns();
    uint64_t t_warmup_end = t_start + (uint64_t)(w->warmup_sec * 1e9);
    uint64_t t_end = t_warmup_end + (uint64_t)(w->duration_sec * 1e9);
    uint64_t iter = 0;

    while (1) {
        uint64_t t0 = now_ns();
        if (t0 >= t_end) break;
        int recording = (t0 >= t_warmup_end);

        if (fd < 0) {
            fd = connect_and_handshake(w->host, w->port);
            if (fd < 0) {
                if (recording) w->err_count++;
                usleep(1000);
                continue;
            }
        }

        if (send_all(fd, tx_frame, tx_len) < 0) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        ssize_t r = read_ws_echo_frame(fd, rx_payload, plen + 16, plen);
        uint64_t t1 = now_ns();
        if (r != (ssize_t)plen || memcmp(rx_payload, plain, plen) != 0) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        if (recording) {
            w->ok_count++;
            w->total_bytes += (uint64_t)plen;
            if ((iter++ % w->sample_stride) == 0 && w->lat_count < MAX_LAT_SAMPLES) {
                w->lat_us[w->lat_count++] = (uint32_t)((t1 - t0) / 1000ULL);
            }
        }
    }

    if (fd >= 0) close(fd);
    free(plain);
    free(tx_frame);
    free(rx_payload);
    return NULL;
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    int port = 3200;
    int threads = 32;
    size_t payload_size = 128;
    double duration = 3.0;
    double warmup = 0.5;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--payload-size") == 0 && i + 1 < argc) payload_size = (size_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) duration = atof(argv[++i]);
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) warmup = atof(argv[++i]);
    }

    pthread_t *tids = calloc((size_t)threads, sizeof(pthread_t));
    ws_worker_arg_t *args = calloc((size_t)threads, sizeof(ws_worker_arg_t));
    for (int i = 0; i < threads; i++) {
        args[i].host = host;
        args[i].port = port;
        args[i].payload_size = payload_size;
        args[i].duration_sec = duration;
        args[i].warmup_sec = warmup;
        args[i].lat_us = malloc(MAX_LAT_SAMPLES * sizeof(uint32_t));
        args[i].lat_count = 0;
        args[i].sample_stride = 4;
    }

    uint64_t wall_t0 = now_ns();
    for (int i = 0; i < threads; i++) {
        pthread_create(&tids[i], NULL, ws_worker_fn, &args[i]);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }
    double actual_sec = (double)(now_ns() - wall_t0) / 1e9 - warmup;
    if (actual_sec <= 0.01) actual_sec = duration;

    uint64_t total_ok = 0, total_err = 0, total_bytes = 0;
    size_t total_samples = 0;
    for (int i = 0; i < threads; i++) {
        total_ok += args[i].ok_count;
        total_err += args[i].err_count;
        total_bytes += args[i].total_bytes;
        total_samples += args[i].lat_count;
    }

    uint32_t *all_lat = malloc((total_samples > 0 ? total_samples : 1) * sizeof(uint32_t));
    size_t pos = 0;
    for (int i = 0; i < threads; i++) {
        if (args[i].lat_count > 0) {
            memcpy(all_lat + pos, args[i].lat_us, args[i].lat_count * sizeof(uint32_t));
            pos += args[i].lat_count;
        }
        free(args[i].lat_us);
    }

    double p50_ms = 0.0, p99_ms = 0.0;
    if (total_samples > 0) {
        qsort(all_lat, total_samples, sizeof(uint32_t), cmp_u32);
        p50_ms = (double)all_lat[(size_t)(total_samples * 0.50)] / 1000.0;
        p99_ms = (double)all_lat[(size_t)(total_samples * 0.99)] / 1000.0;
    }
    free(all_lat);
    free(args);
    free(tids);

    double msg_s = (double)total_ok / actual_sec;
    double mb_s = ((double)total_bytes / (1024.0 * 1024.0)) / actual_sec;
    printf("{\"msg_per_sec\":%.2f,\"p50_ms\":%.3f,\"p99_ms\":%.3f,\"ok\":%llu,\"err\":%llu,\"mb_per_sec\":%.2f,\"duration_sec\":%.3f}\n",
           msg_s, p50_ms, p99_ms,
           (unsigned long long)total_ok, (unsigned long long)total_err,
           mb_s, actual_sec);
    return 0;
}
