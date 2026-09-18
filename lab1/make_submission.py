#!/usr/bin/env python3
"""Pack a student's code into a submission zip.

Run from the lab root (the Makefile does this for you):

    make submission          # -> submission.zip
    python3 make_submission.py [OUTPUT.zip]

It skips build artifacts and other junk by honouring the
.gitignore files in the tree (so *.o, the compiled binaries,
etc. are left out), without needing git to be installed.
"""

import fnmatch
import os
import sys
import zipfile

# Always skipped, regardless of .gitignore.
ALWAYS_IGNORE_DIRS = {".git", "__pycache__"}


class Rule:
    """One .gitignore pattern, anchored at the directory of its .gitignore."""

    def __init__(self, base, pattern):
        self.base = base  # dir (relative to root) the pattern is anchored to
        self.negated = pattern.startswith("!")
        if self.negated:
            pattern = pattern[1:]
        self.dir_only = pattern.endswith("/")
        pattern = pattern.rstrip("/")
        pattern = pattern[1:] if pattern.startswith("/") else pattern
        # A slash anywhere (after the optional leading one) anchors the pattern
        # to `base`; otherwise it matches a basename at any depth below `base`.
        self.anchored = "/" in pattern
        self.pattern = pattern

    def matches(self, rel, is_dir):
        if self.dir_only and not is_dir:
            return False
        # Path relative to this rule's base dir; skip paths outside it.
        if self.base:
            prefix = self.base + "/"
            if not (rel == self.base or rel.startswith(prefix)):
                return False
            sub = rel[len(prefix):] if rel.startswith(prefix) else ""
        else:
            sub = rel
        if self.anchored:
            return fnmatch.fnmatch(sub, self.pattern)
        return fnmatch.fnmatch(os.path.basename(rel), self.pattern)


def load_rules(root):
    """Collect rules from every .gitignore in the tree, shallowest first."""
    rules = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in ALWAYS_IGNORE_DIRS]
        if ".gitignore" in filenames:
            base = os.path.relpath(dirpath, root)
            base = "" if base == "." else base.replace(os.sep, "/")
            with open(os.path.join(dirpath, ".gitignore")) as fh:
                for line in fh:
                    line = line.rstrip("\n")
                    stripped = line.strip()
                    if not stripped or stripped.startswith("#"):
                        continue
                    rules.append(Rule(base, stripped))
    return rules


def is_ignored(rel, is_dir, rules):
    """Last matching rule wins (a later '!' pattern can re-include)."""
    ignored = False
    for rule in rules:
        if rule.matches(rel, is_dir):
            ignored = not rule.negated
    return ignored


def main():
    out_zip = sys.argv[1] if len(sys.argv) > 1 else "submission.zip"
    root = os.path.dirname(os.path.abspath(__file__))
    os.chdir(root)

    # Never pack the archive we're writing, even if it isn't in .gitignore.
    self_names = {os.path.basename(out_zip)}
    rules = load_rules(".")

    n = 0
    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED) as zf:
        for dirpath, dirnames, filenames in os.walk("."):
            rel_dir = os.path.relpath(dirpath, ".")
            rel_dir = "" if rel_dir == "." else rel_dir.replace(os.sep, "/")
            # Prune ignored directories so we don't descend into them.
            keep = []
            for d in dirnames:
                if d in ALWAYS_IGNORE_DIRS:
                    continue
                rel = f"{rel_dir}/{d}" if rel_dir else d
                if not is_ignored(rel, True, rules):
                    keep.append(d)
            dirnames[:] = keep

            for f in filenames:
                rel = f"{rel_dir}/{f}" if rel_dir else f
                if rel in self_names or f in self_names:
                    continue
                if is_ignored(rel, False, rules):
                    continue
                zf.write(rel, rel)
                n += 1

    print(f"Wrote {n} files to {out_zip}")


if __name__ == "__main__":
    main()
