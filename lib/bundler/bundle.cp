// lib/bundler/bundle.cp

import { Transpiler } from "cap:transpiler";
import { LinkerContext } from "cap:linker";
import { Graph } from "cap:graph";
import { Watcher } from "cap:watcher";
import { JSBundler } from "cap:jsc_api";
import { DeferredBatchTask } from "cap:deferred_batch_task";
import { ThreadLock } from "cap:safety";
import { JSBundleCompletionTask } from "cap:js_bundle_completion_task";
import { Framework } from "cap:bake";
import { Index } from "cap:ast";

export struct BundleV2 {
    transpiler: Transpiler;
    client_transpiler: Transpiler?;
    ssr_transpiler: Transpiler;
    framework: Framework?;
    graph: Graph;
    linker: LinkerContext;
    bun_watcher: Watcher?;
    plugins: JSBundler.Plugin?;
    completion: JSBundleCompletionTask?;
    file_map: JSBundler.FileMap?;
    source_code_length: usize;

    // Remaining fields from bundle_v2.zig:
    // resolve_tasks_waiting_for_import_source_index
    // free_list
    // unique_key
    // dynamic_import_entry_points
    // has_on_parse_plugins
    // finalizers
    // drain_defer_task
    // asynchronous
    // thread_lock
    // has_any_top_level_await_modules
    // requested_exports
}
