// cap/test/test_bundler_graph_production_deep.cp
// 深度生产级回归测试：cap:bundler 模块图与两阶段扫描/延迟插件协调器 (lib/bundler/graph.cp)

import { test } from "cap:test";
import {
    GRAPH_MAX_INPUT_FILES,
    GRAPH_MAX_ENTRY_POINTS,
    GRAPH_MAX_HTML_IMPORTS,
    GraphLoaderKind,
    GraphScanTransition,
    BundlerGraph,
} from "cap:bundler";

test("bundler_graph: input file dedup, loader pre-counts & secondary path flag") {
    let g = BundlerGraph.init();

    const idx_ts = g.addInputFile("src/index.ts", "", GraphLoaderKind.Ts, false, 0x101)?;
    const idx_dup = g.addInputFile("src/index.ts", "", GraphLoaderKind.Ts, false, 0x999)?;
    if (idx_ts != 0 || idx_dup != 0) return err.DedupSourceIndexMismatch;

    const idx_css = g.addInputFile("src/app.css", "", GraphLoaderKind.Css, false, 0x202)?;
    const idx_img = g.addInputFile("assets/logo.png", "assets/logo.2x.png", GraphLoaderKind.File, false, 0x303)?;
    const idx_plug = g.addInputFile("virtual:env", "", GraphLoaderKind.Json, true, 0x404)?;

    * = g.markExportStarTarget(idx_ts)?;

    const st = g.stats();
    if (st.input_file_count != 4 ||
        st.css_file_count != 1 ||
        st.estimated_file_loader_count != 1 ||
        !st.has_any_secondary_paths) {
        return err.LoaderStatsMismatch;
    }

    if (f0 := g.getInputFile(idx_ts)) {
        if (!f0.flags.is_export_star_target || f0.flags.is_plugin_file) return err.FlagsMismatch0;
    } else {
        return err.MissingFile0;
    }
    if (f2 := g.getInputFile(idx_img)) {
        if (f2.secondaryPath().len != 18) return err.SecondaryPathMismatch;
    } else {
        return err.MissingFile2;
    }
    if (f3 := g.getInputFile(idx_plug)) {
        if (!f3.flags.is_plugin_file) return err.PluginFlagMismatch;
    } else {
        return err.MissingFile3;
    }
    * = idx_css;
}

test("bundler_graph: two-phase scan & onLoad .defer() wakeup to ReadyToLink") {
    let g = BundlerGraph.init();
    const s_main = g.addInputFile("src/main.ts", "", GraphLoaderKind.Ts, false, 1)?;
    const s_stats = g.addInputFile("virtual:stats.json", "", GraphLoaderKind.Json, true, 2)?;
    const s_manifest = g.addInputFile("virtual:manifest.json", "", GraphLoaderKind.Json, true, 3)?;

    // 启动 3 个并发扫描/加载任务
    * = g.incrementScanCounter()?;
    * = g.incrementScanCounter()?;
    * = g.incrementScanCounter()?;

    // 两个插件模块调用 .defer()
    const d1 = g.deferTask(s_stats)?;
    const d2 = g.deferTask(s_manifest)?;
    if (d1 != GraphScanTransition.Scanning || d2 != GraphScanTransition.Scanning) {
        return err.ExpectedStillScanning;
    }
    if (g.stats().pending_items != 1 || g.stats().deferred_pending != 2) {
        return err.UnexpectedCounterSplit;
    }

    // 主模块扫描结束 -> pending_items 归零，自动唤醒 2 个延迟插件任务
    const t_wake = g.decrementScanCounter()?;
    if (t_wake != GraphScanTransition.WakeDeferredPlugins) return err.ExpectedWakeDeferred;
    if (g.stats().pending_items != 2 || g.stats().deferred_pending != 0 || g.stats().deferred_rounds_completed != 1) {
        return err.WakeCountersMismatch;
    }

    // 两个延迟插件依次完成 -> 进入 ReadyToLink
    const t_mid = g.decrementScanCounter()?;
    if (t_mid != GraphScanTransition.Scanning) return err.ExpectedScanningWithOneLeft;
    const t_done = g.decrementScanCounter()?;
    if (t_done != GraphScanTransition.ReadyToLink) return err.ExpectedReadyToLink;
    if (g.stats().pending_items != 0 || g.stats().deferred_pending != 0) {
        return err.FinalCountersNotZero;
    }
    * = s_main;
}

