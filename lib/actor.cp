/**
 * CP Actor Management (cap:actor)
 */

import { PID } from "cap:beam";


/**
 * 自愿出让执行权。
 */
pub async fn yield() {
    __zig__ {
        ctx.pc += 1;
        return;
    }
}

/**
 * 接收当前信箱的第一条消息。
 */
pub async fn receive(): *! {
    return await @receive();
}

/**
 * 暂停当前 Actor 的执行指定的毫秒数。
 */
pub async fn sleep(ms: u32) {
    if (@is_nest) {
        await @yield_to_beam(2, ms); // SystemAction.SLEEP = 2
    } else {
        __zig__ {
            const ts = std.os.linux.timespec{ .sec = @intCast(@divTrunc(ctx.ms, 1000)), .nsec = @intCast(@mod(ctx.ms, 1000) * 1000000) };
            _ = std.os.linux.nanosleep(&ts, null);
        }
    }
}

/**
 * 等待另一个 Actor 执行完毕。
 */
pub async fn join(pid: PID) {
    // 真实工业级实现应为 Monitor 机制。这里先作简单抽象。
    await @yield_to_beam(99, pid);
}

/**
 * 获取当前 Actor 的 PID。
 */
pub fn self_pid(): PID {
    return @self();
}

/**
 * 获取当前接收到消息的发送者 PID。
 */
pub fn sender(): PID {
    return @sender();
}

/**
 * 向目标 Actor 发送消息。
 */
pub fn send(target: PID, msg: auto): bool {
    return @send(target, msg);
}

/**
 * 向目标 Actor 回复消息。
 */
pub fn reply(target: PID, msg: auto): bool {
    return @reply(target, msg);
}
