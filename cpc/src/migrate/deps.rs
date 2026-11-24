//! `cap from -ts/-rs` 迁移引擎：三层依赖解析、本地依赖源码定位、导入改写、`cap.json` 与 `migrate.md` 报告生成。

use std::collections::{BTreeMap, HashMap};
use std::fs;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use serde_json::{Map as JsonMap, Value as JsonValue};

/// 源项目语言类型。
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum SourceLang {
    Ts,
    Rs,
}

impl SourceLang {
    pub fn display_name(self) -> &'static str {
        match self {
            SourceLang::Ts => "TypeScript / JavaScript",
            SourceLang::Rs => "Rust",
        }
    }

    pub fn short_flag(self) -> &'static str {
        match self {
            SourceLang::Ts => "-ts",
            SourceLang::Rs => "-rs",
        }
    }
}

/// 三层依赖映射分级结果。
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum DepTier {
    /// Tier 1: 直接映射到 CP 内置标准库 `cap:*`（零外部依赖）。
    Builtin { cap_mod: String, reason: String },
    /// Tier 2: 映射到官方可安装的 `@cap/*` 生态包。
    OfficialPkg {
        pkg_name: String,
        version: String,
        reason: String,
    },
    /// Tier 3: 递归转换为本地独立可发布的 CP 库。
    RecursiveConverted {
        pkg_name: String,
        out_rel_dir: String,
        src_dir: String,
        file_count: usize,
    },
    /// 未找到本地源码且不在内置/官方映射表中的依赖，待人工评估。
    PendingManual { reason: String },
}

/// 单个依赖项的映射记录。
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct DepMapping {
    pub orig_name: String,
    pub orig_version: String,
    pub is_dev: bool,
    pub tier: DepTier,
}

/// 单个源文件的迁移明细。
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct FileReport {
    pub orig_rel_path: String,
    pub cp_rel_path: String,
    pub orig_lines: usize,
    pub cp_lines: usize,
    pub parse_ok: bool,
    pub parse_err: Option<String>,
    pub todo_count: usize,
    pub opt_notes: Vec<String>,
}

/// 项目级迁移汇总，用于生成 `cap.json` 与 `migrate.md`。
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ProjectMigrateSummary {
    pub project_name: String,
    pub version: String,
    pub lang: SourceLang,
    pub src_dir: String,
    pub out_dir: String,
    pub deps: Vec<DepMapping>,
    pub files: Vec<FileReport>,
    pub converted_dep_pkgs: Vec<String>,
}

// ============================================================================
// 1. package.json 依赖解析 (TypeScript / JavaScript)
// ============================================================================

/// 解析 `src_dir/package.json` 的项目名、版本以及 `dependencies` + `peerDependencies` + `devDependencies`。
///
/// 返回 `(project_name, version, Vec<(dep_name, dep_version, is_dev)>)`。
pub fn parse_package_json_deps(src_dir: &Path) -> (String, String, Vec<(String, String, bool)>) {
    let fallback_name = src_dir
        .file_name()
        .and_then(|s| s.to_str())
        .filter(|s| !s.is_empty())
        .unwrap_or("migrated-ts-project")
        .to_string();
    let fallback_version = "0.1.0".to_string();

    let pkg_path = src_dir.join("package.json");
    let content = match fs::read_to_string(&pkg_path) {
        Ok(c) => c,
        Err(_) => return (fallback_name, fallback_version, Vec::new()),
    };

    let json: JsonValue = match serde_json::from_str(&content) {
        Ok(v) => v,
        Err(_) => return (fallback_name, fallback_version, Vec::new()),
    };

    let name = json
        .get("name")
        .and_then(|v| v.as_str())
        .filter(|s| !s.trim().is_empty())
        .map(|s| s.trim().to_string())
        .unwrap_or(fallback_name);

    let version = json
        .get("version")
        .and_then(|v| v.as_str())
        .filter(|s| !s.trim().is_empty())
        .map(|s| s.trim().to_string())
        .unwrap_or(fallback_version);

    // 使用 BTreeMap 去重并保证确定性输出；非 dev 依赖优先于 dev 依赖
    let mut dep_map: BTreeMap<String, (String, bool)> = BTreeMap::new();

    for (section, is_dev) in [
        ("dependencies", false),
        ("peerDependencies", false),
        ("devDependencies", true),
    ] {
        if let Some(obj) = json.get(section).and_then(|v| v.as_object()) {
            for (k, v) in obj {
                let ver = v.as_str().unwrap_or("*").trim().to_string();
                match dep_map.get(k) {
                    Some((_, existing_is_dev)) if !*existing_is_dev && is_dev => {
                        // 已在运行时依赖中声明，不被 devDependencies 覆盖
                    }
                    _ => {
                        dep_map.insert(k.clone(), (ver, is_dev));
                    }
                }
            }
        }
    }

    let deps = dep_map
        .into_iter()
        .map(|(k, (v, is_dev))| (k, v, is_dev))
        .collect();

    (name, version, deps)
}

// ============================================================================
// 2. Cargo.toml 轻量纯 Rust 解析器 (支持 workspace 与 path)
// ============================================================================

#[derive(Debug, Default, Clone)]
struct CargoDepEntry {
    package_alias: Option<String>,
    version: Option<String>,
    path: Option<PathBuf>,
    workspace: bool,
    is_dev: bool,
}

#[derive(Debug, Default)]
struct ParsedCargoToml {
    pkg_name: Option<String>,
    pkg_version: Option<String>,
    pkg_version_workspace: bool,
    has_workspace_section: bool,
    ws_version: Option<String>,
    ws_deps: BTreeMap<String, CargoDepEntry>,
    deps: BTreeMap<String, CargoDepEntry>,
}

/// 解析 `src_dir/Cargo.toml` 的 `[package]`、`[dependencies]`、`[dev-dependencies]`，
/// 支持 `path = "..."`、`workspace = true` 以及父级 workspace `Cargo.toml` 继承解析。
///
/// 返回 `(project_name, version, Vec<(dep_name, dep_version, is_dev, explicit_path)>)`。
pub fn parse_cargo_toml_deps(
    src_dir: &Path,
) -> (String, String, Vec<(String, String, bool, Option<PathBuf>)>) {
    let fallback_name = src_dir
        .file_name()
        .and_then(|s| s.to_str())
        .filter(|s| !s.is_empty())
        .unwrap_or("migrated-rs-project")
        .to_string();
    let fallback_version = "0.1.0".to_string();

    let cargo_path = src_dir.join("Cargo.toml");
    let content = match fs::read_to_string(&cargo_path) {
        Ok(c) => c,
        Err(_) => return (fallback_name, fallback_version, Vec::new()),
    };

    let parsed = parse_cargo_toml_str(&content);

    // 按需查找父级 workspace Cargo.toml
    let needs_ws = parsed.pkg_version_workspace || parsed.deps.values().any(|d| d.workspace);
    let workspace_info = if needs_ws {
        find_and_parse_workspace_cargo(src_dir)
    } else {
        None
    };

    let project_name = parsed.pkg_name.unwrap_or(fallback_name);
    let version = if parsed.pkg_version_workspace {
        workspace_info
            .as_ref()
            .and_then(|(_, ws)| ws.ws_version.clone())
            .or(parsed.pkg_version)
            .unwrap_or(fallback_version)
    } else {
        parsed.pkg_version.unwrap_or(fallback_version)
    };

    let mut result = Vec::new();
    for (key, entry) in parsed.deps {
        let mut dep_name = entry.package_alias.clone().unwrap_or_else(|| key.clone());
        let mut dep_ver = entry.version.clone();
        let mut dep_path = entry.path.clone();

        if entry.workspace {
            if let Some((ws_root, ws_parsed)) = &workspace_info {
                if let Some(ws_dep) = ws_parsed.ws_deps.get(&key) {
                    if let Some(alias) = &ws_dep.package_alias {
                        dep_name = alias.clone();
                    }
                    if dep_ver.is_none() {
                        dep_ver = ws_dep.version.clone();
                    }
                    if dep_path.is_none() {
                        if let Some(rel_p) = &ws_dep.path {
                            // workspace.dependencies 中的 path 相对于 workspace 根目录
                            dep_path = Some(if rel_p.is_absolute() {
                                rel_p.clone()
                            } else {
                                ws_root.join(rel_p)
                            });
                        }
                    }
                }
            }
        }

        result.push((
            dep_name,
            dep_ver.unwrap_or_else(|| "*".to_string()),
            entry.is_dev,
            dep_path,
        ));
    }

    (project_name, version, result)
}

