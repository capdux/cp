// Source: Absorbed from _ref/playwright/packages/playwright-core/src/remote/playwrightServer.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/remote.cp — Playwright Remote WS RPC Server & Client
// Dual-mode architecture:
//   1. In-process zero-copy direct calls (0 IPC overhead for CP callers)
//   2. Optional native SO_REUSEPORT epoll WS RPC server (cap:web WsServer)
//      allowing Python / Java / .NET / remote nodes to drive CP browsers.
// ============================================================================

const std = @import("std");
import { WsServer, WsServerConfig } from "cap:web";
import { JSON, JsonValue, JsonField } from "cap:json";
import { CdpFrameScanner, CdpSendTransportFn } from "cap:cdp";
import { PlaywrightError } from "./network";
import { Browser } from "./browser";
import { BrowserContextOptions } from "./context";
import { ChromiumBrowserType, LaunchOptions } from "./chromium";

pub enum RemoteMode {
    Default,
    LaunchServer,
    LaunchServerShared,
    Extension,
}

pub struct PlaywrightRemoteConfig {
    pub host: []const u8 = "127.0.0.1",
    pub port: u16 = 9234,
    pub path: []const u8 = "/playwright",
    pub max_connections: u32 = 32,
    pub workers: usize = 1,
    pub pin_cpu: bool = false,
    pub mode: RemoteMode = RemoteMode.LaunchServer,
}

pub struct PlaywrightRemoteServer {
    allocator: std.mem.Allocator = undefined,
    pub config: PlaywrightRemoteConfig = .{},
    ws_server: WsServer = undefined,
    pub listening: bool = false,
    pub active_connections: u32 = 0,
    pub browser: *Browser? = null,
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,

    pub static fn init(alloc: std.mem.Allocator, config: PlaywrightRemoteConfig): PlaywrightRemoteServer {
        return PlaywrightRemoteServer{
            allocator: alloc,
            config: config,
            ws_server: WsServer.init(WsServerConfig{
                port: config.port,
                workers: config.workers,
                backlog: 1024,
                pin_cpu: config.pin_cpu,
                broadcast_mode: false,
            }),
            listening: false,
            active_connections: 0,
            browser: null,
            transport_fn: null,
            transport_ctx: 0,
        };
    }

    pub fn deinit(*_) {
        _.stop();
        if (b := _.browser) {
            b.deinit();
            _.allocator.destroy(b);
            _.browser = null;
        }
    }

    ~init() {
        _.deinit();
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
    }

    pub fn wsEndpoint(out_buf: []u8): ([]const u8)!PlaywrightError {
        return std.fmt.bufPrint(
            out_buf,
            "ws://{s}:{d}{s}",
            .{ _.config.host, _.config.port, _.config.path },
        ) catch return PlaywrightError.BufferTooSmall;
    }

    pub fn start(*_): void!PlaywrightError {
        if (_.listening) return;
        const ok = _.ws_server.start();
        if (!ok) return PlaywrightError.ProtocolError;
        _.listening = true;
    }

    pub fn stop(*_) {
        if (!_.listening) return;
        _.ws_server.stop();
        _.listening = false;
    }

    pub fn isListening(): bool {
        return _.listening;
    }

