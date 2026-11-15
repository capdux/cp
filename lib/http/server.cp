/**
 * CP HTTP Server & Client (cap:http)
 */

import { SystemAction } from "cap:beam";
import { Request } from "./request";
import { Response } from "./response";

pub class HttpServer {
    pub port: i32;
    pub fd: i32;

    pub fn init(*_, port: i32, fd: i32) {
        _.port = port;
        _.fd = fd;
    }

    pub fn stop(*_) {
        @net_close(_.fd);
    }
}

/**
 * 生产级 fetch 实现
 */
pub async fn fetch(url: string, options: *!?): Response {
    let raw_body = "";

    if (@is_nest) {
        // --- BEAM 模式：外包给 Bun I/O Gateway ---
        raw_body = await @yield_to_beam(@intFromEnum(SystemAction.HTTP_FETCH), url);
    } else {
        // --- Native 模式：调用本地 Zig HTTP Client ---
        raw_body = await @http_fetch(url, options);
    }

    let res = new Response(raw_body);
    return res;
}

/**
 * 快速 JSON 响应构造函数 (对标 Bun 的 Response.json)
 */
pub fn json(data: auto): Response {
    let raw_json = JSON.stringify(data);
    let res = new Response(raw_json);
    res.contentType = "application/json";
    return res;
}

/**
 * 极速 HTTP Server (对标 Bun.serve)
 */
pub fn serve(config: auto): HttpServer {
    zig {
        const Server = HttpServer;
    }
    if (@is_nest) {
        @yield_to_beam(@intFromEnum(SystemAction.HTTP_SERVE), config);
        let s = new HttpServer(3000, 0);
        return s;
    } else {
        return @http_serve(config);
    }
}
