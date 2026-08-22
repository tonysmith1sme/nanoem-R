#!/usr/bin/env python3
"""Summarize and diff effect_corpus JSONL reports.

usage:
  effect_corpus_report.py report.jsonl                    # per-file status table
  effect_corpus_report.py old.jsonl new.jsonl             # diff two runs (regressions first)
"""
import json
import sys


def load(path):
    entries = {}
    order = []
    with open(path) as fp:
        for line in fp:
            line = line.strip()
            if not line:
                continue
            entry = json.loads(line)
            entries[entry["path"]] = entry
            order.append(entry["path"])
    return entries, order


def short_reason(entry):
    reason = entry.get("reason", "")
    return reason[:76].replace("\n", " ")


def main():
    if len(sys.argv) == 2:
        entries, order = load(sys.argv[1])
        num_ok = sum(1 for p in order if entries[p]["status"] == "ok")
        num_fragment = sum(1 for p in order if entries[p]["status"] == "fragment")
        num_fail = len(order) - num_ok - num_fragment
        for path in order:
            entry = entries[path]
            name = path.rsplit("/", 1)[-1]
            if entry["status"] == "ok":
                print("  ok       %-32s %d/%d passes" % (name, entry["numCompiled"], entry["numPasses"]))
            elif entry["status"] == "fragment":
                print("  fragment %-32s %s" % (name, short_reason(entry)))
            else:
                print("  FAIL     %-32s [%s] %s" % (name, entry.get("bucket", "?"), short_reason(entry)))
        print("\ntotal: %d effects, %d ok, %d fail, %d fragments" % (len(order), num_ok, num_fail, num_fragment))
        return 0 if num_fail == 0 else 1
    if len(sys.argv) == 3:
        old, _ = load(sys.argv[1])
        new, order = load(sys.argv[2])
        regressions = improvements = 0
        for path in order:
            new_status = new[path]["status"]
            old_status = old.get(path, {}).get("status")
            if old_status == "ok" and new_status != "ok":
                print("REGRESSION  %-40s %s" % (path.rsplit("/", 1)[-1], short_reason(new[path])))
                regressions += 1
            elif old_status not in (None, "ok") and new_status == "ok":
                print("improved    %-40s (was %s)" % (path.rsplit("/", 1)[-1], old_status))
                improvements += 1
            elif old_status is None:
                print("new         %-40s %s" % (path.rsplit("/", 1)[-1], new_status))
        for path in old:
            if path not in new:
                print("removed     %s" % path.rsplit("/", 1)[-1])
        print("\n%d regressions, %d improvements" % (regressions, improvements))
        return 2 if regressions else 0
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main())
