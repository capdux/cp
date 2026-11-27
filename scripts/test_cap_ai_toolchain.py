#!/usr/bin/env python3
"""
End-to-end automated regression test suite for CP AI-First CLI toolchain:
  - `cap fix`   (dry-run, @cap/* legacy rewrite, native @cap/* preservation,
                 bare package protection, alias/upstream mapping, cap.json
                 dependencies/devDependencies dedup & stdlib pruning, non-import
                 literal preservation, idempotency)
  - `cap brief` (bare packages, native @cap/*, @web/*, built-in cap:* stdlib,
                 missing-package error handling)
  - `cap check` (valid .cp file, syntax error with AI-First diagnostics,
                 type/borrow error, directory summary mode)

Usage:
  python3 scripts/test_cap_ai_toolchain.py
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent
CAP_BIN = REPO_ROOT / "cap" / "bin" / "cap"
SANDBOX_DIR = REPO_ROOT / "temp" / "test_ai_toolchain"


class TestRunner:
    def __init__(self) -> None:
        self.passed: List[str] = []
        self.failed: List[Tuple[str, str]] = []

    def check(self, name: str, condition: bool, detail: str = "") -> None:
        if condition:
            self.passed.append(name)
            print(f"  [PASS] {name}")
        else:
            self.failed.append((name, detail))
            print(f"  [FAIL] {name}" + (f" :: {detail}" if detail else ""))


def run_cap(args: List[str], cwd: Path = REPO_ROOT) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(CAP_BIN), *args],
        cwd=str(cwd),
        capture_output=True,
        text=True,
        timeout=60,
    )


def test_cap_fix(t: TestRunner) -> None:
    print("\n=== 1. Testing `cap fix` ===")
    fix_dir = SANDBOX_DIR / "fix_case"
    src_dir = fix_dir / "src"
    src_dir.mkdir(parents=True, exist_ok=True)

    cp_file = src_dir / "app.cp"
    orig_cp = """\
import { print } from "cap:io";
import { helper } from "./local";
import { shared } from "../shared";
import { LruCache } from "@cap/lru-cache";
import { parseCookie } from "@cap/cookie";
import { parseToml } from "@cap/toml";
import { xxh64 } from "@cap/xxhash";
import { isEmail } from "validator";
import { parseSemver } from "semver";
import { parseMs } from "ms";
import { fromStr } from "serde_json";
import { validate } from "validator-rs";
import { render } from "handlebars-rust";
import { decodeCbor } from "ciborium";

