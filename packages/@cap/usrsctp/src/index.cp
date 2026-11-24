// @cap/usrsctp — Portable Userland SCTP Stack (0.9.5.0) for WebRTC DataChannel in CP
// License: BSD-3-Clause (Upstream: sctplab/usrsctp)

pub @c {
    fn cp_usrsctp_init(udp_port: u16): i32;
    fn cp_usrsctp_finish(): i32;
    fn cp_usrsctp_socket_create(domain: i32, sock_type: i32): *!?;
    fn cp_usrsctp_socket_close(sock: *!?);
    fn cp_usrsctp_socket_create_and_close(): i32;
}

pub err SctpError {
    InitFailed,
    FinishFailed,
    SocketCreateFailed,
}

/// Initializes the global userland SCTP stack (`udp_port = 0` disables UDP encapsulation for WebRTC AF_CONN).
pub fn sctpInit(udp_port: u16): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_usrsctp_init(udp_port);
    }
    if (rc != 0) return SctpError.InitFailed;
}

/// Shuts down the global userland SCTP stack and waits for internal timers to stop.
pub fn sctpFinish(): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_usrsctp_finish();
    }
    if (rc != 0) return SctpError.FinishFailed;
}

/// Verifies userland SCTP socket creation (`AF_CONN` / `SOCK_STREAM` / `IPPROTO_SCTP`) and clean teardown.
pub fn socketCreateAndClose(): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_usrsctp_socket_create_and_close();
    }
    if (rc != 0) return SctpError.SocketCreateFailed;
}

/// Owned RAII wrapper around a userland `usrsctp` socket (`struct socket *`).
/// Automatically closes the socket on scope exit via `~init()`.
pub struct SctpSocket {
    raw: *!? = null,

    /// Creates a new userland SCTP stream socket (`AF_CONN`, `SOCK_STREAM`, `IPPROTO_SCTP`).
    pub static fn createStream(): SctpSocket! {
        let handle: *!? = null;
        unsafe {
            handle = cp_usrsctp_socket_create(0, 0);
        }
        if (handle == null) {
            return SctpError.SocketCreateFailed;
        }
        return SctpSocket{ .raw = handle };
    }

    /// Returns true if the underlying SCTP socket handle is open.
    pub fn isOpen(): bool {
        return _.raw != null;
    }

    /// Explicitly closes the underlying SCTP socket handle (idempotent with `~init()`).
    pub fn close(*_) {
        if (p := _.raw) {
            _.raw = null;
            unsafe {
                cp_usrsctp_socket_close(p);
            }
        }
    }

    ~init() {
        _.close();
    }
}