fn find_and_parse_workspace_cargo(src_dir: &Path) -> Option<(PathBuf, ParsedCargoToml)> {
    let mut cur = src_dir.parent();
    for _ in 0..6 {
        let dir = cur?;
        let candidate = dir.join("Cargo.toml");
        if candidate.is_file() {
            if let Ok(content) = fs::read_to_string(&candidate) {
                let parsed = parse_cargo_toml_str(&content);
                if parsed.has_workspace_section {
                    return Some((dir.to_path_buf(), parsed));
                }
            }
        }
        cur = dir.parent();
    }
    None
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum TomlSection {
    Other,
    Package,
    WorkspacePackage,
    Deps { is_dev: bool },
    DepSubtable { name: String, is_dev: bool },
    WorkspaceDeps,
    WorkspaceDepSubtable { name: String },
}

fn parse_cargo_toml_str(input: &str) -> ParsedCargoToml {
    let mut out = ParsedCargoToml::default();
    let mut section = TomlSection::Other;

    // 合并跨行的 inline table `{ ... }` 或数组 `[ ... ]`
    let logical_lines = collect_logical_toml_lines(input);

    for raw_line in logical_lines {
        let line = raw_line.trim();
        if line.is_empty() {
            continue;
        }

        // Section header: [section.name]（跳过 [[array_of_tables]]）
        if line.starts_with('[') && line.ends_with(']') && !line.starts_with("[[") {
            let header = line[1..line.len() - 1].trim();
            if header == "workspace" || header.starts_with("workspace.") {
                out.has_workspace_section = true;
            }
            section = classify_cargo_section(header);
            continue;
        }

        let Some((raw_key, raw_val)) = split_toml_kv(line) else {
            continue;
        };
        let key = raw_key.trim();
        let val = raw_val.trim();

        match &section {
            TomlSection::Package => match key {
                "name" => {
                    if let Some(s) = parse_toml_string(val) {
                        out.pkg_name = Some(s);
                    }
                }
                "version" => {
                    if let Some(s) = parse_toml_string(val) {
                        out.pkg_version = Some(s);
                    } else if val.starts_with('{') {
                        let fields = parse_inline_table(val);
                        if fields
                            .get("workspace")
                            .map(|v| v == "true")
                            .unwrap_or(false)
                        {
                            out.pkg_version_workspace = true;
                        }
                    }
                }
                "version.workspace" => {
                    if val == "true" {
                        out.pkg_version_workspace = true;
                    }
                }
                _ => {}
            },
            TomlSection::WorkspacePackage => {
                if key == "version" {
                    if let Some(s) = parse_toml_string(val) {
                        out.ws_version = Some(s);
                    }
                }
            }
            TomlSection::Deps { is_dev } => {
                insert_dep_kv(&mut out.deps, key, val, *is_dev);
            }
            TomlSection::DepSubtable { name, is_dev } => {
                let entry = out
                    .deps
                    .entry(name.clone())
                    .or_insert_with(|| CargoDepEntry {
                        is_dev: *is_dev,
                        ..Default::default()
                    });
                if !*is_dev {
                    entry.is_dev = false;
                }
                apply_dep_field(entry, key, val);
            }
            TomlSection::WorkspaceDeps => {
                insert_dep_kv(&mut out.ws_deps, key, val, false);
            }
            TomlSection::WorkspaceDepSubtable { name } => {
                let entry = out.ws_deps.entry(name.clone()).or_default();
                apply_dep_field(entry, key, val);
            }
            TomlSection::Other => {}
        }
    }

    out
}

fn classify_cargo_section(header: &str) -> TomlSection {
    match header {
        "package" => TomlSection::Package,
        "workspace.package" => TomlSection::WorkspacePackage,
        "dependencies" => TomlSection::Deps { is_dev: false },
        "dev-dependencies" => TomlSection::Deps { is_dev: true },
        "workspace.dependencies" => TomlSection::WorkspaceDeps,
        _ => {
            if let Some(rest) = header.strip_prefix("dependencies.") {
                return TomlSection::DepSubtable {
                    name: unquote_toml_key(rest),
                    is_dev: false,
                };
            }
            if let Some(rest) = header.strip_prefix("dev-dependencies.") {
                return TomlSection::DepSubtable {
                    name: unquote_toml_key(rest),
                    is_dev: true,
                };
            }
            if let Some(rest) = header.strip_prefix("workspace.dependencies.") {
                return TomlSection::WorkspaceDepSubtable {
                    name: unquote_toml_key(rest),
                };
            }
            // 处理 target.'cfg(...)'.dependencies / target.'cfg(...)'.dev-dependencies
            if header.starts_with("target.") {
                if header.ends_with(".dependencies") {
                    return TomlSection::Deps { is_dev: false };
                }
                if header.ends_with(".dev-dependencies") {
                    return TomlSection::Deps { is_dev: true };
                }
                if let Some(idx) = header.rfind(".dependencies.") {
                    let name = &header[idx + ".dependencies.".len()..];
                    return TomlSection::DepSubtable {
                        name: unquote_toml_key(name),
                        is_dev: false,
                    };
                }
                if let Some(idx) = header.rfind(".dev-dependencies.") {
                    let name = &header[idx + ".dev-dependencies.".len()..];
                    return TomlSection::DepSubtable {
                        name: unquote_toml_key(name),
                        is_dev: true,
                    };
                }
            }
            TomlSection::Other
        }
    }
}

fn insert_dep_kv(
    map: &mut BTreeMap<String, CargoDepEntry>,
    raw_key: &str,
    val: &str,
    is_dev: bool,
) {
    // 支持 dotted key，如 `axum-core.workspace = true` 或 `foo.path = "../foo"`
    if let Some((dep_key, field_key)) = split_dotted_dep_key(raw_key) {
        let entry = map.entry(dep_key).or_insert_with(|| CargoDepEntry {
            is_dev,
            ..Default::default()
        });
        if !is_dev {
            entry.is_dev = false;
        }
        apply_dep_field(entry, &field_key, val);
        return;
    }

    let dep_key = unquote_toml_key(raw_key);
    let entry = map.entry(dep_key).or_insert_with(|| CargoDepEntry {
        is_dev,
        ..Default::default()
    });
    if !is_dev {
        entry.is_dev = false;
    }

    if let Some(ver) = parse_toml_string(val) {
        entry.version = Some(ver);
    } else if val.starts_with('{') {
        let fields = parse_inline_table(val);
        for (fk, fv) in fields {
            apply_dep_field(entry, &fk, &fv);
        }
    }
}

fn apply_dep_field(entry: &mut CargoDepEntry, field: &str, val: &str) {
    match field.trim() {
        "version" => {
            if let Some(s) = parse_toml_string(val) {
                entry.version = Some(s);
            }
        }
        "path" => {
            if let Some(s) = parse_toml_string(val) {
                entry.path = Some(PathBuf::from(s));
            }
        }
        "package" => {
            if let Some(s) = parse_toml_string(val) {
                entry.package_alias = Some(s);
            }
        }
        "workspace" => {
            if val.trim() == "true" {
                entry.workspace = true;
            }
        }
        _ => {}
    }
}

fn collect_logical_toml_lines(input: &str) -> Vec<String> {
    let mut lines = Vec::new();
    let mut buf = String::new();
    let mut brace_depth: i32 = 0;
    let mut bracket_depth: i32 = 0;

    for raw in input.lines() {
        let clean = strip_toml_comment(raw).trim();
        if clean.is_empty() && buf.is_empty() {
            continue;
        }

        if !buf.is_empty() {
            buf.push(' ');
        }
        buf.push_str(clean);

        // 统计引号外的 `{}` 与 `[]` 嵌套深度（跳过 section header 行）
        let is_section =
            buf.starts_with('[') && brace_depth == 0 && bracket_depth == 0 && !buf.contains('=');
        if !is_section {
            let (db, dbr) = count_delimiters_outside_quotes(clean);
            brace_depth += db;
            bracket_depth += dbr;
            if brace_depth < 0 {
                brace_depth = 0;
            }
            if bracket_depth < 0 {
                bracket_depth = 0;
            }
        }

        if brace_depth == 0 && bracket_depth == 0 {
            lines.push(std::mem::take(&mut buf));
        }
    }

    if !buf.trim().is_empty() {
        lines.push(buf);
    }

    lines
}

fn strip_toml_comment(line: &str) -> &str {
    let bytes = line.as_bytes();
    let mut in_double = false;
    let mut in_single = false;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\'' {
                in_single = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'#' => return &line[..i],
                _ => {}
            }
        }
        i += 1;
    }
    line
}