test("bundler_graph: single-defer-per-onLoad guard & immediate wakeup when last pending defers") {
    let g = BundlerGraph.init();
    const s_only = g.addInputFile("virtual:only.json", "", GraphLoaderKind.Json, true, 9)?;

    * = g.incrementScanCounter()?;
    // 唯一在途任务调用 .defer()：pending 立即归零 -> 当场唤醒
    const t = g.deferTask(s_only)?;
    if (t != GraphScanTransition.WakeDeferredPlugins || g.stats().pending_items != 1) {
        return err.ImmediateWakeFailed;
    }

    // 同一模块二次调用 .defer() 必须被拦截（对齐 Bun plugins.mdx 规范）
    let second_blocked = false;
    * = g.deferTask(s_only) catch {
        second_blocked = true;
        GraphScanTransition.Scanning;
    };
    if (!second_blocked) return err.SecondDeferMustFail;

    // 正常收尾
    const end_t = g.decrementScanCounter()?;
    if (end_t != GraphScanTransition.ReadyToLink) return err.ExpectedReadyAfterEnd;

    // 归零后再 decrement 触发 underflow
    let underflow = false;
    * = g.decrementScanCounter() catch {
        underflow = true;
        GraphScanTransition.Scanning;
    };
    if (!underflow) return err.ExpectedUnderflow;
}

test("bundler_graph: entry point original names & server->HTML import pairs") {
    let g = BundlerGraph.init();
    const srv = g.addInputFile("src/server.ts", "", GraphLoaderKind.Ts, false, 11)?;
    const html = g.addInputFile("src/index.html", "", GraphLoaderKind.Html, false, 22)?;

    const ep_idx = g.addEntryPoint(srv, "virtual:entry-server")?;
    if (ep_idx != 0) return err.UnexpectedEpIndex;
    if (orig := g.getEntryOriginalName(srv)) {
        if (orig.len != 20) return err.OriginalNameLenMismatch;
    } else {
        return err.MissingOriginalName;
    }

    // 重复入口点必须被拦截
    let dup_ep = false;
    * = g.addEntryPoint(srv, "dup") catch {
        dup_ep = true;
        0;
    };
    if (!dup_ep) return err.DuplicateEntryMustFail;

    const hi_idx = g.addHtmlImport(srv, html)?;
    if (hi_idx != 0) return err.UnexpectedHtmlImportIndex;
    if (pair := g.getHtmlImport(0)) {
        if (pair.server_source_index != srv || pair.html_source_index != html) {
            return err.HtmlImportPairMismatch;
        }
    } else {
        return err.MissingHtmlImportPair;
    }
}

test("bundler_graph: input validation & invalid source_index rejection") {
    let g = BundlerGraph.init();

    let empty_err = false;
    * = g.addInputFile("", "", GraphLoaderKind.Js, false, 0) catch {
        empty_err = true;
        0;
    };
    if (!empty_err) return err.EmptyPathMustFail;

    let long_p: [140]u8 = ['p'; 140];
    let long_err = false;
    * = g.addInputFile(long_p[0..], "", GraphLoaderKind.Js, false, 0) catch {
        long_err = true;
        0;
    };
    if (!long_err) return err.LongPathMustFail;

    let bad_star = false;
    * = g.markExportStarTarget(99) catch {
        bad_star = true;
        false;
    };
    if (!bad_star) return err.BadIndexStarMustFail;

    let bad_html = false;
    * = g.addHtmlImport(0, 1) catch {
        bad_html = true;
        0;
    };
    if (!bad_html) return err.BadIndexHtmlMustFail;
}

test("bundler_graph: fixed-capacity bounds for files, entries & html imports") {
    let g = BundlerGraph.init();

    // 填满 64 个唯一路径输入文件
    let name: [8]u8 = ['f', '_', '0', '0', '.', 't', 's', 0];
    let i: usize = 0;
    while (i < GRAPH_MAX_INPUT_FILES) {
        name[2] = ('0' as u8) + ((i / 10) as u8);
        name[3] = ('0' as u8) + ((i % 10) as u8);
        * = g.addInputFile(name[0..7], "", GraphLoaderKind.Ts, false, i as u64)?;
        i += 1;
    }
    let file_overflow = false;
    * = g.addInputFile("overflow.ts", "", GraphLoaderKind.Ts, false, 999) catch {
        file_overflow = true;
        0;
    };
    if (!file_overflow) return err.FileOverflowMustFail;

    // 填满 16 个入口点与 16 个 HTML 导入对
    i = 0;
    while (i < GRAPH_MAX_ENTRY_POINTS) {
        * = g.addEntryPoint(i as u32, "ep")?;
        i += 1;
    }
    let ep_overflow = false;
    * = g.addEntryPoint(20, "ep_over") catch {
        ep_overflow = true;
        0;
    };
    if (!ep_overflow) return err.EpOverflowMustFail;

    i = 0;
    while (i < GRAPH_MAX_HTML_IMPORTS) {
        * = g.addHtmlImport(0, (i + 1) as u32)?;
        i += 1;
    }
    let hi_overflow = false;
    * = g.addHtmlImport(0, 30) catch {
        hi_overflow = true;
        0;
    };
    if (!hi_overflow) return err.HtmlImportOverflowMustFail;
}
