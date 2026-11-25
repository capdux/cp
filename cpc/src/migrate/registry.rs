//! `cap from -npm <pkg[@ver]>` & `cap from -crate <name[@ver]>`
//! Ecosystem Auto-Absorption & Deduplication Registry Engine.
//!
//! Responsibilities:
//! 1. Query local (and optionally remote GitHub raw) `packages/@cap/registry.json` to prevent
//!    duplicate conversions and notify users when an equivalent `cap:*` stdlib or `@cap/*` package
//!    already exists.
//! 2. Fetch package metadata from npm (`https://registry.npmjs.org/<pkg>`) or crates.io
//!    (`https://crates.io/api/v1/crates/<name>`), enforcing permissive license compliance
//!    (MIT/Apache-2.0/BSD/ISC/Zlib/CC0/Unlicense; rejecting GPL/AGPL/LGPL/SSPL).
//! 3. Recursively resolve runtime `dependencies` ONLY (skipping `devDependencies`), emitting each
//!    dependency as its own standalone `@cap/<dep>` package (never merging into the parent package).
//! 4. Download/extract source, transpile via `run_migrate` (with optional `--verify`), emit
//!    `cap.json`, and persist new mappings into `packages/@cap/registry.json`.

use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

use serde::{Deserialize, Serialize};

use super::deps::SourceLang;
use super::{run_migrate, MigrateOptions};

pub const DEFAULT_GITHUB_REGISTRY_URL: &str =
    "https://raw.githubusercontent.com/cp-lang/cp/main/packages/@cap/registry.json";

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PackageEcosystem {
    Npm,
    Crate,
}

