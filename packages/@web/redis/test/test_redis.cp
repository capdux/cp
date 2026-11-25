// Unit tests for @web/redis
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    RedisPluginConfig,
    RedisPingFn,
    redisBuildKey,
    redisEncodeRespCommand,
    registerRedis,
    registerRedisWithPing,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn pingDown(cfg: *const RedisPluginConfig): bool {
    * = cfg;
    return false;
}

fn stateCheckHandler(ctx: *Context): Response {
    const host = ctx.stateVal("redis_host") orelse "";
    const prefix = ctx.stateVal("redis_prefix") orelse "";
    if (strEql(host, "10.0.0.8") && strEql(prefix, "prod:")) {
        return Response.json(200, "{\"ok\":true}");
    }
    return Response.json(500, "{\"ok\":false}");
}

pub fn main() {
    // 1. redisBuildKey valid namespaces
    let kbuf: [128]u8 = [0; 128];
    const k1_len = redisBuildKey("cp:", "cache", "user:42", kbuf[0..]) orelse 0;
    assertTrue(strEql(kbuf[0..k1_len], "cp:cache:user:42"), "cache key built");

    const k2_len = redisBuildKey("cp:", "sess:", "sid99", kbuf[0..]) orelse 0;
    assertTrue(strEql(kbuf[0..k2_len], "cp:sess:sid99"), "sess key avoids double colon");

    const k3_len = redisBuildKey("app", "rl", "127.0.0.1", kbuf[0..]) orelse 0;
    assertTrue(strEql(kbuf[0..k3_len], "app:rl:127.0.0.1"), "prefix without trailing colon normalized");

    // 2. redisBuildKey rejects invalid keys (CRLF, space, empty, overflow)
    assertTrue(redisBuildKey("cp:", "cache", "", kbuf[0..]) == null, "empty key rejected");
    assertTrue(redisBuildKey("cp:", "cache", "bad key", kbuf[0..]) == null, "space in key rejected");
    assertTrue(redisBuildKey("cp:", "cache", "bad\r\nSET x 1", kbuf[0..]) == null, "CRLF injection rejected");
    assertTrue(redisBuildKey("cp:", "bad ns", "k", kbuf[0..]) == null, "space in namespace rejected");
    let tiny: [4]u8 = [0; 4];
    assertTrue(redisBuildKey("cp:", "cache", "user:42", tiny[0..]) == null, "buffer overflow rejected");

    // 3. redisEncodeRespCommand RESP2/3 encoding
    let rbuf: [256]u8 = [0; 256];
    const set_args: [3]string = ["SET", "cp:cache:k1", "hello"];
    const rlen = redisEncodeRespCommand(set_args[0..], rbuf[0..]) orelse 0;
    assertTrue(
        strEql(rbuf[0..rlen], "*3\r\n$3\r\nSET\r\n$11\r\ncp:cache:k1\r\n$5\r\nhello\r\n"),
        "RESP SET command encoded accurately",
    );

    const ping_args: [1]string = ["PING"];
    const plen = redisEncodeRespCommand(ping_args[0..], rbuf[0..]) orelse 0;
    assertTrue(strEql(rbuf[0..plen], "*1\r\n$4\r\nPING\r\n"), "RESP PING command encoded");

    assertTrue(redisEncodeRespCommand(ping_args[0..0], rbuf[0..]) == null, "empty RESP args rejected");

    // 4. registerRedis injects state and serves 200 pong
    const cfg = RedisPluginConfig{
        host: "10.0.0.8",
        port: 6380,
        key_prefix: "prod:",
        ping_path: "/_redis/ping",
    };
    let app = App.init();
    * = registerRedis(*app, cfg);
    * = app.get("/state", stateCheckHandler);

    const r_ping = app.dispatch(Request.init(AppMethod.GET, "/_redis/ping"));
    assertTrue(r_ping.status == 200, "default redis ping returns 200");
    assertTrue(strEql(r_ping.body, "{\"redis\":\"pong\"}"), "default redis ping body");

    const r_state = app.dispatch(Request.init(AppMethod.GET, "/state"));
    assertTrue(r_state.status == 200, "redis_host and redis_prefix injected into app state");

    // 5. registerRedisWithPing custom down callback -> 503
    let app2 = App.init();
    * = registerRedisWithPing(*app2, cfg, pingDown);
    const r_down = app2.dispatch(Request.init(AppMethod.GET, "/_redis/ping"));
    assertTrue(r_down.status == 503, "down redis ping returns 503");
    assertTrue(strEql(r_down.body, "{\"redis\":\"down\"}"), "down redis ping body");

    std.debug.print("PASS: @web/redis all tests passed!\n", .{});
}
