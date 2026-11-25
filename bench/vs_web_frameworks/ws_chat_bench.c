// High-performance non-blocking Linux epoll RFC 6455 WebSocket Chat Broadcast load generator
// Compiles cleanly with: gcc -O3 ws_chat_bench.c -o /data/dev/cp/temp/ws_chat_bench_bin
//
// Opens N (default 32) persistent TCP connections, performs RFC 6455 WebSocket upgrade,
// keeps up to `inflight` masked text messages in flight per client while draining all
// broadcast frames across all clients via epoll, and outputs JSON metrics.

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define RX_BUF_CAP (256 * 1024)
#define TX_BUF_CAP (64 * 1024)
#define MAX_EVENTS 128

typedef struct {
    int fd;
    int idx;
    int closed;
    uint32_t ep_events;

    uint8_t *rx_buf;
    size_t rx_len;

    uint8_t *tx_buf;
    size_t tx_off;
    size_t tx_len;

    int credits;
    int recv_credit_acc;
    uint64_t last_recv_ns;
} client_t;

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int send_all_blocking(int fd, const uint8_t *buf, size_t len) {
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

static int connect_and_handshake(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    int rcvbuf = 2 * 1024 * 1024;
    int sndbuf = 1 * 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    struct timeval tv;
    tv.tv_sec = 5;
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
    if (send_all_blocking(fd, (const uint8_t *)req, (size_t)req_len) < 0) {
        close(fd);
        return -1;
    }

    char resp[2048];
    size_t used = 0;
    char *hdr_end = NULL;
    while (used < sizeof(resp) - 1) {
        ssize_t n = recv(fd, resp + used, sizeof(resp) - 1 - used, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
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

    if (set_nonblocking(fd) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static size_t build_masked_text_frame(uint8_t *out, size_t payload_size) {
    const uint8_t mask[4] = {0x37, 0xFA, 0x21, 0x3D};
    size_t hdr_len = 0;
    out[0] = 0x81; // FIN + Text
    if (payload_size < 126) {
        out[1] = (uint8_t)(0x80 | payload_size);
        hdr_len = 2;
    } else if (payload_size <= 65535) {
        out[1] = (uint8_t)(0x80 | 126);
        out[2] = (uint8_t)((payload_size >> 8) & 0xFF);
        out[3] = (uint8_t)(payload_size & 0xFF);
        hdr_len = 4;
    } else {
        out[1] = (uint8_t)(0x80 | 127);
        uint64_t plen64 = (uint64_t)payload_size;
        for (int i = 0; i < 8; i++) {
            out[2 + i] = (uint8_t)((plen64 >> (56 - i * 8)) & 0xFF);
        }
        hdr_len = 10;
    }
    memcpy(out + hdr_len, mask, 4);
    hdr_len += 4;
    for (size_t i = 0; i < payload_size; i++) {
        uint8_t ch = (uint8_t)('a' + (i % 26));
        out[hdr_len + i] = ch ^ mask[i & 3];
    }
    return hdr_len + payload_size;
}

static void update_epoll(int epfd, client_t *c, uint32_t want) {
    if (c->closed || c->ep_events == want) return;
    struct epoll_event ev;
    ev.events = want;
    ev.data.ptr = c;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, c->fd, &ev) == 0) {
        c->ep_events = want;
    }
}

static void queue_and_flush(
    int epfd,
    client_t *c,
    const uint8_t *frame,
    size_t frame_len,
    int recording,
    uint64_t *sent_msgs,
    uint64_t *err_count
) {
    if (c->closed) return;

    // Pack as many frames as credits & buffer space allow
    while (c->credits > 0 && (TX_BUF_CAP - c->tx_len) >= frame_len) {
        if (c->tx_off == c->tx_len) {
            c->tx_off = 0;
            c->tx_len = 0;
        }
        if ((TX_BUF_CAP - c->tx_len) < frame_len) break;
        memcpy(c->tx_buf + c->tx_len, frame, frame_len);
        c->tx_len += frame_len;
        c->credits--;
        if (recording) (*sent_msgs)++;
    }

    // Attempt non-blocking flush
    while (c->tx_off < c->tx_len) {
        ssize_t n = send(c->fd, c->tx_buf + c->tx_off, c->tx_len - c->tx_off, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) {
            c->tx_off += (size_t)n;
            if (c->tx_off == c->tx_len) {
                c->tx_off = 0;
                c->tx_len = 0;
                break;
            }
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            update_epoll(epfd, c, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
            return;
        } else {
            c->closed = 1;
            if (recording) (*err_count)++;
            return;
        }
    }

    // All queued bytes sent; only listen for readable
    if (c->tx_off == c->tx_len && (c->ep_events & EPOLLOUT)) {
        update_epoll(epfd, c, EPOLLIN | EPOLLRDHUP);
    }
}

static void drain_rx(
    int epfd,
    client_t *c,
    int inflight,
    int credit_divisor,
    int recording,
    uint64_t now,
    uint64_t *recv_msgs,
    uint64_t *recv_bytes,
    uint64_t *err_count
) {
    if (c->closed) return;

    for (;;) {
        size_t avail = RX_BUF_CAP - c->rx_len;
        if (avail == 0) {
            // Buffer full without complete frame -> protocol error
            c->closed = 1;
            if (recording) (*err_count)++;
            return;
        }
        ssize_t n = recv(c->fd, c->rx_buf + c->rx_len, avail, MSG_DONTWAIT);
        if (n > 0) {
            c->rx_len += (size_t)n;
            c->last_recv_ns = now;

            // Parse complete WebSocket frames in tight loop
            const uint8_t *buf = c->rx_buf;
            size_t len = c->rx_len;
            size_t pos = 0;
            uint64_t batch_msgs = 0;
            uint64_t batch_bytes = 0;

            while (pos + 2 <= len) {
                uint8_t b0 = buf[pos];
                uint8_t b1 = buf[pos + 1];

                // Fast path: unmasked single-frame text/binary with len < 126
                if (__builtin_expect((b0 == 0x81 || b0 == 0x82) && (b1 < 126), 1)) {
                    size_t f_len = 2 + (size_t)b1;
                    if (pos + f_len > len) break;
                    pos += f_len;
                    batch_msgs++;
                    batch_bytes += (uint64_t)b1;
                    continue;
                }

                uint8_t opcode = b0 & 0x0F;
                uint8_t masked = (b1 & 0x80) != 0;
                uint64_t plen = b1 & 0x7F;
                size_t hdr_len = 2;

                if (plen == 126) {
                    if (pos + 4 > len) break;
                    plen = ((uint64_t)buf[pos + 2] << 8) | (uint64_t)buf[pos + 3];
                    hdr_len = 4;
                } else if (plen == 127) {
                    if (pos + 10 > len) break;
                    plen = 0;
                    for (int k = 0; k < 8; k++) plen = (plen << 8) | buf[pos + 2 + k];
                    hdr_len = 10;
                }
                if (masked) hdr_len += 4;
                if (plen > (uint64_t)RX_BUF_CAP) {
                    c->closed = 1;
                    if (recording) (*err_count)++;
                    return;
                }
                if (pos + hdr_len + (size_t)plen > len) break;
                pos += hdr_len + (size_t)plen;

                if (opcode == 0x1 || opcode == 0x2 || opcode == 0x0) {
                    batch_msgs++;
                    batch_bytes += plen;
                } else if (opcode == 0x8) {
                    c->closed = 1;
                    if (recording) (*err_count)++;
                    return;
                }
                // Ping (0x9) / Pong (0xA) ignored
            }

            if (pos > 0) {
                size_t rem = len - pos;
                if (rem > 0) memmove(c->rx_buf, c->rx_buf + pos, rem);
                c->rx_len = rem;
            }

            if (batch_msgs > 0) {
                if (recording) {
                    *recv_msgs += batch_msgs;
                    *recv_bytes += batch_bytes;
                }
                c->recv_credit_acc += (int)batch_msgs;
                if (c->recv_credit_acc >= credit_divisor) {
                    int add = c->recv_credit_acc / credit_divisor;
                    c->recv_credit_acc %= credit_divisor;
                    c->credits += add;
                    if (c->credits > inflight) c->credits = inflight;
                }
            }
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            // Peer closed or error
            c->closed = 1;
            if (recording) (*err_count)++;
            (void)epfd;
            break;
        }
    }
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    int port = 18301;
    int num_clients = 32;
    double duration = 3.0;
    double warmup = 0.5;
    size_t payload_size = 32;
    int inflight = 16;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--clients") == 0 && i + 1 < argc) num_clients = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) duration = atof(argv[++i]);
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) warmup = atof(argv[++i]);
        else if ((strcmp(argv[i], "--payload") == 0 || strcmp(argv[i], "--payload-size") == 0) && i + 1 < argc) {
            payload_size = (size_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--inflight") == 0 && i + 1 < argc) inflight = atoi(argv[++i]);
    }

    if (num_clients < 1) num_clients = 1;
    if (inflight < 1) inflight = 1;
    if (payload_size > 32768) payload_size = 32768;

    uint8_t *tx_frame = malloc(payload_size + 16);
    if (!tx_frame) {
        fprintf(stderr, "OOM allocating tx_frame\n");
        return 1;
    }
    size_t tx_frame_len = build_masked_text_frame(tx_frame, payload_size);

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        perror("epoll_create1");
        free(tx_frame);
        return 1;
    }

    client_t *clients = calloc((size_t)num_clients, sizeof(client_t));
    if (!clients) {
        fprintf(stderr, "OOM allocating clients\n");
        close(epfd);
        free(tx_frame);
        return 1;
    }

    uint64_t err_count = 0;
    uint64_t t_init = now_ns();
    for (int i = 0; i < num_clients; i++) {
        clients[i].idx = i;
        clients[i].fd = connect_and_handshake(host, port);
        clients[i].rx_buf = malloc(RX_BUF_CAP);
        clients[i].tx_buf = malloc(TX_BUF_CAP);
        clients[i].credits = inflight;
        clients[i].last_recv_ns = t_init;
        if (clients[i].fd < 0 || !clients[i].rx_buf || !clients[i].tx_buf) {
            clients[i].closed = 1;
            err_count++;
            continue;
        }
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.ptr = &clients[i];
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, clients[i].fd, &ev) < 0) {
            close(clients[i].fd);
            clients[i].fd = -1;
            clients[i].closed = 1;
            err_count++;
            continue;
        }
        clients[i].ep_events = ev.events;
    }

    // Give server a brief moment to complete open()/subscribe() for all clients
    usleep(20000);

    int credit_divisor = (num_clients > 1) ? (num_clients - 1) : 1;

    uint64_t t_start = now_ns();
    uint64_t t_warmup_end = t_start + (uint64_t)(warmup * 1e9);
    uint64_t t_end = t_warmup_end + (uint64_t)(duration * 1e9);
    uint64_t t_record_start = 0;

    uint64_t recv_msgs = 0;
    uint64_t recv_bytes = 0;
    uint64_t sent_msgs = 0;

    // Kick off initial inflight sends for all connected clients
    for (int i = 0; i < num_clients; i++) {
        if (!clients[i].closed) {
            queue_and_flush(epfd, &clients[i], tx_frame, tx_frame_len, 0, &sent_msgs, &err_count);
        }
    }

    struct epoll_event events[MAX_EVENTS];
    uint64_t loop_iters = 0;

    for (;;) {
        uint64_t now = now_ns();
        if (now >= t_end) break;

        int recording = (now >= t_warmup_end);
        if (recording && t_record_start == 0) {
            t_record_start = now;
        }

        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 5);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            err_count++;
            break;
        }

        now = now_ns();
        recording = (now >= t_warmup_end && now < t_end);
        if (recording && t_record_start == 0) {
            t_record_start = now;
        }

        for (int i = 0; i < nfds; i++) {
            client_t *c = (client_t *)events[i].data.ptr;
            uint32_t ev = events[i].events;

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                // Drain any remaining buffered bytes before marking closed
                drain_rx(epfd, c, inflight, credit_divisor, recording, now, &recv_msgs, &recv_bytes, &err_count);
                if (!c->closed) {
                    c->closed = 1;
                    if (recording) err_count++;
                }
                epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
                continue;
            }

            if (ev & EPOLLIN) {
                drain_rx(epfd, c, inflight, credit_divisor, recording, now, &recv_msgs, &recv_bytes, &err_count);
            }
            if (!c->closed && ((ev & EPOLLOUT) || c->credits > 0)) {
                queue_and_flush(epfd, c, tx_frame, tx_frame_len, recording, &sent_msgs, &err_count);
            }
            if (c->closed && c->fd >= 0) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
            }
        }

        // Periodic stall-guard (every 64 iterations): if any active client has 0 credits
        // and no rx for >50ms, grant 1 credit so pipeline cannot stall.
        if ((++loop_iters & 63) == 0) {
            int alive = 0;
            for (int i = 0; i < num_clients; i++) {
                if (clients[i].closed) continue;
                alive++;
                if (clients[i].credits == 0 && (now - clients[i].last_recv_ns) > 50000000ULL) {
                    clients[i].credits = 1;
                    queue_and_flush(epfd, &clients[i], tx_frame, tx_frame_len, recording, &sent_msgs, &err_count);
                }
            }
            if (alive == 0) break;
        }
    }

    uint64_t t_finish = now_ns();
    double actual_sec = (t_record_start > 0) ? (double)(t_finish - t_record_start) / 1e9 : duration;
    if (actual_sec <= 0.01) actual_sec = duration;

    for (int i = 0; i < num_clients; i++) {
        if (clients[i].fd >= 0) close(clients[i].fd);
        free(clients[i].rx_buf);
        free(clients[i].tx_buf);
    }
    free(clients);
    close(epfd);
    free(tx_frame);

    double msgs_per_sec = (double)recv_msgs / actual_sec;
    double mb_per_sec = ((double)recv_bytes / (1024.0 * 1024.0)) / actual_sec;

    printf(
        "{\"msgs_per_sec\":%.2f,\"mb_per_sec\":%.2f,\"recv_msgs\":%llu,\"sent_msgs\":%llu,"
        "\"err\":%llu,\"clients\":%d,\"payload\":%zu,\"inflight\":%d,\"duration_sec\":%.3f}\n",
        msgs_per_sec,
        mb_per_sec,
        (unsigned long long)recv_msgs,
        (unsigned long long)sent_msgs,
        (unsigned long long)err_count,
        num_clients,
        payload_size,
        inflight,
        actual_sec
    );
    return 0;
}
