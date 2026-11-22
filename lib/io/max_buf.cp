// Source: Absorbed from /data/dev/bun/src/io/MaxBuf.cp | Synced: 2026-11-22T10:27:34Z
/**
 * CP Buffer Limit Guard (cap:io)
 * 缓冲区最大字节限制守卫 (Ported from Bun.js MaxBuf)
 * 防止未受信进程或超大流耗尽系统内存
 */

import { IOError } from "./traits";
import { IoStatus } from "./source";

pub enum MaxBufKind {
    Stdout,
    Stderr,
    Custom,
}

pub class MaxBuf {
    remaining_bytes: i64;
    initial_bytes: i64;
    kind: MaxBufKind;
    is_unlimited: bool;

    pub fn init(limit: i64) {
        _.remaining_bytes = limit;
        _.initial_bytes = limit;
        _.kind = MaxBufKind.Custom;
        _.is_unlimited = false;
    }

    pub static fn withKind(limit: i64, kind: MaxBufKind): MaxBuf {
        let mb = new MaxBuf(limit);
        mb.kind = kind;
        return mb;
    }

    pub static fn unlimited(): MaxBuf {
        let mb = new MaxBuf(0);
        mb.is_unlimited = true;
        return mb;
    }

    pub fn onReadBytes(*_, bytes: usize): bool! {
        if (_.is_unlimited) {
            return true;
        }
        if (_.remaining_bytes < 0) {
            return false;
        }
        _.remaining_bytes = _.remaining_bytes - (bytes as i64);
        if (_.remaining_bytes < 0) {
            return err.BufferFull;
        }
        return true;
    }

    pub fn onReadBytesStatus(*_, bytes: usize): IoStatus {
        if (_.is_unlimited) {
            return IoStatus.Ok;
        }
        if (_.remaining_bytes < 0) {
            return IoStatus.BufferFull;
        }
        _.remaining_bytes = _.remaining_bytes - (bytes as i64);
        if (_.remaining_bytes < 0) {
            return IoStatus.BufferFull;
        }
        return IoStatus.Ok;
    }

    pub fn remaining(): i64 {
        return _.remaining_bytes;
    }

    pub fn getLimit(): i64 {
        return _.initial_bytes;
    }

    pub fn consumed(): i64 {
        if (_.is_unlimited) return 0;
        return _.initial_bytes - _.remaining_bytes;
    }

    pub fn reset(*_) {
        _.remaining_bytes = _.initial_bytes;
    }

    pub fn isExceeded(): bool {
        if (_.is_unlimited) return false;
        return _.remaining_bytes < 0;
    }
}