fn main() {
    let s = "serde_json";
    let legacy = "@cap/lru-cache";
    let alias_lit = "validator-rs";
    let other = "ciborium";
    print(s);
}
"""
    cp_file.write_text(orig_cp, encoding="utf-8")

    cap_json_file = fix_dir / "cap.json"
    orig_cap_json = """\
{
  "name": "ai-toolchain-fixture",
  "version": "0.1.0",
  "dependencies": {
    "@cap/lru-cache": "^1.0.0",
    "lru-cache": "^1.0.0",
    "@cap/cookie": "^1.0.0",
    "@cap/toml": "^1.0.0",
    "@cap/xxhash": "^0.8.3",
    "validator": "^1.0.0",
    "semver": "^1.0.0",
    "ms": "^1.0.0",
    "validator-rs": "^1.0.0",
    "zod": "^1.0.0",
    "serde_json": "^1.0.0"
  },
  "devDependencies": {
    "handlebars-rust": "^1.0.0",
    "ciborium": "^0.2.0"
  }
}
"""
    cap_json_file.write_text(orig_cap_json, encoding="utf-8")

    # 1a. --dry-run must report fixes without modifying any file
    rel_fix_dir = os.path.relpath(fix_dir, REPO_ROOT)
    dry_res = run_cap(["fix", "--dry-run", rel_fix_dir])
    t.check(
        "cap fix --dry-run exits 0",
        dry_res.returncode == 0,
        f"rc={dry_res.returncode}, stderr={dry_res.stderr!r}",
    )
    t.check(
        "cap fix --dry-run reports dry-run summary with non-zero fixes",
        "(dry-run)" in dry_res.stdout and "would be fixed" in dry_res.stdout,
        f"stdout={dry_res.stdout!r}",
    )
    # Expected fixes: 7 imports in app.cp + 9 dep rewrites/drops/dedups in cap.json = 16
    m_dry = re.search(r"cap fix:\s*(\d+)\s+fix\(es\)\s+across\s+(\d+)\s+file\(s\)\s+\(dry-run\)", dry_res.stdout)
    t.check(
        "cap fix --dry-run reports exact file & fix counts (16 fixes across 2 files)",
        m_dry is not None and int(m_dry.group(1)) == 16 and int(m_dry.group(2)) == 2,
        f"stdout={dry_res.stdout!r}",
    )
    t.check(
        "cap fix --dry-run does not modify app.cp",
        cp_file.read_text(encoding="utf-8") == orig_cp,
        "app.cp was modified during --dry-run",
    )
    t.check(
        "cap fix --dry-run does not modify cap.json",
        cap_json_file.read_text(encoding="utf-8") == orig_cap_json,
        "cap.json was modified during --dry-run",
    )

    # 1b. Real run of `cap fix`
    real_res = run_cap(["fix", rel_fix_dir])
    t.check(
        "cap fix exits 0",
        real_res.returncode == 0,
        f"rc={real_res.returncode}, stderr={real_res.stderr!r}",
    )
    m_real = re.search(r"cap fix:\s*(\d+)\s+fix\(es\)\s+across\s+(\d+)\s+file\(s\)\.", real_res.stdout)
    t.check(
        "cap fix reports matching fix count (16 fixes across 2 files)",
        m_real is not None and int(m_real.group(1)) == 16 and int(m_real.group(2)) == 2,
        f"stdout={real_res.stdout!r}",
    )

    fixed_cp = cp_file.read_text(encoding="utf-8")

    # Legacy @cap/<bare> rewrites
    t.check(
        'legacy "@cap/lru-cache" -> "lru-cache"',
        'import { LruCache } from "lru-cache";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'legacy "@cap/cookie" -> "cookie"',
        'import { parseCookie } from "cookie";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'legacy "@cap/toml" -> "cap:*" stdlib',
        re.search(r'import \{ parseToml \} from "cap:[a-z0-9_]+";', fixed_cp) is not None,
        fixed_cp,
    )

    # Native C/C++ @cap/* preserved
    t.check(
        'native "@cap/xxhash" preserved untouched',
        'import { xxh64 } from "@cap/xxhash";' in fixed_cp,
        fixed_cp,
    )

    # Standalone bare packages preserved (not hijacked by other packages' aliases)
    t.check(
        'standalone bare "validator" preserved untouched',
        'import { isEmail } from "validator";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'standalone bare "semver" preserved untouched',
        'import { parseSemver } from "semver";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'standalone bare "ms" preserved untouched',
        'import { parseMs } from "ms";' in fixed_cp,
        fixed_cp,
    )

    # Pure aliases / upstream crate/npm names rewritten
    t.check(
        'upstream "serde_json" -> "cap:serde"',
        'import { fromStr } from "cap:serde";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'alias "validator-rs" -> "zod"',
        'import { validate } from "zod";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'alias "handlebars-rust" -> "handlebars"',
        'import { render } from "handlebars";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'upstream "ciborium" -> "cap:cbor"',
        'import { decodeCbor } from "cap:cbor";' in fixed_cp,
        fixed_cp,
    )

    # Relative paths, canonical cap:*, and non-import string literals preserved
    t.check(
        'canonical "cap:io" preserved untouched',
        'import { print } from "cap:io";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        'relative "./local" & "../shared" preserved untouched',
        'import { helper } from "./local";' in fixed_cp
        and 'import { shared } from "../shared";' in fixed_cp,
        fixed_cp,
    )
    t.check(
        "non-import string literals preserved untouched",
        'let s = "serde_json";' in fixed_cp
        and 'let legacy = "@cap/lru-cache";' in fixed_cp
        and 'let alias_lit = "validator-rs";' in fixed_cp
        and 'let other = "ciborium";' in fixed_cp,
        fixed_cp,
    )

    # 1c. cap.json validation
    fixed_json_raw = cap_json_file.read_text(encoding="utf-8")
    parsed_json = None
    json_err = ""
    try:
        parsed_json = json.loads(fixed_json_raw)
    except Exception as exc:  # noqa: BLE001
        json_err = str(exc)

    t.check(
        "fixed cap.json is valid JSON (json.loads passes)",
        parsed_json is not None,
        f"error={json_err}\ncontent=\n{fixed_json_raw}",
    )

    if parsed_json is not None:
        deps = parsed_json.get("dependencies", {})
        dev_deps = parsed_json.get("devDependencies", {})

        t.check(
            "cap.json preserves top-level metadata (name/version)",
            parsed_json.get("name") == "ai-toolchain-fixture" and parsed_json.get("version") == "0.1.0",
            f"parsed={parsed_json}",
        )
        t.check(
            "cap.json dependencies remapped & deduplicated correctly",
            set(deps.keys()) == {"lru-cache", "cookie", "@cap/xxhash", "validator", "semver", "ms", "zod"},
            f"deps={deps}",
        )
        t.check(
            "cap.json dependencies has no duplicate keys in raw text",
            fixed_json_raw.count('"lru-cache"') == 1 and fixed_json_raw.count('"zod"') == 1,
            fixed_json_raw,
        )
        t.check(
            "cap.json removes stdlib deps (serde_json, @cap/toml, ciborium)",
            all(k not in deps for k in ("serde_json", "@cap/toml", "cap:serde", "cap:interchange", "cap:toml"))
            and all(k not in dev_deps for k in ("ciborium", "cap:cbor")),
            f"deps={deps}, dev_deps={dev_deps}",
        )
        t.check(
            "cap.json devDependencies remaps handlebars-rust -> handlebars and strips dangling comma",
            set(dev_deps.keys()) == {"handlebars"},
            f"dev_deps={dev_deps}",
        )

    # 1d. Idempotency check
    idem_res = run_cap(["fix", rel_fix_dir])
    t.check(
        "cap fix is idempotent (second run reports 0 fixes across 0 files)",
        idem_res.returncode == 0 and "0 fix(es) across 0 file(s)" in idem_res.stdout,
        f"rc={idem_res.returncode}, stdout={idem_res.stdout!r}",
    )


def test_cap_brief(t: TestRunner) -> None:
    print("\n=== 2. Testing `cap brief` ===")
    cases = [
        ("semver", "packages/semver", ["semver", "README_AGENT.md", "index.d.cp"]),
        ("lz4_flex", "packages/lz4_flex", ["lz4_flex", "README_AGENT.md", "index.d.cp"]),
        ("hkdf", "packages/hkdf", ["hkdf", "README_AGENT.md", "index.d.cp"]),
        ("@cap/xxhash", "packages/@cap/xxhash", ["xxhash", "README_AGENT.md", "index.d.cp"]),
        ("@web/jwt", "packages/@web/jwt", ["jwt", "README_AGENT.md", "index.d.cp"]),
        ("cap:ascii", "doc/lib/ascii.md", ["cap:ascii"]),
        ("cap:json", "doc/lib/json.md", ["cap:json"]),
        ("cap:http", "doc/lib/http.md", ["cap:http"]),
    ]

    for pkg, expected_loc, markers in cases:
        res = run_cap(["brief", pkg])
        out = res.stdout
        ok = (
            res.returncode == 0
            and f"=== cap brief: {pkg}" in out
            and expected_loc in out
            and all(m.lower() in out.lower() for m in markers)
            and len(out.strip()) > 100
        )
        t.check(
            f"cap brief {pkg} exits 0 with valid header & signatures/docs",
            ok,
            f"rc={res.returncode}, len={len(out)}, stderr={res.stderr!r}, head={out[:240]!r}",
        )

    # Negative test: nonexistent package
    miss = run_cap(["brief", "__nonexistent_pkg_for_test__"])
    t.check(
        "cap brief on missing package returns non-zero",
        miss.returncode != 0,
        f"rc={miss.returncode}, stdout={miss.stdout!r}, stderr={miss.stderr!r}",
    )


def test_cap_check(t: TestRunner) -> None:
    print("\n=== 3. Testing `cap check` ===")
    chk_dir = SANDBOX_DIR / "check_case"
    chk_dir.mkdir(parents=True, exist_ok=True)

    valid_file = chk_dir / "valid.cp"
    valid_file.write_text(
        """\
