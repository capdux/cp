// Live Real-Socket E2E Test for `cap:web` Multi-Core SO_REUSEPORT + epoll + 32B SIMD WebSocket Server
import { WsServer, WsServerConfig, computeWebSocketAccept, simdMaskXorInPlace } from "../../lib/web/wsServer.cp";

@c struct WsClientSockAddrIn {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [8]u8,
}

extern fn connect(fd: i32, addr: *const WsClientSockAddrIn, addrlen: u32): i32;

fn htons16(v: u16): u16 {
    return ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsStr(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn main() {
    // 1. Verify RFC 6455 official Sec-WebSocket-Accept test vector
    const accept = computeWebSocketAccept("dGhlIHNhbXBsZSBub25jZQ==");
    if (!strEq(accept[0..28], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=")) {
        @panic("RFC 6455 Sec-WebSocket-Accept vector mismatch");
    }

    // 2. Verify 32-byte SIMD mask XOR roundtrip (73 bytes: 2x32B SIMD + 9B tail)
    let simd_buf: [73]u8 = [0; 73];
    let si: usize = 0;
    while (si < 73) : (si += 1) {
        simd_buf[si] = ((si * 7 + 3) & 0xFF) as u8;
    }
    const test_mask: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];
    simdMaskXorInPlace(simd_buf[0..], test_mask);
    simdMaskXorInPlace(simd_buf[0..], test_mask);
    si = 0;
    while (si < 73) : (si += 1) {
        if (simd_buf[si] != (((si * 7 + 3) & 0xFF) as u8)) {
            @panic("32B SIMD mask XOR roundtrip mismatch");
        }
    }

    // 3. Start 2-worker SO_REUSEPORT epoll WsServer
    const port: u16 = 18924;
    let srv = WsServer.init(WsServerConfig{ port: port, workers: 2, backlog: 128 });
    if (!srv.start()) {
        @panic("Failed to start WsServer on port 18924");
    }

    const cfd = socket(2, 1, 0);
    if (cfd < 0) {
        srv.stop();
        @panic("client socket failed");
    }
    let addr = WsClientSockAddrIn{
        sin_family: 2,
        sin_port: htons16(port),
        sin_addr: 0x0100007F, // 127.0.0.1
        sin_zero: [0; 8],
    };
    if (connect(cfd, *addr, 16) < 0) {
        * = close(cfd);
        srv.stop();
        @panic("client connect failed");
    }

    let rbuf: [2048]u8 = [0; 2048];

    // 4. RFC 6455 Upgrade Handshake
    const req = "GET /ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
    * = write(cfd, req.ptr, req.len);
    const n1 = read(cfd, &rbuf[0], 2048);
    if (n1 <= 0
        || !containsStr(rbuf[0..(n1 as usize)], "101 Switching Protocols")
        || !containsStr(rbuf[0..(n1 as usize)], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="))
    {
        * = close(cfd);
        srv.stop();
        @panic("WebSocket handshake failed");
    }

    // 5. Send a 72-byte Masked Text Frame (exercises 2x32B SIMD + 8B tail on server)
    const msg = "Hello-CP-WebSocket-32Byte-SIMD-Mask-Echo-Payload-0123456789-ABCDEFGHIJKL";
    const mask: [4]u8 = [0x12, 0x34, 0x56, 0x78];
    let frame: [128]u8 = [0; 128];
    frame[0] = 0x81; // FIN + Text
    frame[1] = (0x80 | (msg.len as u8)) as u8; // MASKED + len (72)
    frame[2] = mask[0];
    frame[3] = mask[1];
    frame[4] = mask[2];
    frame[5] = mask[3];
    let mi: usize = 0;
    while (mi < msg.len) : (mi += 1) {
        frame[6 + mi] = msg[mi] ^ mask[mi & 3];
    }
    * = write(cfd, frame[0..].ptr, 6 + msg.len);

    // Read Echo frame
    const n2 = read(cfd, &rbuf[0], 2048);
    if (n2 != ((2 + msg.len) as isize) || rbuf[0] != 0x81 || rbuf[1] != (msg.len as u8) || !strEq(rbuf[2 .. 2 + msg.len], msg)) {
        * = close(cfd);
        srv.stop();
        @panic("WebSocket Masked Text Echo mismatch");
    }

    // 6. Send a Masked Ping frame and verify Pong reply
    let ping_frame: [10]u8 = [0x89, 0x84, 0xAA, 0xBB, 0xCC, 0xDD, 'p' ^ 0xAA, 'i' ^ 0xBB, 'n' ^ 0xCC, 'g' ^ 0xDD];
    * = write(cfd, ping_frame[0..].ptr, 10);
    const n3 = read(cfd, &rbuf[0], 2048);
    if (n3 != 6 || rbuf[0] != 0x8A || rbuf[1] != 4 || !strEq(rbuf[2..6], "ping")) {
        * = close(cfd);
        srv.stop();
        @panic("WebSocket Ping->Pong failed");
    }

    // 7. Send a Masked Close frame (code 1000 = 0x03E8) and verify Close echo
    let close_frame: [8]u8 = [0x88, 0x82, 0x11, 0x22, 0x33, 0x44, 0x03 ^ 0x11, 0xE8 ^ 0x22];
    * = write(cfd, close_frame[0..].ptr, 8);
    const n4 = read(cfd, &rbuf[0], 2048);
    if (n4 != 4 || rbuf[0] != 0x88 || rbuf[1] != 2 || rbuf[2] != 0x03 || rbuf[3] != 0xE8) {
        * = close(cfd);
        srv.stop();
        @panic("WebSocket Close echo failed");
    }

    * = close(cfd);
    srv.stop();
    @print("PASS testWebSocketServerLive (2-worker SO_REUSEPORT epoll + 32B SIMD + RFC 6455 Handshake/Echo/Ping/Close)");
}
