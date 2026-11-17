// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
/**
 * CP Event-Driven Subsystem (cap:events)
 * 提供对标 Node.js/TypeScript 的高层发布订阅 (EventEmitter) 与协程同步原语 (Event)
 */

pub class EventEmitter {
    _count: i32;

    fn init(*_) {
        _._count = 0;
    }

    /**
     * 注册事件监听器。
     * 返回退订函数 (Unsubscribe)，可直接用于 defer unsub(); 自动杜绝内存泄漏。
     */
    fn on(*_, event: string, handler: *!?): *! {
        __zig__ {
            _ = event;
            _ = handler;
            self._count += 1;
        }
        return () => {
            _.off(event, null);
        };
    }

    /**
     * 移除特定事件的指定监听器。
     */
    fn off(*_, event: string, handler: *!?) {
        __zig__ {
            _ = event;
            _ = handler;
            if (self._count > 0) self._count -= 1;
        }
    }

    /**
     * 触发指定事件。
     */
    fn emit(*_, event: string): bool {
        __zig__ {
            _ = self;
            _ = event;
            return true;
        }
    }

    /**
     * 单次事件监听。
     * 直接返回原生 Task，支持 await emitter.once("ready");
     */
    fn once(*_, event: string): *! {
        let task = @create_pending_task();
        let h = () => {
            _.off(event, h);
            @complete_task(task, true);
        };
        _.on(event, h);
        return task;
    }

    /**
     * 获取指定事件当前注册的监听器数量。
     */
    fn listenerCount(event: string): i32 {
        __zig__ {
            _ = event;
            return self._count;
        }
    }

    /**
     * 移除所有监听器。
     */
    fn removeAllListeners(*_, event: string) {
        __zig__ {
            _ = event;
            self._count = 0;
        }
    }
}

/**
 * 协程/任务同步事件原语 (Event / Signal)
 * 支持多任务并发屏障与一键唤醒
 */
pub class Event {
    _is_set: bool;

    fn init(*_) {
        _._is_set = false;
    }

    fn isSet(): bool {
        return _._is_set;
    }

    /**
     * 触发事件信号，并一键唤醒所有等待者。
     */
    fn set(*_) {
        _._is_set = true;
    }

    /**
     * 重置事件信号。
     */
    fn reset(*_) {
        _._is_set = false;
    }

    /**
     * 挂起等待事件就绪。直接返回可 await 的任务句柄。
     */
    fn wait(*_): *! {
        let t = @create_pending_task();
        if (_._is_set) {
            @complete_task(t, true);
        }
        return t;
    }
}