fn add(a: i32, b: i32): i32 {
    return a + b;
}

fn main() {
    let x: i32 = add(1, 2);
    if (x == 3) {
        let y: i32 = x + 1;
    }
}
""",
        encoding="utf-8",
    )

    syntax_err_file = chk_dir / "bad_syntax.cp"
    syntax_err_file.write_text(
        """\
fn main() {
    let x: i32 = 10;
    if x > 5 {
        let y: i32 = x;
    }
}
""",
        encoding="utf-8",
    )

    semantic_err_file = chk_dir / "bad_semantic.cp"
    semantic_err_file.write_text(
        """\
fn main() {
    const x: i32 = 1;
    x = 2;
}
""",
        encoding="utf-8",
    )

    rel_valid = os.path.relpath(valid_file, REPO_ROOT)
    rel_syntax = os.path.relpath(syntax_err_file, REPO_ROOT)
    rel_semantic = os.path.relpath(semantic_err_file, REPO_ROOT)
    rel_dir = os.path.relpath(chk_dir, REPO_ROOT)

    # 3a. Valid .cp file
    r_ok = run_cap(["check", rel_valid])
    t.check(
        "cap check on valid .cp exits 0 with 1 passed, 0 failed",
        r_ok.returncode == 0 and "1 passed, 0 failed (1 total)" in r_ok.stdout,
        f"rc={r_ok.returncode}, stdout={r_ok.stdout!r}, stderr={r_ok.stderr!r}",
    )

    # 3b. Syntax error with AI-First diagnostics
    r_syn = run_cap(["check", rel_syntax])
    combined_syn = r_syn.stdout + "\n" + r_syn.stderr
    has_ai_diag_syn = ("error[" in combined_syn) and (("help:" in combined_syn) or ("suggestion:" in combined_syn))
    t.check(
        "cap check on syntax-error .cp exits non-zero with AI-First diagnostics",
        r_syn.returncode != 0
        and has_ai_diag_syn
        and "0 passed, 1 failed (1 total)" in r_syn.stdout,
        f"rc={r_syn.returncode}, combined={combined_syn!r}",
    )

    # 3c. Semantic/immutability error (const reassignment) with AI-First diagnostics
    r_sem = run_cap(["check", rel_semantic])
    combined_sem = r_sem.stdout + "\n" + r_sem.stderr
    has_ai_diag_sem = ("error[" in combined_sem) and (("help:" in combined_sem) or ("suggestion:" in combined_sem))
    t.check(
        "cap check on semantic-error .cp exits non-zero with AI-First diagnostics",
        r_sem.returncode != 0
        and has_ai_diag_sem
        and "0 passed, 1 failed (1 total)" in r_sem.stdout,
        f"rc={r_sem.returncode}, combined={combined_sem!r}",
    )

    # 3d. Directory summary mode
    r_dir = run_cap(["check", rel_dir])
    t.check(
        "cap check on directory summarizes passed & failed counts (1 passed, 2 failed)",
        r_dir.returncode != 0 and "1 passed, 2 failed (3 total)" in r_dir.stdout,
        f"rc={r_dir.returncode}, stdout={r_dir.stdout!r}, stderr={r_dir.stderr!r}",
    )


def main() -> int:
    if not CAP_BIN.exists():
        print(f"ERROR: cap binary not found at {CAP_BIN}", file=sys.stderr)
        return 2

    if SANDBOX_DIR.exists():
        shutil.rmtree(SANDBOX_DIR)
    SANDBOX_DIR.mkdir(parents=True, exist_ok=True)

    runner = TestRunner()
    try:
        test_cap_fix(runner)
        test_cap_brief(runner)
        test_cap_check(runner)
    finally:
        if SANDBOX_DIR.exists():
            shutil.rmtree(SANDBOX_DIR)

    total = len(runner.passed) + len(runner.failed)
    print(f"\n=== Summary: {len(runner.passed)}/{total} assertions passed ===")
    if runner.failed:
        print("Failed assertions:")
        for name, detail in runner.failed:
            print(f"  - {name}: {detail}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