fn count_delimiters_outside_quotes(s: &str) -> (i32, i32) {
    let bytes = s.as_bytes();
    let mut in_double = false;
    let mut in_single = false;
    let mut brace = 0i32;
    let mut bracket = 0i32;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\'' {
                in_single = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'{' => brace += 1,
                b'}' => brace -= 1,
                b'[' => bracket += 1,
                b']' => bracket -= 1,
                _ => {}
            }
        }
        i += 1;
    }
    (brace, bracket)
}

fn split_toml_kv(line: &str) -> Option<(&str, &str)> {
    let bytes = line.as_bytes();
    let mut in_double = false;
    let mut in_single = false;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\'' {
                in_single = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'=' => return Some((&line[..i], &line[i + 1..])),
                _ => {}
            }
        }
        i += 1;
    }
    None
}

fn split_dotted_dep_key(raw_key: &str) -> Option<(String, String)> {
    let trimmed = raw_key.trim();
    if trimmed.starts_with('"') || trimmed.starts_with('\'') {
        let quote = trimmed.chars().next()?;
        let rest = &trimmed[1..];
        let end = rest.find(quote)?;
        let dep = rest[..end].to_string();
        let after = rest[end + 1..].trim();
        let field = after.strip_prefix('.')?.trim();
        if field.is_empty() {
            return None;
        }
        return Some((dep, field.to_string()));
    }
    let dot = trimmed.find('.')?;
    let dep = trimmed[..dot].trim();
    let field = trimmed[dot + 1..].trim();
    if dep.is_empty() || field.is_empty() {
        return None;
    }
    Some((dep.to_string(), field.to_string()))
}

fn unquote_toml_key(raw: &str) -> String {
    let s = raw.trim();
    if (s.starts_with('"') && s.ends_with('"')) || (s.starts_with('\'') && s.ends_with('\'')) {
        if s.len() >= 2 {
            return s[1..s.len() - 1].to_string();
        }
    }
    s.to_string()
}

fn parse_toml_string(val: &str) -> Option<String> {
    let s = val.trim();
    if s.starts_with('"') && s.ends_with('"') && s.len() >= 2 {
        return Some(
            s[1..s.len() - 1]
                .replace("\\\"", "\"")
                .replace("\\\\", "\\"),
        );
    }
    if s.starts_with('\'') && s.ends_with('\'') && s.len() >= 2 {
        return Some(s[1..s.len() - 1].to_string());
    }
    None
}

fn parse_inline_table(val: &str) -> HashMap<String, String> {
    let mut map = HashMap::new();
    let trimmed = val.trim();
    let inner = trimmed
        .strip_prefix('{')
        .and_then(|s| s.strip_suffix('}'))
        .unwrap_or(trimmed);

    // 按顶层逗号切分字段（忽略引号和嵌套 `{}` / `[]` 内的逗号）
    let mut parts = Vec::new();
    let bytes = inner.as_bytes();
    let mut start = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut depth = 0i32;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\'' {
                in_single = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'{' | b'[' => depth += 1,
                b'}' | b']' => depth -= 1,
                b',' if depth == 0 => {
                    parts.push(&inner[start..i]);
                    start = i + 1;
                }
                _ => {}
            }
        }
        i += 1;
    }
    if start < inner.len() {
        parts.push(&inner[start..]);
    }

    for part in parts {
        if let Some((k, v)) = split_toml_kv(part.trim()) {
            map.insert(unquote_toml_key(k), v.trim().to_string());
        }
    }
    map
}

// ============================================================================
// 3. Tier 1 (内置 cap:*) 与 Tier 2 (官方 @cap/*) 依赖映射表
// ============================================================================

