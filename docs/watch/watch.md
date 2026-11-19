# `cap:watch` (`cap:watcher`) — 跨平台文件系统监控、路径过滤与事件合并防抖引擎

> **上游溯源**: `/data/dev/bun/src/watcher/Watcher.cp` & `/data/dev/bun/src/watcher/INotifyWatcher.cp`  
> **模块路径**: `lib/watch/` & `lib/watcher/`  
> **导入协议**: `import { Watcher, createWatcher, WatchOptions, WatchEvent, WatchAction, WatchEventOp, WatchEventKind, WatchMask, FileWatcher, WatchFilter, DebounceTracker, DebounceEngine, InotifyBackend } from "cap:watch";`（或 `"cap:watcher"`）

---

## 一、 架构概述与设计亮点

`cap:watch`（与 `cap:watcher` 共享统一解析出口）吸收自 Bun 的文件系统热重载监控子系统（`bun/src/watcher`），采用**内核事件驱动（Linux `inotify` / macOS `kqueue` 掩码抽象）+ 路径黑名单快筛 + 时间窗口防抖合并（Coalescing Debouncer）**三层流水线架构：

1. **内核原生非阻塞 `InotifyBackend`**：
   - 直接通过 `inotify_init1(IN_NONBLOCK | IN_CLOEXEC)`、`inotify_add_watch` 与 `inotify_rm_watch` 系统调用在 `4KB` 栈缓冲区内批量读取 16 字节对齐的内核 `inotify_event` 帧，全程零堆分配。
2. **双形态事件合并与防抖器 (`DebounceEngine` & `DebounceTracker<CAP>`)**：
   - **`DebounceEngine`（多操作位图合并）**：在设定的时间窗口（如 `50ms`）内，对同一路径连续触发的多次保存/修改/重命名事件通过 `WatchEventOp.merge` 合并为单次最终语义动作（优先级：`delete > rename > create > modify`），消除编辑器保存文件时产生的事件风暴。
   - **`DebounceTracker<CAP>`（固定容量纳秒级环形防抖器）**：基于编译期常量容量 `CAP` 的零分配环形驱逐表，通过 `shouldEmit(path, now_ns)` 在纳秒级时间戳下直接过滤高频重复触发。
3. **VCS 与构建缓存路径过滤器 (`WatchFilter`)**：
   - `WatchFilter.shouldIgnore(path)` 自动识别并拦截 `.git`、`node_modules`、`.cap`、`zig-cache` 目录以及编辑器临时交换文件（`*.swp`、`*.swo`、`*.tmp`、`*~`、`.DS_Store`）。

---

## 二、 事件掩码 (`WatchMask`) 与 API 矩阵

### 1. 内核事件掩码与 `WatchEventKind` 映射表

| 掩码常量 (`WatchMask`) | 十六进制值 | 映射 `WatchEventKind` | 映射 `WatchAction` | 触发场景说明 |
| :--- | :---: | :--- | :--- | :--- |
| `IN_CREATE` | `0x00000100` | `.Created` | `.create` | 在受监控目录内新建文件或子目录 |
| `IN_MODIFY` / `IN_CLOSE_WRITE` | `0x00000002` / `0x08` | `.Modified` | `.modify` | 文件内容被写入或可写文件句柄关闭 |
| `IN_DELETE` / `IN_DELETE_SELF` | `0x00000200` / `0x400` | `.Deleted` | `.delete` | 文件或受监控项自身被删除 |
| `IN_MOVED_FROM` / `IN_MOVED_TO` | `0x00000040` / `0x80` | `.Renamed` | `.rename` | 文件在目录间移动或重命名 |
| `IN_MOVE_SELF` | `0x00000800` | `.MovedSelf` | `.rename` | 受监控对象自身发生移动 |
| `IN_ATTRIB` | `0x00000004` | `.Attrib` | `.modify` | 文件权限、时间戳或扩展属性变更 |
| `IN_ISDIR` | `0x40000000` | `ev.isDirectory() == true` | `ev.is_dir == true` | 指示当前事件目标为目录 |

### 2. 核心组件 API 一览

| 组件名称 | 核心方法 | 功能说明 |
| :--- | :--- | :--- |
| **`Watcher`** | `createWatcher(opts)`, `watch(p)`, `unwatch(p)`, `pause()`, `resumeWatch()`, `poll(now_ms, &out)`, `close()` | 完整封装 `InotifyBackend` 与 `DebounceEngine` 的热重载监控器 |
| **`FileWatcher`** | `FileWatcher.init()`, `addWatch(path, is_dir)`, `removeWatch(wd)`, `findPath(wd)`, `activeCount()` | 轻量级监控描述符（`wd` <-> `path`）映射表管理器 |
| **`WatchFilter`** | `WatchFilter.shouldIgnore(path): bool` | 零分配检测路径是否属于构建缓存、VCS 或编辑器交换文件 |
| **`DebounceTracker<CAP>`** | `DebounceTracker<8>.init(window_ns)`, `shouldEmit(path, now_ns)`, `clear()` | 固定容量泛型纳秒时间戳防抖器 |
| **`DebounceEngine`** | `DebounceEngine.init(ms)`, `push(p, act, dir, ms)`, `drainReady(ms, &out)`, `drainAll(&out)` | 支持操作位图合并（`WatchEventOp.merge`）的事件防抖队列 |

---

## 三、 代码示例

### 1. 路径过滤 (`WatchFilter`) 与纳秒防抖器 (`DebounceTracker<CAP>`)

```typescript
import { WatchFilter, DebounceTracker } from "cap:watch";

fn demoFilterAndDebounce(): void {
    // 1. 过滤构建产物与编辑器临时文件
    let ig1 = WatchFilter.shouldIgnore(".git/HEAD");           // true
    let ig2 = WatchFilter.shouldIgnore("build/.cap/out.zig");  // true
    let ig3 = WatchFilter.shouldIgnore("src/main.cp~");        // true
    let ig4 = WatchFilter.shouldIgnore("src/main.cp");         // false

    // 2. 50ms (50,000,000 ns) 窗口的 8 槽位零分配防抖器
    let tracker = DebounceTracker<8>.init(50000000);
    let emit1 = tracker.shouldEmit("src/main.cp", 100000000);  // true (首次触发)
    let emit2 = tracker.shouldEmit("src/main.cp", 120000000);  // false (20ms 内重复触发被抑制)
    let emit3 = tracker.shouldEmit("src/main.cp", 160000000);  // true (60ms 后超出窗口，正常放行)
}
```

### 2. `Watcher` 生命周期管理、事件合并与轮询 (`createWatcher`)

```typescript
import {
    WatchOptions,
    WatchAction,
    WatchEvent,
    createWatcher,
} from "cap:watch";

fn demoWatcherLifecycle(): void {
    let watcher = createWatcher(WatchOptions{
        recursive: true,
        debounce_ms: 30,
    });
    defer watcher.close();

    // 注册监控目录与文件
    * = watcher.watch("src");
    * = watcher.watch("cap.json");

    // 模拟在 100ms 与 110ms 连续发生两次事件（create + modify）
    * = watcher.dispatchMockEvent("src/app.cp", WatchAction.create, false, 100);
    * = watcher.dispatchMockEvent("src/app.cp", WatchAction.modify, false, 110);

    // 在 150ms（超过 30ms 防抖窗口）拉取合并后的就绪事件
    let events: [8]WatchEvent = undefined;
    const ready_n = watcher.poll(150, &events);
    // ready_n => 1 (两次事件被合并为 1 条 WatchAction.create 事件)
}
```