    /// Dispatch a Playwright RPC JSON frame (e.g. from WS RPC client) to the in-process
    /// Browser/Page engine and return a serialized JSON-RPC response via cap:json.
    pub fn handleRpcMessage(*_, alloc: std.mem.Allocator, msg_json: []const u8): ([]u8)!PlaywrightError {
        const id_str = CdpFrameScanner.extractTopLevelField(msg_json, "id", false) orelse "1";
        const method = CdpFrameScanner.extractTopLevelField(msg_json, "method", true) orelse return PlaywrightError.ProtocolError;

        let status_str: []const u8 = "ok";
        let detail_str: []const u8 = "";

        if (std.mem.eql(u8, method, "initialize")) {
            _.active_connections += 1;
            detail_str = "playwright-cp/1.66.0-next";
        } else if (std.mem.eql(u8, method, "launch")) {
            if (_.browser == null) {
                let bp = _.allocator.create(Browser) catch return PlaywrightError.OutOfMemory;
                bp.* = ChromiumBrowserType.launch(_.allocator, LaunchOptions{
                    headless: true,
                    transport_fn: _.transport_fn,
                    transport_ctx: _.transport_ctx,
                }) catch |e| {
                    _.allocator.destroy(bp);
                    return e;
                };
                _.browser = bp;
            }
            detail_str = "chromium-launched";
        } else if (std.mem.eql(u8, method, "newPage")) {
            if (_.browser == null) {
                let bp = _.allocator.create(Browser) catch return PlaywrightError.OutOfMemory;
                bp.* = ChromiumBrowserType.launch(_.allocator, LaunchOptions{
                    headless: true,
                    transport_fn: _.transport_fn,
                    transport_ctx: _.transport_ctx,
                }) catch |e| {
                    _.allocator.destroy(bp);
                    return e;
                };
                _.browser = bp;
            }
            if (b := _.browser) {
                const page = b.newPage()?;
                detail_str = page.session_id;
            }
        } else if (std.mem.eql(u8, method, "goto")) {
            const url = CdpFrameScanner.extractTopLevelField(msg_json, "url", true) orelse "about:blank";
            if (b := _.browser) {
                const ctxs = b.contexts();
                if (ctxs.len > 0 && ctxs[0].pages().len > 0) {
                    const resp = ctxs[0].pages()[0].goto(url, .{})?;
                    detail_str = resp.url;
                } else {
                    const page = b.newPage()?;
                    const resp = page.goto(url, .{})?;
                    detail_str = resp.url;
                }
            } else {
                return PlaywrightError.NotConnected;
            }
        } else if (std.mem.eql(u8, method, "close")) {
            if (b := _.browser) {
                b.close();
            }
            if (_.active_connections > 0) _.active_connections -= 1;
            detail_str = "closed";
        } else if (std.mem.eql(u8, method, "ping")) {
            detail_str = "pong";
        } else {
            status_str = "unknown_method";
            detail_str = method;
        }

        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        let res_fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
        res_fields[0] = JsonField{ key: "status", value: JsonValue{ tag: .String, str_val: status_str } };
        res_fields[1] = JsonField{ key: "method", value: JsonValue{ tag: .String, str_val: method } };
        res_fields[2] = JsonField{ key: "detail", value: JsonValue{ tag: .String, str_val: detail_str } };

        let root_fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
        root_fields[0] = JsonField{ key: "id", value: JsonValue{ tag: .String, str_val: id_str } };
        root_fields[1] = JsonField{ key: "result", value: JsonValue{ tag: .Object, obj_val: res_fields } };

        return JSON.stringifyValue(alloc, JsonValue{ tag: .Object, obj_val: root_fields }) catch return PlaywrightError.OutOfMemory;
    }
}

pub struct PlaywrightRemoteClient {
    allocator: std.mem.Allocator = undefined,
    pub ws_endpoint: []const u8 = "",
    pub connected: bool = false,
    pub next_rpc_id: u64 = 1,
    owned_endpoint: []u8 = &.{},

    pub static fn connect(alloc: std.mem.Allocator, ws_endpoint: []const u8): PlaywrightRemoteClient!PlaywrightError {
        if (!std.mem.startsWith(u8, ws_endpoint, "ws://") && !std.mem.startsWith(u8, ws_endpoint, "wss://")) {
            return PlaywrightError.InvalidUrl;
        }
        const duped = alloc.dupe(u8, ws_endpoint) catch return PlaywrightError.OutOfMemory;
        return PlaywrightRemoteClient{
            allocator: alloc,
            ws_endpoint: duped,
            connected: true,
            next_rpc_id: 1,
            owned_endpoint: duped,
        };
    }

    pub fn deinit(*_) {
        _.close();
        if (_.owned_endpoint.len > 0) {
            _.allocator.free(_.owned_endpoint);
            _.owned_endpoint = &.{};
            _.ws_endpoint = "";
        }
    }

    ~init() {
        _.deinit();
    }

    pub fn buildRpcFrame(*_, method: []const u8, url_param: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        if (!_.connected) return PlaywrightError.NotConnected;
        const id = _.next_rpc_id;
        _.next_rpc_id += 1;
        if (url_param.len > 0) {
            return std.fmt.bufPrint(
                out_buf,
                "{{\"id\":{d},\"method\":\"{s}\",\"url\":\"{s}\"}}",
                .{ id, method, url_param },
            ) catch return PlaywrightError.BufferTooSmall;
        }
        return std.fmt.bufPrint(
            out_buf,
            "{{\"id\":{d},\"method\":\"{s}\"}}",
            .{ id, method },
        ) catch return PlaywrightError.BufferTooSmall;
    }

    pub fn close(*_) {
        _.connected = false;
    }
}