/// 按三层策略查询依赖是否可直接映射到 Tier 1 内置 `cap:*` 或 Tier 2 官方 `@cap/*` 包。
/// 返回 `None` 表示需进入 Tier 3（本地源码递归转换）或标记为 `PendingManual`。
pub fn resolve_builtin_or_official(lang: SourceLang, orig_name: &str) -> Option<DepTier> {
    let name = orig_name.trim();
    if name.is_empty() {
        return None;
    }

    // ---------- Tier 1: 语言专属内置标准库 cap:* 映射 ----------
    let tier1: Option<(&'static str, &'static str)> = match lang {
        SourceLang::Ts => match name {
            "cookie" => Some(("cap:web", "CP 内置 Cookie 解析与序列化（cap:web）")),
            "memoirist" | "find-my-way" | "rou3" => {
                Some(("cap:web", "CP 内置 Radix Tree 高性能路由器（cap:web）"))
            }
            "fast-decode-uri-component" | "qs" => {
                Some(("cap:web", "CP 内置 URI 解码与 QueryString 解析（cap:web）"))
            }
            "exact-mirror" | "zod" | "valibot" | "arktype" | "@sinclair/typebox" => {
                Some(("cap:web", "CP 内置强类型 Schema 校验与数据镜像（cap:web）"))
            }
            "file-type" | "mime-types" => {
                Some(("cap:web", "CP 内置 MIME 类型与文件魔数识别（cap:web）"))
            }
            "@elysiajs/openapi" | "openapi-types" => {
                Some(("cap:web", "CP 内置 OpenAPI 契约与文档生成（cap:web）"))
            }
            "better-sqlite3" | "bun:sqlite" => {
                Some(("cap:sqlite", "CP 内置原生 SQLite3 绑定（cap:sqlite）"))
            }
            "jose" | "jsonwebtoken" | "crypto" | "node:crypto" => Some((
                "cap:crypto",
                "CP 内置 BoringSSL 密码学与 JWT 支持（cap:crypto）",
            )),
            "node:fs" | "fs" | "node:fs/promises" | "fs/promises" => {
                Some(("cap:fs", "CP 内置文件系统标准库（cap:fs）"))
            }
            "node:path" | "path" => Some(("cap:path", "CP 内置跨平台路径处理库（cap:path）")),
            "node:url" | "url" => Some(("cap:url", "CP 内置 WHATWG URL 解析库（cap:url）")),
            "node:http" | "http" | "node:https" | "https" => {
                Some(("cap:http", "CP 内置高性能 HTTP 协议栈（cap:http）"))
            }
            "node:net" | "net" => Some(("cap:net", "CP 内置异步 TCP/UDP 网络库（cap:net）")),
            "ws" => Some(("cap:ws", "CP 内置 WebSocket 客户端/服务端（cap:ws）")),
            "uuid" => Some(("cap:time", "CP 内置 UUID/ULID 与时间标准库（cap:time）")),
            "semver" => Some(("cap:semver", "CP 内置 Semver 语义化版本库（cap:semver）")),
            "toml" => Some(("cap:toml", "CP 内置 TOML 解析与序列化库（cap:toml）")),
            "node:zlib" | "zlib" => Some(("cap:zlib", "CP 内置 zlib/gzip 压缩库（cap:zlib）")),
            "node:buffer" | "buffer" => Some(("cap:bytes", "CP 内置零拷贝字节缓冲（cap:bytes）")),
            "node:os" | "os" | "node:process" | "process" => {
                Some(("cap:os", "CP 内置操作系统与环境变量标准库（cap:os）"))
            }
            "node:child_process" | "child_process" => {
                Some(("cap:process", "CP 内置子进程管理标准库（cap:process）"))
            }
            "node:test" | "bun:test" => Some(("cap:test", "CP 内置单元测试框架（cap:test）")),
            _ => None,
        },
        SourceLang::Rs => {
            let norm = name.replace('_', "-");
            match norm.as_str() {
                "matchit" | "http" | "headers" | "mime" | "tower-http" | "axum-core" | "h2"
                | "httparse" => Some((
                    "cap:web",
                    "CP 内置 Web 路由、HTTP/1.1/2、头与中间件原语（cap:web）",
                )),
                "cookie" | "httpdate" => {
                    Some(("cap:http", "CP 内置 HTTP Cookie 与日期头编解码（cap:http）"))
                }
                "memchr" => Some(("cap:simd", "CP 内置 16-lane SIMD 字节扫描核（cap:simd）")),
                "smallvec" => Some((
                    "cap:collections",
                    "CP 内置零堆内联溢出 SmallVec（cap:collections）",
                )),
                "url" | "percent-encoding" | "form-urlencoded" => {
                    Some(("cap:url", "CP 内置 URL 与百分号编解码库（cap:url）"))
                }
                "serde-json" => Some(("cap:json", "CP 内置高性能 JSON 编解码库（cap:json）")),
                "regex" | "regex-automata" => {
                    Some(("cap:regex", "CP 内置正则引擎标准库（cap:regex）"))
                }
                "rusqlite" => Some(("cap:sqlite", "CP 内置原生 SQLite3 绑定（cap:sqlite）")),
                "chrono" | "time" | "uuid" => {
                    Some(("cap:time", "CP 内置时间日期与 UUID 标准库（cap:time）"))
                }
                "sha2" | "hmac" | "aes-gcm" | "ed25519-dalek" | "base64" => Some((
                    "cap:crypto",
                    "CP 内置 BoringSSL 密码学与 Base64 库（cap:crypto）",
                )),
                "tokio" | "futures" | "futures-util" | "pin-project-lite" => Some((
                    "cap:async",
                    "CP 内置异步运行时与 Future/Actor 调度（cap:async）",
                )),
                "bytes" => Some(("cap:bytes", "CP 内置零拷贝字节切片与缓冲（cap:bytes）")),
                "tracing" | "log" => {
                    Some(("cap:obs", "CP 内置结构化日志与可观测性标准库（cap:obs）"))
                }
                "toml" => Some(("cap:toml", "CP 内置 TOML 解析与序列化库（cap:toml）")),
                "csv" => Some(("cap:doc", "CP 内置文档与 CSV 解析库（cap:doc）")),
                "flate2" => Some(("cap:zlib", "CP 内置 zlib/gzip 压缩库（cap:zlib）")),
                "tar" => Some(("cap:pack", "CP 内置归档与 tar 打包库（cap:pack）")),
                "prost" => Some(("cap:compiler", "CP 内置协议与代码生成支持（cap:compiler）")),
                "semver" => Some(("cap:semver", "CP 内置 Semver 语义化版本库（cap:semver）")),
                "thiserror" | "anyhow" => Some((
                    "cap:core",
                    "CP 原生错误联合 `?T` 与核心错误原语（cap:core）",
                )),
                _ => None,
            }
        }
    };

    if let Some((cap_mod, reason)) = tier1 {
        return Some(DepTier::Builtin {
            cap_mod: cap_mod.to_string(),
            reason: reason.to_string(),
        });
    }

    // ---------- Tier 2: 官方可安装包 @cap/* 映射表 ----------
    let norm = name.replace('_', "-");
    let tier2: Option<(&'static str, &'static str)> = match norm.as_str() {
        "zstd" => Some(("@cap/zstd", "官方 Zstandard 原生压缩包")),
        "lz4" => Some(("@cap/lz4", "官方 LZ4 极速压缩包")),
        "yyjson" => Some(("@cap/yyjson", "官方 yyjson 高性能 JSON 引擎")),
        "lmdb" | "heed" => Some(("@cap/lmdb", "官方 LMDB 嵌入式 KV 存储")),
        "tree-sitter" => Some(("@cap/tree-sitter", "官方 Tree-sitter 增量语法解析包")),
        "yaml" | "js-yaml" | "serde-yaml" => Some(("@cap/yaml", "官方 YAML 解析与序列化包")),
        "duckdb" => Some(("@cap/duckdb", "官方 DuckDB 嵌入式分析型数据库")),
        "pcre2" => Some(("@cap/pcre2", "官方 PCRE2 完整正则引擎包")),
        "onnxruntime" | "onnxruntime-node" | "ort" => {
            Some(("@cap/onnxruntime", "官方 ONNX Runtime AI 推理引擎"))
        }
        "opus" => Some(("@cap/opus", "官方 Opus 音频编解码包")),
        "libsodium" | "libsodium-wrappers" | "sodiumoxide" => {
            Some(("@cap/libsodium", "官方 libsodium 现代密码学库"))
        }
        "cmark" | "marked" | "pulldown-cmark" => {
            Some(("@cap/cmark", "官方 CommonMark / Markdown 解析器"))
        }
        "sdl3" => Some(("@cap/sdl3", "官方 SDL3 多媒体与窗口系统包")),
        "freetype" => Some(("@cap/freetype", "官方 FreeType 字体光栅化包")),
        "harfbuzz" => Some(("@cap/harfbuzz", "官方 HarfBuzz 文本塑形引擎")),
        "opencv" => Some(("@cap/opencv", "官方 OpenCV 计算机视觉包")),
        "expat" => Some(("@cap/expat", "官方 Expat 流式 XML 解析包")),
        "mbedtls" => Some(("@cap/mbedtls", "官方 mbedTLS 嵌入式 TLS 库")),
        "snappy" | "snap" => Some(("@cap/snappy", "官方 Snappy 压缩算法包")),
        "xxhash" | "xxhash-rust" => Some(("@cap/xxhash", "官方 xxHash 极速非加密哈希包")),
        "curl" | "reqwest" => Some(("@cap/curl", "官方 libcurl HTTP 客户端包")),
        "hdr-histogram" | "hdrhistogram" => {
            Some(("@cap/hdr-histogram", "官方 HdrHistogram 高精度延迟直方图"))
        }
        "miniz" | "miniz-oxide" => Some(("@cap/miniz", "官方 miniz 轻量压缩库")),
        _ => None,
    };

    tier2.map(|(pkg_name, reason)| DepTier::OfficialPkg {
        pkg_name: pkg_name.to_string(),
        version: "^0.1.0".to_string(),
        reason: reason.to_string(),
    })
}

// ============================================================================
// 4. 本地依赖源码定位（供 Tier 3 递归转换独立 CP 库）
// ============================================================================

/// 查找依赖库的本地源码目录。
///
/// - TS：依次查找 `explicit_path`、`src_dir/node_modules/<dep>`、父目录 `node_modules/<dep>`、`packages/<dep>`。
/// - Rust：优先查找 `explicit_path`（相对于 `src_dir`），其次查找父目录同级 workspace crate
///   （如 `../axum-core`、`../<dep>`）、`vendor/<dep>`、`$CARGO_HOME/registry/src/*/<dep>-*`（或 `~/.cargo/registry/src/*/<dep>-*`）。
pub fn locate_local_dep_source(
    lang: SourceLang,
    src_dir: &Path,
    dep_name: &str,
    explicit_path: Option<&Path>,
) -> Option<PathBuf> {
    // 0. 显式路径最优先
    if let Some(p) = explicit_path {
        let candidate = if p.is_absolute() {
            p.to_path_buf()
        } else {
            src_dir.join(p)
        };
        if candidate.is_dir() {
            return Some(candidate);
        }
    }

    match lang {
        SourceLang::Ts => locate_ts_dep_source(src_dir, dep_name),
        SourceLang::Rs => locate_rs_dep_source(src_dir, dep_name),
    }
}

