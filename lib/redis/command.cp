// lib/redis/command.cp
// Redis / Valkey Command Builders & Pipelined Batch Buffer
// Absorbed from Bun src/valkey/valkey_protocol.cp

import { RESPEncoder } from "./resp.cp";

pub struct RedisCommand {
    /// Encodes a 1-token Redis command (e.g. `PING`) into `out`.
    pub static fn encodeCommand1(out: []u8, cmd: []const u8): usize {
        let pos = RESPEncoder.encodeArrayHeader(out, 1);
        if (pos == 0) { return 0; }
        const n1 = RESPEncoder.encodeBulkString(out[pos..], cmd);
        if (n1 == 0) { return 0; }
        return pos + n1;
    }

    /// Encodes a 4-token Redis command (e.g. `HSET key field val`) into `out`.
    pub static fn encodeCommand4(out: []u8, cmd: []const u8, arg1: []const u8, arg2: []const u8, arg3: []const u8): usize {
        let pos = RESPEncoder.encodeArrayHeader(out, 4);
        if (pos == 0) { return 0; }
        const n1 = RESPEncoder.encodeBulkString(out[pos..], cmd);
        if (n1 == 0) { return 0; }
        pos += n1;
        const n2 = RESPEncoder.encodeBulkString(out[pos..], arg1);
        if (n2 == 0) { return 0; }
        pos += n2;
        const n3 = RESPEncoder.encodeBulkString(out[pos..], arg2);
        if (n3 == 0) { return 0; }
        pos += n3;
        const n4 = RESPEncoder.encodeBulkString(out[pos..], arg3);
        if (n4 == 0) { return 0; }
        return pos + n4;
    }

    pub static fn encodePing(out: []u8): usize {
        return RedisCommand.encodeCommand1(out, "PING");
    }

    pub static fn encodeAuth(out: []u8, password: []const u8): usize {
        return RESPEncoder.encodeCommand2(out, "AUTH", password);
    }

    pub static fn encodeGet(out: []u8, key: []const u8): usize {
        return RESPEncoder.encodeCommand2(out, "GET", key);
    }

    pub static fn encodeSet(out: []u8, key: []const u8, val: []const u8): usize {
        return RESPEncoder.encodeCommand3(out, "SET", key, val);
    }

    pub static fn encodeDel(out: []u8, key: []const u8): usize {
        return RESPEncoder.encodeCommand2(out, "DEL", key);
    }

    pub static fn encodeExpire(out: []u8, key: []const u8, seconds: usize): usize {
        let sec_buf: [24]u8 = [0; 24];
        const sec_len = RESPEncoder.writeUint(sec_buf[0..24], seconds);
        if (sec_len == 0) { return 0; }
        return RESPEncoder.encodeCommand3(out, "EXPIRE", key, sec_buf[0..sec_len]);
    }

    pub static fn encodeHSet(out: []u8, key: []const u8, field: []const u8, val: []const u8): usize {
        return RedisCommand.encodeCommand4(out, "HSET", key, field, val);
    }

    pub static fn encodeHGet(out: []u8, key: []const u8, field: []const u8): usize {
        return RESPEncoder.encodeCommand3(out, "HGET", key, field);
    }

    pub static fn encodeLPush(out: []u8, key: []const u8, val: []const u8): usize {
        return RESPEncoder.encodeCommand3(out, "LPUSH", key, val);
    }

    pub static fn encodeRPop(out: []u8, key: []const u8): usize {
        return RESPEncoder.encodeCommand2(out, "RPOP", key);
    }
}

pub struct RedisPipeline {
    pub buf: [4096]u8 = undefined;
    pub len: usize = 0;
    pub command_count: usize = 0;

    pub static fn init(): RedisPipeline {
        return RedisPipeline {
            buf: undefined,
            len: 0,
            command_count: 0,
        };
    }

    pub fn reset(*_) {
        _.len = 0;
        _.command_count = 0;
    }

    pub fn bytes(*_): []const u8 {
        return _.buf[0.._.len];
    }

    pub fn ping(*_): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodePing(_.buf[_.len..4096]);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn auth(*_, password: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeAuth(_.buf[_.len..4096], password);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn get(*_, key: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeGet(_.buf[_.len..4096], key);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn set(*_, key: []const u8, val: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeSet(_.buf[_.len..4096], key, val);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn del(*_, key: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeDel(_.buf[_.len..4096], key);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn expire(*_, key: []const u8, seconds: usize): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeExpire(_.buf[_.len..4096], key, seconds);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn hset(*_, key: []const u8, field: []const u8, val: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeHSet(_.buf[_.len..4096], key, field, val);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn hget(*_, key: []const u8, field: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeHGet(_.buf[_.len..4096], key, field);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn lpush(*_, key: []const u8, val: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeLPush(_.buf[_.len..4096], key, val);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }

    pub fn rpop(*_, key: []const u8): bool {
        if (_.len >= 4096) { return false; }
        const n = RedisCommand.encodeRPop(_.buf[_.len..4096], key);
        if (n == 0) { return false; }
        _.len += n;
        _.command_count += 1;
        return true;
    }
}
