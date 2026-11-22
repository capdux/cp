// Source: Absorbed from /data/dev/bun/src/watcher/Watcher.cp | Synced: 2026-11-22T12:49:17Z
// CP Standard Library: cap:watcher

export {
    WatchAction,
    WatchEventOp,
    WatchEventKind,
    WatchMask,
    WatchEvent,
    WatchItemKind,
    WatchOptions,
    WatchCallback,
    getNowMs,
} from "./events.cp";

export {
    WatchFilter,
    TrackerEntry,
    DebounceTracker,
} from "./filter.cp";

export {
    WatchEntry,
    FileWatcher,
} from "./fileWatcher.cp";

export {
    MAX_DEBOUNCE_ENTRIES,
    DEBOUNCE_PATH_MAX,
    DebounceEntry,
    DebounceEngine,
} from "./debounce.cp";

export {
    IN_ACCESS,
    IN_MODIFY,
    IN_ATTRIB,
    IN_CLOSE_WRITE,
    IN_CLOSE_NOWRITE,
    IN_OPEN,
    IN_MOVED_FROM,
    IN_MOVED_TO,
    IN_CREATE,
    IN_DELETE,
    IN_DELETE_SELF,
    IN_MOVE_SELF,
    IN_ISDIR,
    IN_NONBLOCK,
    IN_CLOEXEC,
    DEFAULT_WATCH_MASK,
    InotifyRawEvent,
    InotifyBackend,
} from "./inotify.cp";

export {
    MAX_WATCH_ITEMS,
    MAX_PATH_LEN,
    MAX_RAW_BATCH,
    WatchItem,
    Watcher,
} from "./watcher.cp";

import { Watcher } from "./watcher.cp";
import { WatchOptions } from "./events.cp";

pub fn createWatcher(options: WatchOptions): Watcher {
    return Watcher.init(options);
}