fn locate_ts_dep_source(src_dir: &Path, dep_name: &str) -> Option<PathBuf> {
    // 1. 当前项目 node_modules/<dep>
    let direct = src_dir.join("node_modules").join(dep_name);
    if direct.is_dir() {
        return Some(direct);
    }

    // 2. 向上查找父目录 node_modules/<dep>（兼容 monorepo hoisting）
    let mut ancestor = src_dir.parent();
    for _ in 0..4 {
        let Some(dir) = ancestor else { break };
        let candidate = dir.join("node_modules").join(dep_name);
        if candidate.is_dir() {
            return Some(candidate);
        }
        ancestor = dir.parent();
    }

    // 3. 当前目录或父目录 packages/<dep>（以及去掉 @scope/ 后的短名）
    let short_name = dep_name.split_once('/').map(|(_, s)| s).unwrap_or(dep_name);

    let mut pkg_bases = vec![src_dir.to_path_buf()];
    if let Some(p) = src_dir.parent() {
        pkg_bases.push(p.to_path_buf());
    }

    for base in pkg_bases {
        for sub in [dep_name, short_name] {
            let candidate = base.join("packages").join(sub);
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }

    None
}

fn locate_rs_dep_source(src_dir: &Path, dep_name: &str) -> Option<PathBuf> {
    let hyphen_name = dep_name.replace('_', "-");
    let under_name = dep_name.replace('-', "_");
    let names = if hyphen_name == under_name {
        vec![dep_name.to_string()]
    } else {
        vec![dep_name.to_string(), hyphen_name.clone(), under_name]
    };

    // 1. 父目录同级 workspace crate（如 ../axum-core、../axum-macros、../<dep>）
    if let Some(parent) = src_dir.parent() {
        for n in &names {
            let sibling = parent.join(n);
            if sibling.is_dir() && sibling != src_dir && sibling.join("Cargo.toml").is_file() {
                return Some(sibling);
            }
            let in_crates = parent.join("crates").join(n);
            if in_crates.is_dir() && in_crates.join("Cargo.toml").is_file() {
                return Some(in_crates);
            }
        }
    }

    // 2. 当前目录或父目录 vendor/<dep> / crates/<dep>
    let mut bases = vec![src_dir.to_path_buf()];
    if let Some(parent) = src_dir.parent() {
        bases.push(parent.to_path_buf());
    }
    for base in &bases {
        for n in &names {
            let vendor = base.join("vendor").join(n);
            if vendor.is_dir() {
                return Some(vendor);
            }
            let crates = base.join("crates").join(n);
            if crates.is_dir() && crates.join("Cargo.toml").is_file() {
                return Some(crates);
            }
        }
    }

    // 3. $CARGO_HOME/registry/src/*/<dep>-* 或 ~/.cargo/registry/src/*/<dep>-*
    let cargo_home = std::env::var_os("CARGO_HOME")
        .map(PathBuf::from)
        .or_else(|| std::env::var_os("HOME").map(|h| PathBuf::from(h).join(".cargo")));

    if let Some(home) = cargo_home {
        let reg_src = home.join("registry").join("src");
        if let Ok(indices) = fs::read_dir(&reg_src) {
            let mut matches: Vec<PathBuf> = Vec::new();
            let prefixes: Vec<String> = names.iter().map(|n| format!("{}-", n)).collect();

            for idx_entry in indices.flatten() {
                let idx_path = idx_entry.path();
                if !idx_path.is_dir() {
                    continue;
                }
                if let Ok(crates) = fs::read_dir(&idx_path) {
                    for crate_entry in crates.flatten() {
                        let cpath = crate_entry.path();
                        if !cpath.is_dir() {
                            continue;
                        }
                        if let Some(fname) = cpath.file_name().and_then(|s| s.to_str()) {
                            for prefix in &prefixes {
                                if let Some(rest) = fname.strip_prefix(prefix.as_str()) {
                                    // 确认连字符后以数字版本号开头，避免误匹配前缀相近的 crate
                                    if rest
                                        .chars()
                                        .next()
                                        .map(|c| c.is_ascii_digit())
                                        .unwrap_or(false)
                                    {
                                        matches.push(cpath.clone());
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if !matches.is_empty() {
                matches.sort();
                return matches.pop();
            }
        }
    }

    None
}

// ============================================================================
// 5. 导入路径改写
// ============================================================================

/// 将源码中的导入路径按三层依赖映射表改写为合法的 CP 导入路径：
/// - 相对路径（`.ts`/`.tsx`/`.js`/`.mjs`/`.rs` 等）转为 `.cp`；
/// - 第三方包名按 `dep_map` 改写为 `cap:*`、`@cap/*` 或 `cap_modules/<pkg>/src/index.cp`。
pub fn rewrite_import_source(
    lang: SourceLang,
    raw_source: &str,
    dep_map: &HashMap<String, DepTier>,
) -> String {
    let raw = raw_source.trim();
    if raw.is_empty() {
        return String::new();
    }

    // 已是 cap:* 或 @cap/* 协议直接保留
    if raw.starts_with("cap:") || raw.starts_with("@cap/") {
        return raw.to_string();
    }

    // 1. 相对路径改写为 .cp
    if raw.starts_with("./") || raw.starts_with("../") || raw.starts_with('/') {
        return rewrite_relative_path_to_cp(raw);
    }

    // Rust 模块内相对路径（crate:: / super:: / self::）
    if lang == SourceLang::Rs
        && (raw.starts_with("crate::") || raw.starts_with("super::") || raw.starts_with("self::"))
    {
        return rewrite_rust_module_path_to_cp(raw);
    }

    // 2. 第三方包名查找
    let base_pkg = extract_base_package_name(lang, raw);

    // 优先查精确路径，其次查主包名（Rust 同时兼容 _ 与 -）
    let tier = dep_map
        .get(raw)
        .or_else(|| dep_map.get(&base_pkg))
        .or_else(|| {
            if lang == SourceLang::Rs {
                dep_map
                    .get(&base_pkg.replace('_', "-"))
                    .or_else(|| dep_map.get(&base_pkg.replace('-', "_")))
            } else {
                None
            }
        })
        .cloned()
        .or_else(|| resolve_builtin_or_official(lang, raw))
        .or_else(|| resolve_builtin_or_official(lang, &base_pkg));

    match tier {
        Some(DepTier::Builtin { cap_mod, .. }) => cap_mod,
        Some(DepTier::OfficialPkg { pkg_name, .. }) => pkg_name,
        Some(DepTier::RecursiveConverted {
            pkg_name,
            out_rel_dir,
            ..
        }) => {
            let trimmed_dir = out_rel_dir.trim_matches('/');
            if trimmed_dir.is_empty() {
                pkg_name
            } else {
                format!("{}/src/index.cp", trimmed_dir)
            }
        }
        Some(DepTier::PendingManual { .. }) | None => raw.to_string(),
    }
}

fn rewrite_relative_path_to_cp(raw: &str) -> String {
    if raw.ends_with(".cp") {
        return raw.to_string();
    }
    if raw.ends_with('/') {
        return format!("{}index.cp", raw);
    }
    for ext in [
        ".tsx", ".ts", ".mts", ".cts", ".jsx", ".js", ".mjs", ".cjs", ".rs",
    ] {
        if let Some(stem) = raw.strip_suffix(ext) {
            return format!("{}.cp", stem);
        }
    }
    format!("{}.cp", raw)
}

fn rewrite_rust_module_path_to_cp(raw: &str) -> String {
    if let Some(rest) = raw.strip_prefix("crate::") {
        let parts: Vec<&str> = rest.split("::").filter(|s| !s.is_empty()).collect();
        if parts.is_empty() {
            return "./index.cp".to_string();
        }
        return format!("./{}.cp", parts.join("/"));
    }
    if let Some(rest) = raw.strip_prefix("self::") {
        let parts: Vec<&str> = rest.split("::").filter(|s| !s.is_empty()).collect();
        if parts.is_empty() {
            return "./index.cp".to_string();
        }
        return format!("./{}.cp", parts.join("/"));
    }
    if let Some(rest) = raw.strip_prefix("super::") {
        let mut up_count = 1usize;
        let mut rem = rest;
        while let Some(next) = rem.strip_prefix("super::") {
            up_count += 1;
            rem = next;
        }
        let prefix = "../".repeat(up_count);
        let parts: Vec<&str> = rem.split("::").filter(|s| !s.is_empty()).collect();
        if parts.is_empty() {
            return format!("{}index.cp", prefix);
        }
        return format!("{}{}.cp", prefix, parts.join("/"));
    }
    raw.to_string()
}

fn extract_base_package_name(lang: SourceLang, raw: &str) -> String {
    match lang {
        SourceLang::Ts => {
            if raw.starts_with("node:") || raw.starts_with("bun:") {
                return raw.to_string();
            }
            if raw.starts_with('@') {
                // @scope/pkg/subpath -> @scope/pkg
                let mut iter = raw.splitn(3, '/');
                let scope = iter.next().unwrap_or("");
                let pkg = iter.next().unwrap_or("");
                if !pkg.is_empty() {
                    return format!("{}/{}", scope, pkg);
                }
                return raw.to_string();
            }
            raw.split('/').next().unwrap_or(raw).to_string()
        }
        SourceLang::Rs => raw.split("::").next().unwrap_or(raw).to_string(),
    }
}

// ============================================================================
// 6. cap.json 生成器
// ============================================================================

/// 生成规范的、可直接 `cap build` / `cap publish` 的 `cap.json`。
pub fn generate_cap_json(summary: &ProjectMigrateSummary) -> String {
    let mut root = JsonMap::new();

    let pkg_name = sanitize_cap_pkg_name(&summary.project_name);
    let version = if summary.version.trim().is_empty() {
        "0.1.0".to_string()
    } else {
        summary.version.trim().to_string()
    };

    let main_entry = if summary
        .files
        .iter()
        .any(|f| f.cp_rel_path == "src/index.cp")
    {
        "src/index.cp"
    } else if summary.files.iter().any(|f| f.cp_rel_path == "src/main.cp") {
        "src/main.cp"
    } else if summary.files.iter().any(|f| f.cp_rel_path == "src/lib.cp") {
        "src/lib.cp"
    } else {
        "src/index.cp"
    };

    root.insert("name".to_string(), JsonValue::String(pkg_name));
    root.insert("version".to_string(), JsonValue::String(version));
    root.insert(
        "description".to_string(),
        JsonValue::String(format!(
            "Migrated from {} to CP via `cap from {}`",
            summary.lang.display_name(),
            summary.lang.short_flag()
        )),
    );
    root.insert(
        "main".to_string(),
        JsonValue::String(main_entry.to_string()),
    );
    root.insert("license".to_string(), JsonValue::String("MIT".to_string()));

    let mut deps = JsonMap::new();
    let mut dev_deps = JsonMap::new();

    for dep in &summary.deps {
        let target = if dep.is_dev { &mut dev_deps } else { &mut deps };
        match &dep.tier {
            DepTier::OfficialPkg {
                pkg_name, version, ..
            } => {
                target.insert(pkg_name.clone(), JsonValue::String(version.clone()));
            }
            DepTier::RecursiveConverted {
                pkg_name,
                out_rel_dir,
                ..
            } => {
                let rel = out_rel_dir.trim_start_matches("./");
                let spec = if rel.is_empty() {
                    "^0.1.0".to_string()
                } else {
                    format!("file:./{}", rel)
                };
                target.insert(pkg_name.clone(), JsonValue::String(spec));
            }
            DepTier::Builtin { .. } | DepTier::PendingManual { .. } => {}
        }
    }

    root.insert("dependencies".to_string(), JsonValue::Object(deps));
    if !dev_deps.is_empty() {
        root.insert("devDependencies".to_string(), JsonValue::Object(dev_deps));
    }

    serde_json::to_string_pretty(&JsonValue::Object(root)).unwrap_or_else(|_| "{}".to_string())
}

fn sanitize_cap_pkg_name(raw: &str) -> String {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return "migrated-cp-pkg".to_string();
    }
    trimmed
        .chars()
        .map(|c| {
            if c.is_ascii_alphanumeric() || matches!(c, '-' | '_' | '@' | '/') {
                c.to_ascii_lowercase()
            } else {
                '-'
            }
        })
        .collect()
}

// ============================================================================
// 7. migrate.md 中文迁移报告生成器
// ============================================================================

/// 生成详尽的中文迁移报告 `migrate.md`。
pub fn generate_migrate_md(summary: &ProjectMigrateSummary) -> String {
    let mut md = String::with_capacity(8192);

    let total_files = summary.files.len();
    let parse_ok_count = summary.files.iter().filter(|f| f.parse_ok).count();
    let parse_rate = if total_files == 0 {
        100.0
    } else {
        (parse_ok_count as f64) * 100.0 / (total_files as f64)
    };
    let total_orig_lines: usize = summary.files.iter().map(|f| f.orig_lines).sum();
    let total_cp_lines: usize = summary.files.iter().map(|f| f.cp_lines).sum();
    let total_todos: usize = summary.files.iter().map(|f| f.todo_count).sum();

    let mut tier1_deps = Vec::new();
    let mut tier2_deps = Vec::new();
    let mut tier3_deps = Vec::new();
    let mut pending_deps = Vec::new();

    for dep in &summary.deps {
        match &dep.tier {
            DepTier::Builtin { .. } => tier1_deps.push(dep),
            DepTier::OfficialPkg { .. } => tier2_deps.push(dep),
            DepTier::RecursiveConverted { .. } => tier3_deps.push(dep),
            DepTier::PendingManual { .. } => pending_deps.push(dep),
        }
    }

    // 标题
    md.push_str(&format!(
        "# `{}` 迁移报告（{} → CP）\n\n",
        summary.project_name,
        summary.lang.display_name()
    ));

    // ========================================================================
    // 一、迁移总览与统计
    // ========================================================================
    md.push_str("## 1. 迁移总览与统计\n\n");
    md.push_str("| 指标 | 数值 |\n");
    md.push_str("| --- | --- |\n");
    md.push_str(&format!("| 项目名称 | `{}` |\n", summary.project_name));
    md.push_str(&format!("| 版本号 | `{}` |\n", summary.version));
    md.push_str(&format!("| 源语言 | {} |\n", summary.lang.display_name()));
    md.push_str(&format!("| 源码目录 | `{}` |\n", summary.src_dir));
    md.push_str(&format!("| 输出目录 | `{}` |\n", summary.out_dir));
    md.push_str(&format!("| 迁移文件总数 | {} |\n", total_files));
    md.push_str(&format!(
        "| 代码行数变化 | {} 行 → {} 行 |\n",
        total_orig_lines, total_cp_lines
    ));
    md.push_str(&format!(
        "| `cpc` 语法解析通过率 | {}/{} ({:.1}%) |\n",
        parse_ok_count, total_files, parse_rate
    ));
    md.push_str(&format!(
        "| 剩余 `TODO(cap-migrate)` 标记数 | {} |\n",
        total_todos
    ));
    md.push_str(&format!(
        "| 依赖总计 | {}（Tier 1 内置: {} / Tier 2 官方包: {} / Tier 3 递归转换: {} / 待人工评估: {}） |\n\n",
        summary.deps.len(),
        tier1_deps.len(),
        tier2_deps.len(),
        tier3_deps.len(),
        pending_deps.len()
    ));

    // ========================================================================
    // 二、依赖库 3 级迁移清单
    // ========================================================================
    md.push_str("## 2. 依赖库 3 级迁移清单\n\n");

    md.push_str("### 2.1 Tier 1 — CP 内置标准库（`cap:*`，零外部依赖直连）\n\n");
    if tier1_deps.is_empty() {
        md.push_str("_无 Tier 1 内置映射依赖。_\n\n");
    } else {
        md.push_str("| 原依赖 | 原版本 | 类型 | 目标模块 | 映射说明 |\n");
        md.push_str("| --- | --- | --- | --- | --- |\n");
        for dep in &tier1_deps {
            if let DepTier::Builtin { cap_mod, reason } = &dep.tier {
                md.push_str(&format!(
                    "| `{}` | `{}` | {} | `{}` | {} |\n",
                    dep.orig_name,
                    dep.orig_version,
                    if dep.is_dev { "dev" } else { "runtime" },
                    cap_mod,
                    escape_md_cell(reason)
                ));
            }
        }
        md.push('\n');
    }

    md.push_str("### 2.2 Tier 2 — 官方生态可安装包（`@cap/*`）\n\n");
    if tier2_deps.is_empty() {
        md.push_str("_无 Tier 2 官方包依赖。_\n\n");
    } else {
        md.push_str("| 原依赖 | 原版本 | 类型 | 官方包 | 建议版本 | 说明 |\n");
        md.push_str("| --- | --- | --- | --- | --- | --- |\n");
        for dep in &tier2_deps {
            if let DepTier::OfficialPkg {
                pkg_name,
                version,
                reason,
            } = &dep.tier
            {
                md.push_str(&format!(
                    "| `{}` | `{}` | {} | `{}` | `{}` | {} |\n",
                    dep.orig_name,
                    dep.orig_version,
                    if dep.is_dev { "dev" } else { "runtime" },
                    pkg_name,
                    version,
                    escape_md_cell(reason)
                ));
            }
        }
        md.push_str(
            "\n> 以上依赖已自动写入 `cap.json`，执行 `cap install` 即可拉取预编译二进制矩阵。\n\n",
        );
    }

    md.push_str("### 2.3 Tier 3 — 递归转换的独立可发布 CP 库\n\n");
    if tier3_deps.is_empty() {
        md.push_str("_无 Tier 3 递归转换依赖。_\n\n");
    } else {
        md.push_str("| 原依赖 | 独立 CP 包名 | 输出相对路径 | 转换文件数 | 来源目录 |\n");
        md.push_str("| --- | --- | --- | --- | --- |\n");
        for dep in &tier3_deps {
            if let DepTier::RecursiveConverted {
                pkg_name,
                out_rel_dir,
                src_dir,
                file_count,
            } = &dep.tier
            {
                md.push_str(&format!(
                    "| `{}` | `{}` | `{}` | {} | `{}` |\n",
                    dep.orig_name, pkg_name, out_rel_dir, file_count, src_dir
                ));
            }
        }
        md.push_str("\n#### Tier 3 独立 CP 库发布指南\n\n");
        md.push_str("每个递归转换的依赖库均已生成独立的 `cap.json` 与 `migrate.md`，可直接独立校验并发布至 `cap` 仓库：\n\n");
        md.push_str("```bash\n");
        for dep in &tier3_deps {
            if let DepTier::RecursiveConverted {
                pkg_name,
                out_rel_dir,
                ..
            } = &dep.tier
            {
                md.push_str(&format!(
                    "# 校验并发布独立库 {}\n(cd {} && cpc check src/ && cap test && cap publish)\n",
                    pkg_name, out_rel_dir
                ));
            }
        }
        md.push_str("```\n\n");
    }

    if !summary.converted_dep_pkgs.is_empty() {
        md.push_str(&format!(
            "已汇总的递归转换子包列表：`{}`。\n\n",
            summary.converted_dep_pkgs.join("`, `")
        ));
    }

    if !pending_deps.is_empty() {
        md.push_str("### 2.4 待人工评估的依赖\n\n");
        md.push_str("| 原依赖 | 原版本 | 类型 | 原因 |\n");
        md.push_str("| --- | --- | --- | --- |\n");
        for dep in &pending_deps {
            if let DepTier::PendingManual { reason } = &dep.tier {
                md.push_str(&format!(
                    "| `{}` | `{}` | {} | {} |\n",
                    dep.orig_name,
                    dep.orig_version,
                    if dep.is_dev { "dev" } else { "runtime" },
                    escape_md_cell(reason)
                ));
            }
        }
        md.push('\n');
    }

    // ========================================================================
    // 三、逐文件迁移明细表
    // ========================================================================
    md.push_str("## 3. 逐文件迁移明细\n\n");
    if summary.files.is_empty() {
        md.push_str("_未扫描到可迁移的源文件。_\n\n");
    } else {
        md.push_str(
            "| 原文件 | 生成 CP 文件 | 行数 (原 → CP) | `cpc` 解析 | `TODO` 数 | 优化项 |\n",
        );
        md.push_str("| --- | --- | --- | --- | --- | --- |\n");
        for f in &summary.files {
            let status = if f.parse_ok {
                "✅ 通过".to_string()
            } else {
                let err_brief = f
                    .parse_err
                    .as_deref()
                    .unwrap_or("语法需人工调整")
                    .lines()
                    .next()
                    .unwrap_or("语法需人工调整");
                format!("⚠️ 需修: {}", escape_md_cell(err_brief))
            };
            let opts = if f.opt_notes.is_empty() {
                "-".to_string()
            } else {
                escape_md_cell(&f.opt_notes.join("; "))
            };
            md.push_str(&format!(
                "| `{}` | `{}` | {} → {} | {} | {} | {} |\n",
                f.orig_rel_path,
                f.cp_rel_path,
                f.orig_lines,
                f.cp_lines,
                status,
                f.todo_count,
                opts
            ));
        }
        md.push('\n');
    }

    // ========================================================================
    // 四、功能完整性、安全性增强与 CP 性能增强说明
    // ========================================================================
    md.push_str("## 4. 功能完整性、安全性增强与 CP 性能增强\n\n");
    match summary.lang {
        SourceLang::Ts => {
            md.push_str("### 4.1 安全性增强（消除动态类型与 GC 隐患）\n\n");
            md.push_str("- **消除 `any` / `var` / 隐式原型链**：全部变量收敛为块级 `let`/`const` 与静态强类型签名，编译期拦截空值与类型错配。\n");
            md.push_str("- **统一错误联合 `?T` 与可选类型 `T?`**：替代脆弱的 `try/catch` 隐式异常栈，强制显式传播（`?`）或处理错误。\n");
            md.push_str("- **确定性 RAII 内存释放**：通过编译期自动注入 `defer`/`errdefer` 与借用检查，彻底消除 GC Stop-The-World 停顿与资源句柄泄漏。\n");
            md.push_str("- **4 门禁质量保障**：支持 `cpc check` + `cap fmt` + `cap lint` + `cap test` 全链路静态校验。\n\n");
            md.push_str("### 4.2 CP 性能增强\n\n");
            md.push_str("- **原生机器码执行**：CP 编译为 Zig/LLVM 原生状态机（CPS），消除 V8/JSC JIT 预热开销与装箱分配。\n");
            md.push_str("- **内置零拷贝网络与路由**：Tier 1 依赖直连 `cap:web` / `cap:http` / `cap:crypto`（uSockets + picohttpparser + BoringSSL），内存占用显著下降。\n");
            md.push_str("- **百万级 Actor 并发**：天然兼容 BEAM 抢占式调度与零拷贝 `SharedTensor` / `move` 所有权传递。\n\n");
        }
        SourceLang::Rs => {
            md.push_str("### 4.1 安全性增强与心智负担降低\n\n");
            md.push_str("- **消除 `'a` 生命周期标注与复杂宏展开**：保留值语义、不可变默认借用与所有权安全，大幅提升代码可读性与编译速度。\n");
            md.push_str("- **错误联合 `?T` 与可选 `T?` 一体化**：将 `Result<T, E>` / `Option<T>` 平滑映射为 CP 原生错误传播体系，配合 `defer`/`errdefer` 实现确定性资源释放。\n");
            md.push_str("- **4 门禁质量保障**：全项目可直接纳入 `cpc check`、`cap fmt`、`cap lint`、`cap test` 持续集成门禁。\n\n");
            md.push_str("### 4.2 CP 性能增强\n\n");
            md.push_str("- **原生 Zig/LLVM 后端与 CPS 状态机**：无 GC 开销，异步函数自动切分为轻量状态机，低延迟高吞吐。\n");
            md.push_str("- **标准库零外部依赖直连**：`cap:web`、`cap:json`、`cap:async`、`cap:crypto` 统一共享底层高性能原生实现，显著减少依赖树体积与增量构建耗时。\n\n");
        }
    }

    // 汇总实际触发的优化项统计
    let mut opt_counts: BTreeMap<&str, usize> = BTreeMap::new();
    for f in &summary.files {
        for note in &f.opt_notes {
            *opt_counts.entry(note.as_str()).or_insert(0) += 1;
        }
    }
    if !opt_counts.is_empty() {
        md.push_str("### 4.3 本次迁移自动应用的优化项统计\n\n");
        for (note, cnt) in opt_counts {
            md.push_str(&format!("- {}（涉及 {} 个文件）\n", note, cnt));
        }
        md.push('\n');
    }

    // ========================================================================
    // 五、尚未自动完成、需人工精修的清单与定位方法
    // ========================================================================
    md.push_str("## 5. 需人工精修清单与定位方法\n\n");

    let failed_files: Vec<&FileReport> = summary.files.iter().filter(|f| !f.parse_ok).collect();
    let todo_files: Vec<&FileReport> = summary.files.iter().filter(|f| f.todo_count > 0).collect();

    if failed_files.is_empty() && todo_files.is_empty() && pending_deps.is_empty() {
        md.push_str("🎉 本次迁移所有文件均已通过 `cpc` 语法解析，且无残留 `TODO(cap-migrate)` 或未解析依赖。\n\n");
    } else {
        if !failed_files.is_empty() {
            md.push_str("### 5.1 需优先修复的 `cpc` 语法报错文件\n\n");
            for f in &failed_files {
                let err_msg = f.parse_err.as_deref().unwrap_or("未知解析错误");
                md.push_str(&format!(
                    "- `{}`（原 `{}`）：{}\n",
                    f.cp_rel_path,
                    f.orig_rel_path,
                    err_msg.lines().next().unwrap_or(err_msg)
                ));
            }
            md.push('\n');
        }

        if !todo_files.is_empty() {
            md.push_str("### 5.2 含 `TODO(cap-migrate)` 标记的文件\n\n");
            for f in &todo_files {
                md.push_str(&format!(
                    "- `{}`：{} 处 `TODO(cap-migrate)`\n",
                    f.cp_rel_path, f.todo_count
                ));
            }
            md.push('\n');
        }

        if !pending_deps.is_empty() {
            md.push_str("### 5.3 待人工评估的第三方依赖\n\n");
            for dep in &pending_deps {
                if let DepTier::PendingManual { reason } = &dep.tier {
                    md.push_str(&format!(
                        "- `{}` (`{}`)：{}\n",
                        dep.orig_name, dep.orig_version, reason
                    ));
                }
            }
            md.push('\n');
        }

        md.push_str("### 5.4 快速定位命令\n\n");
        md.push_str("```bash\n");
        md.push_str(&format!(
            "# 检索全部迁移遗留 TODO 标记\nrg -n \"TODO\\(cap-migrate\\)\" {}\n\n",
            summary.out_dir
        ));
        md.push_str(&format!(
            "# 运行 cpc 逐文件静态诊断（自带 AI-First 修复建议）\ncpc check {}/src/\n",
            summary.out_dir
        ));
        md.push_str("```\n\n");
    }

    // ========================================================================
    // 六、测试、调试、运行与正式发布操作手册
    // ========================================================================
    md.push_str("## 6. 测试、调试、运行与正式发布操作手册\n\n");
    md.push_str("```bash\n");
    md.push_str(&format!("cd {}\n\n", summary.out_dir));
    md.push_str("# 1. 安装 Tier 2 官方 @cap/* 依赖与本地链接\n");
    md.push_str("cap install\n\n");
    md.push_str("# 2. 代码格式化与静态检查（4 门禁）\n");
    md.push_str("cap fmt\n");
    md.push_str("cap lint\n");
    md.push_str("cpc check src/\n\n");
    md.push_str("# 3. 运行单元测试与本地调试\n");
    md.push_str("cap test\n");
    md.push_str("cap run\n\n");
    md.push_str("# 4. 构建跨平台原生二进制产物\n");
    md.push_str("cap build\n\n");
    md.push_str("# 5. 打包与正式发布到 cap 包仓库\n");
    if !tier3_deps.is_empty() {
        md.push_str("# 注意：请先按第 2.3 节顺序发布 Tier 3 独立依赖库，再将本包 cap.json 中的 file: 依赖替换为正式版本号\n");
    }
    md.push_str("cap pack\n");
    md.push_str("cap publish\n");
    md.push_str("```\n");

    md
}

fn escape_md_cell(s: &str) -> String {
    s.replace('|', "\\|").replace('\n', " ").replace('\r', "")
}

// ============================================================================
// 单元测试
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_resolve_tier1_ts_and_rs() {
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Ts, "memoirist"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:web"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Ts, "@sinclair/typebox"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:web"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Ts, "better-sqlite3"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:sqlite"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "matchit"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:web"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "cookie"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:http"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "serde_json"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:json"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "form_urlencoded"),
            Some(DepTier::Builtin { cap_mod, .. }) if cap_mod == "cap:url"
        ));
    }

    #[test]
    fn test_resolve_tier2_official() {
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Ts, "js-yaml"),
            Some(DepTier::OfficialPkg { pkg_name, .. }) if pkg_name == "@cap/yaml"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "pulldown-cmark"),
            Some(DepTier::OfficialPkg { pkg_name, .. }) if pkg_name == "@cap/cmark"
        ));
        assert!(matches!(
            resolve_builtin_or_official(SourceLang::Rs, "reqwest"),
            Some(DepTier::OfficialPkg { pkg_name, .. }) if pkg_name == "@cap/curl"
        ));
    }

    #[test]
    fn test_parse_cargo_toml_str() {
        let sample = r#"
[package]
name = "axum"
version.workspace = true

[dependencies]
http = "1.0"
axum-core = { path = "../axum-core", version = "0.4" }
tower-http = {
    version = "0.5",
    features = [
        "cors",
        "trace",
    ]
}
bytes.workspace = true

[dev-dependencies]
reqwest = "0.12"
"#;
        let parsed = parse_cargo_toml_str(sample);
        assert_eq!(parsed.pkg_name.as_deref(), Some("axum"));
        assert!(parsed.pkg_version_workspace);
        assert_eq!(parsed.deps["http"].version.as_deref(), Some("1.0"));
        assert!(!parsed.deps["http"].is_dev);
        assert_eq!(
            parsed.deps["axum-core"].path,
            Some(PathBuf::from("../axum-core"))
        );
        assert_eq!(parsed.deps["tower-http"].version.as_deref(), Some("0.5"));
        assert!(parsed.deps["bytes"].workspace);
        assert!(parsed.deps["reqwest"].is_dev);
    }

    #[test]
    fn test_rewrite_import_source() {
        let mut dep_map = HashMap::new();
        dep_map.insert(
            "memoirist".to_string(),
            DepTier::Builtin {
                cap_mod: "cap:web".to_string(),
                reason: "builtin".to_string(),
            },
        );
        dep_map.insert(
            "my-helper".to_string(),
            DepTier::RecursiveConverted {
                pkg_name: "my-helper".to_string(),
                out_rel_dir: "cap_modules/my-helper".to_string(),
                src_dir: "/tmp/my-helper".to_string(),
                file_count: 3,
            },
        );

        assert_eq!(
            rewrite_import_source(SourceLang::Ts, "./router.ts", &dep_map),
            "./router.cp"
        );
        assert_eq!(
            rewrite_import_source(SourceLang::Ts, "../utils/parse", &dep_map),
            "../utils/parse.cp"
        );
        assert_eq!(
            rewrite_import_source(SourceLang::Ts, "memoirist", &dep_map),
            "cap:web"
        );
        assert_eq!(
            rewrite_import_source(SourceLang::Ts, "my-helper/sub", &dep_map),
            "cap_modules/my-helper/src/index.cp"
        );
        assert_eq!(
            rewrite_import_source(SourceLang::Rs, "crate::routing::get", &dep_map),
            "./routing/get.cp"
        );
        assert_eq!(
            rewrite_import_source(SourceLang::Rs, "serde_json::Value", &dep_map),
            "cap:json"
        );
    }
}