impl PackageEcosystem {
    pub fn as_str(&self) -> &'static str {
        match self {
            PackageEcosystem::Npm => "npm",
            PackageEcosystem::Crate => "crates.io",
        }
    }

    pub fn source_lang(&self) -> SourceLang {
        match self {
            PackageEcosystem::Npm => SourceLang::Ts,
            PackageEcosystem::Crate => SourceLang::Rs,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegistryEntry {
    pub source_registry: String,
    pub source_name: String,
    #[serde(default)]
    pub source_repo: String,
    #[serde(default)]
    pub license: String,
    pub cp_target: String,
    /// `"stdlib"` for built-in `cap:*` modules, `"package"` for `@cap/*` installable packages.
    pub kind: String,
    #[serde(default)]
    pub aliases: Vec<String>,
    #[serde(default)]
    pub absorbed_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegistryIndex {
    #[serde(default = "default_version")]
    pub version: String,
    #[serde(default)]
    pub description: String,
    #[serde(default = "default_github_url")]
    pub github_raw_url: String,
    #[serde(default)]
    pub updated_at: String,
    #[serde(default)]
    pub entries: Vec<RegistryEntry>,
}

fn default_version() -> String {
    "1.0.0".to_string()
}

fn default_github_url() -> String {
    DEFAULT_GITHUB_REGISTRY_URL.to_string()
}

impl Default for RegistryIndex {
    fn default() -> Self {
        Self {
            version: default_version(),
            description: "Global deduplication & mapping registry for CP stdlib (cap:*) and @cap/* packages.".to_string(),
            github_raw_url: default_github_url(),
            updated_at: String::new(),
            entries: Vec::new(),
        }
    }
}

impl RegistryIndex {
    pub fn load_from_file(path: &Path) -> Result<Self, String> {
        if !path.exists() {
            return Ok(Self::default());
        }
        let text = fs::read_to_string(path)
            .map_err(|e| format!("failed to read registry '{}': {}", path.display(), e))?;
        serde_json::from_str::<Self>(&text)
            .map_err(|e| format!("failed to parse registry '{}': {}", path.display(), e))
    }

    pub fn save_to_file(&self, path: &Path) -> Result<(), String> {
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent)
                .map_err(|e| format!("failed to create dir '{}': {}", parent.display(), e))?;
        }
        let pretty = serde_json::to_string_pretty(self)
            .map_err(|e| format!("failed to serialize registry: {}", e))?;
        fs::write(path, format!("{}\n", pretty))
            .map_err(|e| format!("failed to write registry '{}': {}", path.display(), e))
    }

    /// Look up an upstream package by exact `source_name` or any registered `alias` (case-insensitive).
    pub fn lookup(&self, pkg_name: &str) -> Option<&RegistryEntry> {
        let needle = pkg_name.trim().to_ascii_lowercase();
        for entry in &self.entries {
            if entry.source_name.to_ascii_lowercase() == needle {
                return Some(entry);
            }
            if entry
                .aliases
                .iter()
                .any(|a| a.to_ascii_lowercase() == needle)
            {
                return Some(entry);
            }
        }
        None
    }

    /// Insert or update an entry deduplicated by `(source_registry, source_name)`.
    pub fn upsert(&mut self, entry: RegistryEntry) {
        let key_reg = entry.source_registry.to_ascii_lowercase();
        let key_name = entry.source_name.to_ascii_lowercase();
        if let Some(existing) = self.entries.iter_mut().find(|e| {
            e.source_registry.to_ascii_lowercase() == key_reg
                && e.source_name.to_ascii_lowercase() == key_name
        }) {
            *existing = entry;
        } else {
            self.entries.push(entry);
        }
    }
}

/// Parse `<name>[@<version>]` supporting scoped npm names like `@scope/pkg@1.2.3`.
pub fn parse_pkg_spec(spec: &str) -> (String, Option<String>) {
    let trimmed = spec.trim();
    if let Some(rest) = trimmed.strip_prefix('@') {
        if let Some(at_idx) = rest.find('@') {
            let name = trimmed[..at_idx + 1].to_string();
            let ver = rest[at_idx + 1..].trim();
            return (
                name,
                if ver.is_empty() {
                    None
                } else {
                    Some(ver.to_string())
                },
            );
        }
        return (trimmed.to_string(), None);
    }
    if let Some(at_idx) = trimmed.rfind('@') {
        let name = trimmed[..at_idx].trim().to_string();
        let ver = trimmed[at_idx + 1..].trim();
        return (
            name,
            if ver.is_empty() {
                None
            } else {
                Some(ver.to_string())
            },
        );
    }
    (trimmed.to_string(), None)
}

/// Sanitize an upstream package name (e.g. `@scope/my-pkg` or `serde_qs`) into a clean `@cap/<slug>` name.
pub fn normalize_cap_pkg_slug(pkg_name: &str) -> String {
    let mut out = String::new();
    let base = pkg_name
        .trim()
        .strip_prefix('@')
        .unwrap_or(pkg_name.trim());
    let mut prev_dash = false;
    for ch in base.chars() {
        if ch.is_ascii_alphanumeric() {
            out.push(ch.to_ascii_lowercase());
            prev_dash = false;
        } else if !prev_dash && !out.is_empty() {
            out.push('-');
            prev_dash = true;
        }
    }
    while out.ends_with('-') {
        out.pop();
    }
    if out.is_empty() {
        "pkg".to_string()
    } else {
        out
    }
}

/// License compliance gate: permits MIT/Apache-2.0/BSD/ISC/Zlib/CC0/Unlicense and rejects copyleft (GPL/AGPL/LGPL/SSPL).
pub fn is_permissive_license(license: &str) -> bool {
    let up = license.trim().to_ascii_uppercase();
    if up.is_empty() {
        return false;
    }
    for forbidden in ["AGPL", "GPL", "LGPL", "SSPL", "BSL"] {
        if up.contains(forbidden) {
            // Allow dual-licensed packages such as "MIT OR GPL-3.0" only if MIT/Apache-2.0 is explicitly offered via OR.
            if up.contains(" OR ")
                && (up.contains("MIT") || up.contains("APACHE-2.0") || up.contains("BSD"))
            {
                return true;
            }
            return false;
        }
    }
    for allowed in [
        "MIT",
        "APACHE-2.0",
        "APACHE 2.0",
        "BSD-2-CLAUSE",
        "BSD-3-CLAUSE",
        "BSD",
        "ISC",
        "ZLIB",
        "CC0-1.0",
        "UNLICENSE",
        "0BSD",
        "MPL-2.0",
    ] {
        if up.contains(allowed) {
            return true;
        }
    }
    false
}

#[derive(Debug, Clone)]
pub struct FetchedPkgMeta {
    pub name: String,
    pub version: String,
    pub license: String,
    pub repo_url: String,
    pub tarball_url: String,
    /// Runtime dependencies ONLY (excludes devDependencies).
    pub runtime_deps: BTreeMap<String, String>,
}

fn curl_get_json(url: &str) -> Result<serde_json::Value, String> {
    let out = Command::new("curl")
        .args([
            "-fsSL",
            "--max-time",
            "20",
            "-H",
            "User-Agent: cap-from-migrator/1.0 (cp-lang)",
            url,
        ])
        .output()
        .map_err(|e| format!("failed to spawn curl for '{}': {}", url, e))?;
    if !out.status.success() {
        let stderr = String::from_utf8_lossy(&out.stderr);
        return Err(format!("HTTP fetch failed for '{}': {}", url, stderr.trim()));
    }
    serde_json::from_slice::<serde_json::Value>(&out.stdout)
        .map_err(|e| format!("invalid JSON from '{}': {}", url, e))
}

fn clean_repo_url(raw: &str) -> String {
    raw.trim()
        .trim_start_matches("git+")
        .trim_end_matches(".git")
        .replace("git://", "https://")
        .replace("ssh://git@github.com/", "https://github.com/")
}

pub fn fetch_npm_metadata(name: &str, version_opt: Option<&str>) -> Result<FetchedPkgMeta, String> {
    let encoded_name = name.replace('/', "%2F");
    let url = format!("https://registry.npmjs.org/{}", encoded_name);
    let doc = curl_get_json(&url)?;

    let version = if let Some(v) = version_opt {
        v.to_string()
    } else {
        doc.get("dist-tags")
            .and_then(|d| d.get("latest"))
            .and_then(|v| v.as_str())
            .ok_or_else(|| format!("npm package '{}' missing dist-tags.latest", name))?
            .to_string()
    };

    let ver_obj = doc
        .get("versions")
        .and_then(|vs| vs.get(&version))
        .ok_or_else(|| format!("npm package '{}@{}' not found", name, version))?;

    let license = ver_obj
        .get("license")
        .and_then(|v| {
            v.as_str()
                .map(|s| s.to_string())
                .or_else(|| v.get("type").and_then(|t| t.as_str()).map(|s| s.to_string()))
        })
        .unwrap_or_default();

    let repo_url = ver_obj
        .get("repository")
        .and_then(|r| {
            r.as_str()
                .map(|s| s.to_string())
                .or_else(|| r.get("url").and_then(|u| u.as_str()).map(|s| s.to_string()))
        })
        .map(|s| clean_repo_url(&s))
        .unwrap_or_default();

    let tarball_url = ver_obj
        .get("dist")
        .and_then(|d| d.get("tarball"))
        .and_then(|u| u.as_str())
        .unwrap_or_default()
        .to_string();

    let mut runtime_deps = BTreeMap::new();
    if let Some(deps) = ver_obj.get("dependencies").and_then(|d| d.as_object()) {
        for (k, v) in deps {
            runtime_deps.insert(k.clone(), v.as_str().unwrap_or("*").to_string());
        }
    }

    Ok(FetchedPkgMeta {
        name: name.to_string(),
        version,
        license,
        repo_url,
        tarball_url,
        runtime_deps,
    })
}

pub fn fetch_crate_metadata(name: &str, version_opt: Option<&str>) -> Result<FetchedPkgMeta, String> {
    let url = format!("https://crates.io/api/v1/crates/{}", name);
    let doc = curl_get_json(&url)?;

    let crate_obj = doc
        .get("crate")
        .ok_or_else(|| format!("crates.io response missing 'crate' for '{}'", name))?;

    let version = if let Some(v) = version_opt {
        v.to_string()
    } else {
        crate_obj
            .get("max_stable_version")
            .or_else(|| crate_obj.get("newest_version"))
            .and_then(|v| v.as_str())
            .ok_or_else(|| format!("crate '{}' has no published version", name))?
            .to_string()
    };

    let repo_url = crate_obj
        .get("repository")
        .and_then(|v| v.as_str())
        .map(clean_repo_url)
        .unwrap_or_default();

    let mut license = String::new();
    if let Some(versions) = doc.get("versions").and_then(|v| v.as_array()) {
        if let Some(vobj) = versions
            .iter()
            .find(|v| v.get("num").and_then(|n| n.as_str()) == Some(version.as_str()))
        {
            if let Some(lic) = vobj.get("license").and_then(|l| l.as_str()) {
                license = lic.to_string();
            }
        }
    }

    let tarball_url = format!(
        "https://static.crates.io/crates/{}/{}-{}.crate",
        name, name, version
    );

    let deps_url = format!(
        "https://crates.io/api/v1/crates/{}/{}/dependencies",
        name, version
    );
    let mut runtime_deps = BTreeMap::new();
    if let Ok(deps_doc) = curl_get_json(&deps_url) {
        if let Some(arr) = deps_doc.get("dependencies").and_then(|d| d.as_array()) {
            for dep in arr {
                let kind = dep.get("kind").and_then(|k| k.as_str()).unwrap_or("normal");
                let optional = dep
                    .get("optional")
                    .and_then(|o| o.as_bool())
                    .unwrap_or(false);
                if kind == "normal" && !optional {
                    if let Some(cid) = dep.get("crate_id").and_then(|c| c.as_str()) {
                        let req = dep.get("req").and_then(|r| r.as_str()).unwrap_or("*");
                        runtime_deps.insert(cid.to_string(), req.to_string());
                    }
                }
            }
        }
    }

    Ok(FetchedPkgMeta {
        name: name.to_string(),
        version,
        license,
        repo_url,
        tarball_url,
        runtime_deps,
    })
}

pub struct RegistryImportOptions {
    pub ecosystem: PackageEcosystem,
    pub spec: String,
    pub registry_path: PathBuf,
    pub packages_root: PathBuf,
    pub max_depth: usize,
    pub verify: bool,
    pub no_deps: bool,
}

#[derive(Debug, Clone)]
pub struct RegistryImportOutcome {
    pub already_existed: bool,
    pub cp_target: String,
    pub kind: String,
    pub converted_packages: Vec<String>,
}

pub fn locate_default_registry_path() -> PathBuf {
    let candidates = [
        PathBuf::from("packages/@cap/registry.json"),
        PathBuf::from("/data/dev/cp/packages/@cap/registry.json"),
    ];
    for c in &candidates {
        if c.exists() {
            return c.clone();
        }
    }
    candidates[0].clone()
}

pub fn run_registry_import(opts: RegistryImportOptions) -> Result<RegistryImportOutcome, String> {
    let mut index = RegistryIndex::load_from_file(&opts.registry_path)?;
    let (pkg_name, version_opt) = parse_pkg_spec(&opts.spec);

    // Step 1: Check global registry first to avoid duplicate conversion & notify user of existing stdlib/package.
    if let Some(existing) = index.lookup(&pkg_name) {
        return Ok(RegistryImportOutcome {
            already_existed: true,
            cp_target: existing.cp_target.clone(),
            kind: existing.kind.clone(),
            converted_packages: Vec::new(),
        });
    }

    let mut visited: HashSet<String> = HashSet::new();
    let mut converted: Vec<String> = Vec::new();
    let cp_target = convert_recursive(
        opts.ecosystem,
        &pkg_name,
        version_opt.as_deref(),
        &opts.packages_root,
        &mut index,
        &mut visited,
        &mut converted,
        0,
        if opts.no_deps { 0 } else { opts.max_depth },
        opts.verify,
    )?;

    index.save_to_file(&opts.registry_path)?;

    Ok(RegistryImportOutcome {
        already_existed: false,
        cp_target,
        kind: "package".to_string(),
        converted_packages: converted,
    })
}

#[allow(clippy::too_many_arguments)]
fn convert_recursive(
    ecosystem: PackageEcosystem,
    pkg_name: &str,
    version_opt: Option<&str>,
    packages_root: &Path,
    index: &mut RegistryIndex,
    visited: &mut HashSet<String>,
    converted: &mut Vec<String>,
    depth: usize,
    max_depth: usize,
    verify: bool,
) -> Result<String, String> {
    if let Some(existing) = index.lookup(pkg_name) {
        return Ok(existing.cp_target.clone());
    }

    let slug = normalize_cap_pkg_slug(pkg_name);
    let cp_target = format!("@cap/{}", slug);
    let visit_key = format!("{}:{}", ecosystem.as_str(), pkg_name.to_ascii_lowercase());
    if !visited.insert(visit_key) {
        return Ok(cp_target);
    }

    // Step 2: Fetch registry metadata & verify license compliance.
    let meta = match ecosystem {
        PackageEcosystem::Npm => fetch_npm_metadata(pkg_name, version_opt)?,
        PackageEcosystem::Crate => fetch_crate_metadata(pkg_name, version_opt)?,
    };

    if !is_permissive_license(&meta.license) {
        return Err(format!(
            "License compliance gate rejected '{}@{}' (license='{}'). Only permissive licenses (MIT/Apache-2.0/BSD/ISC/Zlib/CC0/Unlicense) are permitted.",
            meta.name, meta.version, meta.license
        ));
    }

    // Step 3: Recursively convert runtime dependencies as independent @cap/<dep> packages (never merged).
    let mut dep_targets: BTreeMap<String, String> = BTreeMap::new();
    if depth < max_depth {
        for dep_name in meta.runtime_deps.keys() {
            if let Some(existing) = index.lookup(dep_name) {
                dep_targets.insert(existing.cp_target.clone(), "^1.0.0".to_string());
            } else {
                let dep_target = convert_recursive(
                    ecosystem,
                    dep_name,
                    None,
                    packages_root,
                    index,
                    visited,
                    converted,
                    depth + 1,
                    max_depth,
                    verify,
                )?;
                dep_targets.insert(dep_target, "^1.0.0".to_string());
            }
        }
    }

    // Step 4: Download & extract tarball into isolated temp dir and invoke `run_migrate`.
    let tmp_dir = std::env::temp_dir().join(format!("cap_from_{}_{}", slug, meta.version));
    let _ = fs::remove_dir_all(&tmp_dir);
    fs::create_dir_all(&tmp_dir)
        .map_err(|e| format!("failed to create temp dir '{}': {}", tmp_dir.display(), e))?;

    let tarball_path = tmp_dir.join("pkg.tgz");
    let dl_status = Command::new("curl")
        .args([
            "-fsSL",
            "--max-time",
            "30",
            "-o",
            tarball_path.to_str().unwrap_or("pkg.tgz"),
            &meta.tarball_url,
        ])
        .status()
        .map_err(|e| format!("failed to download '{}': {}", meta.tarball_url, e))?;
    if !dl_status.success() {
        return Err(format!(
            "failed to download package tarball from '{}'",
            meta.tarball_url
        ));
    }

    let extract_dir = tmp_dir.join("src");
    fs::create_dir_all(&extract_dir).map_err(|e| e.to_string())?;
    let tar_status = Command::new("tar")
        .args([
            "-xzf",
            tarball_path.to_str().unwrap_or("pkg.tgz"),
            "-C",
            extract_dir.to_str().unwrap_or("src"),
            "--strip-components=1",
        ])
        .status()
        .map_err(|e| format!("failed to extract tarball: {}", e))?;
    if !tar_status.success() {
        return Err(format!("tar extraction failed for '{}'", meta.name));
    }

    let out_dir = packages_root.join(&slug);
    let _summary = run_migrate(MigrateOptions {
        lang: ecosystem.source_lang(),
        src_dir: extract_dir,
        out_dir: out_dir.clone(),
        recursive_deps: false,
        max_depth: 0,
        verify,
    })?;

    // Emit standardized @cap/<slug>/cap.json with independent runtime deps
    let mut cap_json = serde_json::json!({
        "name": cp_target,
        "ver": meta.version,
        "description": format!("Absorbed from {} '{}' ({})", ecosystem.as_str(), meta.name, meta.repo_url),
        "main": "src/index.cp",
        "license": "MIT",
        "upstream": {
            "registry": ecosystem.as_str(),
            "name": meta.name,
            "version": meta.version,
            "repository": meta.repo_url,
            "license": meta.license
        }
    });
    if !dep_targets.is_empty() {
        cap_json["dep"] = serde_json::json!(dep_targets);
    }
    let _ = fs::write(
        out_dir.join("cap.json"),
        format!(
            "{}\n",
            serde_json::to_string_pretty(&cap_json).unwrap_or_default()
        ),
    );

    let _ = fs::remove_dir_all(&tmp_dir);

    index.upsert(RegistryEntry {
        source_registry: ecosystem.as_str().to_string(),
        source_name: meta.name.clone(),
        source_repo: meta.repo_url.clone(),
        license: meta.license.clone(),
        cp_target: cp_target.clone(),
        kind: "package".to_string(),
        aliases: Vec::new(),
        absorbed_at: "2026-11-26".to_string(),
    });
    converted.push(cp_target.clone());

    Ok(cp_target)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_pkg_spec_plain_and_scoped() {
        assert_eq!(parse_pkg_spec("lodash"), ("lodash".to_string(), None));
        assert_eq!(
            parse_pkg_spec("ms@2.1.3"),
            ("ms".to_string(), Some("2.1.3".to_string()))
        );
        assert_eq!(
            parse_pkg_spec("@types/node@20.11.0"),
            ("@types/node".to_string(), Some("20.11.0".to_string()))
        );
        assert_eq!(
            parse_pkg_spec("@msgpack/msgpack"),
            ("@msgpack/msgpack".to_string(), None)
        );
    }

    #[test]
    fn test_normalize_cap_pkg_slug() {
        assert_eq!(normalize_cap_pkg_slug("lru-cache"), "lru-cache");
        assert_eq!(normalize_cap_pkg_slug("serde_qs"), "serde-qs");
        assert_eq!(normalize_cap_pkg_slug("@msgpack/msgpack"), "msgpack-msgpack");
    }

    #[test]
    fn test_permissive_license_gate() {
        assert!(is_permissive_license("MIT"));
        assert!(is_permissive_license("Apache-2.0"));
        assert!(is_permissive_license("MIT OR Apache-2.0"));
        assert!(is_permissive_license("BSD-3-Clause"));
        assert!(is_permissive_license("ISC"));
        assert!(!is_permissive_license("GPL-3.0"));
        assert!(!is_permissive_license("AGPL-3.0-only"));
        assert!(!is_permissive_license("LGPL-2.1"));
        assert!(!is_permissive_license(""));
        assert!(is_permissive_license("MIT OR GPL-3.0"));
    }

    #[test]
    fn test_registry_lookup_dedup_and_alias() {
        let mut idx = RegistryIndex::default();
        idx.upsert(RegistryEntry {
            source_registry: "npm".to_string(),
            source_name: "lru-cache".to_string(),
            source_repo: "https://github.com/isaacs/node-lru-cache".to_string(),
            license: "MIT".to_string(),
            cp_target: "@cap/lru".to_string(),
            kind: "package".to_string(),
            aliases: vec!["lru".to_string(), "quick-lru".to_string()],
            absorbed_at: "2026-11-26".to_string(),
        });
        assert_eq!(idx.lookup("lru-cache").unwrap().cp_target, "@cap/lru");
        assert_eq!(idx.lookup("quick-lru").unwrap().cp_target, "@cap/lru");
        assert!(idx.lookup("nonexistent-pkg").is_none());

        // Upsert updates instead of duplicating
        idx.upsert(RegistryEntry {
            source_registry: "npm".to_string(),
            source_name: "lru-cache".to_string(),
            source_repo: "https://github.com/isaacs/node-lru-cache".to_string(),
            license: "MIT".to_string(),
            cp_target: "@cap/lru".to_string(),
            kind: "package".to_string(),
            aliases: vec!["lru".to_string()],
            absorbed_at: "2026-11-26".to_string(),
        });
        assert_eq!(idx.entries.len(), 1);
    }
}
