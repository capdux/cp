// Source: Custom implementation by Antigravity AI
/**
 * CP Buffer Limit Guard (cap:io)
 * 缓冲区最大字节限制守卫 (Ported from Bun.js MaxBuf)
 * 防止未受信进程或超大流耗尽系统内存
 */

import { IOError } from "./traits";

pub enum MaxBufKind {
    Stdout,
    Stderr,
    Custom,
}

pub class MaxBuf {
    remaining_bytes: i64;
    initial_bytes: i64;
    kind: MaxBufKind;

    pub fn init(limit: i64) {
        _.remaining_bytes = limit;
        _.initial_bytes = limit;
        _.kind = MaxBufKind.Custom;
    }

    pub static fn withKind(limit: i64, kind: MaxBufKind): MaxBuf {
        let mb = new MaxBuf();
        mb.remaining_bytes = limit;
        mb.initial_bytes = limit;
        mb.kind = kind;
        return mb;
    }

    pub fn onReadBytes(*_, bytes: usize): bool! {
        if (_.remaining_bytes < 0) {
            return false;
        }
        _.remaining_bytes = _.remaining_bytes - (bytes as i64);
        if (_.remaining_bytes < 0) {
            return err.BufferFull;
        }
        return true;
    }

    pub fn remaining(): i64 {
        return _.remaining_bytes;
    }

    pub fn reset(*_) {
        _.remaining_bytes = _.initial_bytes;
    }

    pub fn isExceeded(): bool {
        return _.remaining_bytes < 0;
    }
}
