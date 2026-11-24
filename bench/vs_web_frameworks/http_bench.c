// High-performance multi-threaded HTTP/1.1 Keep-Alive load generator (wrk-class)
// Compiles cleanly with: gcc -O3 -pthread http_bench.c -o http_bench_bin
// Outputs JSON with QPS, P50/P99 latency (ms), ok/err counts, and MB/s.

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
    const char *req_buf;
    size_t req_len;
    double duration_sec;
    double warmup_sec;
    int conns_per_thread;
    uint64_t ok_count;
    uint64_t err_count;
    uint64_t total_bytes;
    uint32_t *lat_us;
    size_t lat_count;
    uint64_t sample_stride;
} worker_arg_t;

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int connect_tcp(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct timeval tv;
    tv.tv_sec = 2;
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
    return fd;
}

// Reads one complete HTTP/1.1 response (Content-Length or chunked or single-packet).
// Returns bytes read (>0) if HTTP 200, -1 on error/disconnect.
static ssize_t read_http_response(int fd, char *buf, size_t cap) {
    size_t used = 0;
    char *hdr_end = NULL;
    while (used < cap - 1) {
        ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
        if (n <= 0) return -1;
        used += (size_t)n;
        buf[used] = '\0';
        hdr_end = strstr(buf, "\r\n\r\n");
        if (hdr_end) break;
    }
    if (!hdr_end) return -1;

    // Verify status 200
    if (used < 12 || (memcmp(buf + 9, "200", 3) != 0)) {
        return -1;
    }

    size_t hdr_len = (size_t)(hdr_end - buf) + 4;
    // Look for Content-Length (case-insensitive search)
    long content_len = -1;
    char *p = buf;
    while (p < hdr_end) {
        char *eol = strstr(p, "\r\n");
        if (!eol || eol > hdr_end) break;
        if (eol - p > 15 && strncasecmp(p, "Content-Length:", 15) == 0) {
            content_len = strtol(p + 15, NULL, 10);
            break;
        }
        p = eol + 2;
    }

    if (content_len >= 0) {
        size_t total_need = hdr_len + (size_t)content_len;
        while (used < total_need && used < cap - 1) {
            ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
            if (n <= 0) return -1;
            used += (size_t)n;
        }
        return (used >= total_need) ? (ssize_t)used : -1;
    }

    // Fallback for Transfer-Encoding: chunked -> wait for 0\r\n\r\n
    while (!strstr(buf + hdr_len, "0\r\n\r\n") && used < cap - 1) {
        ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
        if (n <= 0) break;
        used += (size_t)n;
        buf[used] = '\0';
    }
    return (ssize_t)used;
}

static void *worker_fn(void *arg) {
    worker_arg_t *w = (worker_arg_t *)arg;
    char buf[8192];
    int fd = connect_tcp(w->host, w->port);

    uint64_t t_start = now_ns();
    uint64_t t_warmup_end = t_start + (uint64_t)(w->warmup_sec * 1e9);
    uint64_t t_end = t_warmup_end + (uint64_t)(w->duration_sec * 1e9);
    uint64_t iter = 0;

    while (1) {
        uint64_t t0 = now_ns();
        if (t0 >= t_end) break;
        int recording = (t0 >= t_warmup_end);

        if (fd < 0) {
            fd = connect_tcp(w->host, w->port);
            if (fd < 0) {
                if (recording) w->err_count++;
                usleep(1000);
                continue;
            }
        }

        ssize_t sent = send(fd, w->req_buf, w->req_len, MSG_NOSIGNAL);
        if (sent != (ssize_t)w->req_len) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        ssize_t r = read_http_response(fd, buf, sizeof(buf));
        uint64_t t1 = now_ns();
        if (r < 0) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        if (recording) {
            w->ok_count++;
            w->total_bytes += (uint64_t)r;
            if ((iter++ % w->sample_stride) == 0 && w->lat_count < MAX_LAT_SAMPLES) {
                w->lat_us[w->lat_count++] = (uint32_t)((t1 - t0) / 1000ULL);
            }
        }
    }

    if (fd >= 0) close(fd);
    return NULL;
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    int port = 3000;
    const char *method = "GET";
    const char *path = "/";
    const char *body = "";
    const char *content_type = NULL;
    int threads = 16;
    double duration = 3.0;
    double warmup = 0.5;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) method = argv[++i];
        else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) path = argv[++i];
        else if (strcmp(argv[i], "--body") == 0 && i + 1 < argc) body = argv[++i];
        else if (strcmp(argv[i], "--content-type") == 0 && i + 1 < argc) content_type = argv[++i];
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) duration = atof(argv[++i]);
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) warmup = atof(argv[++i]);
    }

    char req_buf[4096];
    size_t body_len = strlen(body);
    int req_len;
    if (body_len > 0) {
        req_len = snprintf(req_buf, sizeof(req_buf),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n\r\n%s",
            method, path, host, port,
            content_type ? content_type : "application/json",
            body_len, body);
    } else {
        req_len = snprintf(req_buf, sizeof(req_buf),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Connection: keep-alive\r\n\r\n",
            method, path, host, port);
    }

    pthread_t *tids = calloc((size_t)threads, sizeof(pthread_t));
    worker_arg_t *args = calloc((size_t)threads, sizeof(worker_arg_t));
    for (int i = 0; i < threads; i++) {
        args[i].host = host;
        args[i].port = port;
        args[i].req_buf = req_buf;
        args[i].req_len = (size_t)req_len;
        args[i].duration_sec = duration;
        args[i].warmup_sec = warmup;
        args[i].lat_us = malloc(MAX_LAT_SAMPLES * sizeof(uint32_t));
        args[i].lat_count = 0;
        args[i].sample_stride = 4;
    }

    uint64_t wall_t0 = now_ns();
    for (int i = 0; i < threads; i++) {
        pthread_create(&tids[i], NULL, worker_fn, &args[i]);
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

    double qps = (double)total_ok / actual_sec;
    double mb_s = ((double)total_bytes / (1024.0 * 1024.0)) / actual_sec;
    printf("{\"qps\":%.2f,\"p50_ms\":%.3f,\"p99_ms\":%.3f,\"ok\":%llu,\"err\":%llu,\"mb_per_sec\":%.2f,\"duration_sec\":%.3f}\n",
           qps, p50_ms, p99_ms,
           (unsigned long long)total_ok, (unsigned long long)total_err,
           mb_s, actual_sec);
    return 0;
}
