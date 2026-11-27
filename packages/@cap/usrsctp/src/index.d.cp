// Type definitions for CP

export fn cp_usrsctp_init(udp_port: u16): i32;
export fn cp_usrsctp_finish(): i32;
export fn cp_usrsctp_socket_create(domain: i32, sock_type: i32): *auto!?;
export fn cp_usrsctp_socket_close(sock: *auto!?);
export fn cp_usrsctp_socket_create_and_close(): i32;
export error SctpError { InitFailed, FinishFailed, SocketCreateFailed }

export fn sctpInit(udp_port: u16): !void;
export fn sctpFinish(): !void;
export fn socketCreateAndClose(): !void;
export class SctpSocket {
    raw: *auto!?,
    fn createStream(): !SctpSocket;
    fn isOpen(): bool;
    fn close(_: auto);
    fn __cp_deinit(_: &mut SctpSocket);
}

