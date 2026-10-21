#!/usr/bin/env python3
"""Report (and optionally apply) Airwindows upstream changes for the included effects.

Usage:
  git clone https://github.com/baconpaul/airwin2rack /tmp/airwin2rack
  python3 tools/sync_upstream.py /tmp/airwin2rack            # report only, changes nothing
  python3 tools/sync_upstream.py /tmp/airwin2rack --apply    # copy changed files in

Rules (see README "Updating"):
  * An effect whose DSP files changed may sound different. After --apply, bump that effect's
    "version" in manifest/effects.json, so old sessions can be told apart.
  * Never change an effect's "id".
  * Run tools/gen_effects.py and the tests afterwards. The null tests then compare against
    the new upstream code.
"""
import filecmp
import json
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LOCAL = ROOT / "third_party" / "airwin_consolidated"
MANIFEST = ROOT / "manifest" / "effects.json"
BASE_FILES = ("airwin_consolidated_base.h", "airwin_consolidated_base.cpp")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    upstream_root = pathlib.Path(sys.argv[1]).resolve()
    apply = "--apply" in sys.argv
    src = upstream_root / "src"
    up_aw = src / "autogen_airwin"
    if not up_aw.is_dir():
        sys.exit(f"not an airwin2rack checkout (missing {up_aw})")

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    pinned = manifest["upstream"]["consolidated"]["commit"]
    try:
        head = subprocess.check_output(["git", "-C", str(upstream_root), "rev-parse", "HEAD"], text=True).strip()
    except Exception:
        head = "(unknown)"
    print(f"pinned commit : {pinned}\nupstream HEAD : {head}\n")

    changed_effects, missing, changed_base = [], [], []

    for f in BASE_FILES:
        if not (src / f).exists():
            missing.append(f)
        elif not filecmp.cmp(src / f, LOCAL / f, shallow=False):
            changed_base.append(f)

    for e in manifest["effects"]:
        if not e.get("included"):
            continue
        n = e["upstream"]
        files = [f"{n}.h", f"{n}.cpp", f"{n}Proc.cpp"]
        diffs = []
        for f in files:
            up = up_aw / f
            if not up.exists():
                missing.append(f)
            elif not filecmp.cmp(up, LOCAL / "autogen_airwin" / f, shallow=False):
                diffs.append(f)
        if diffs:
            changed_effects.append((e, diffs))

    if changed_base:
        print("BASE CLASS CHANGED (review carefully, affects every effect):")
        for f in changed_base:
            print(f"  {f}")
        print()
    if changed_effects:
        print("EFFECTS WITH CHANGED CODE (may sound different -> bump 'version' after applying):")
        for e, diffs in changed_effects:
            print(f"  {e['id']:<22} v{e['version']}  {', '.join(diffs)}")
        print()
    if missing:
        print("MISSING UPSTREAM (renamed or removed? do NOT apply blindly):")
        for f in missing:
            print(f"  {f}")
        print()
    if not (changed_base or changed_effects or missing):
        print("All included effects are identical to upstream. Nothing to do.")
        return

    if not apply:
        print("Report only. Re-run with --apply to copy the changed files in.")
        return
    if missing:
        sys.exit("Refusing to apply while upstream files are missing.")

    for f in changed_base:
        shutil.copy2(src / f, LOCAL / f)
    for e, diffs in changed_effects:
        for f in diffs:
            shutil.copy2(up_aw / f, LOCAL / "autogen_airwin" / f)
    if (upstream_root / "LICENSE.md").exists():
        shutil.copy2(upstream_root / "LICENSE.md", LOCAL / "LICENSE.md")

    manifest_text = MANIFEST.read_text(encoding="utf-8").replace(pinned, head)
    MANIFEST.write_text(manifest_text, encoding="utf-8")
    print(f"Applied. manifest commit updated to {head}.")
    print("Next: bump 'version' of the effects listed above, run tools/gen_effects.py, build, run tests.")


if __name__ == "__main__":
    main()
