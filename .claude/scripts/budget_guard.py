#!/usr/bin/env python
"""预算看门狗：后台定期统计当前会话花费，到线就告警并写停止标记。

它不是真正的硬限额（拦截不了 API 调用），真正的硬限额是 DeepSeek 的预付费余额。
它解决的是「我还剩多少」这个问题，让我在每次动手前能查到。

用法：
    python budget_guard.py                 # 单次检查，超线写标记
    python budget_guard.py --watch --interval 30    # 后台循环

停止标记：.claude/scripts/.BUDGET_STOP（存在就表示已超预算）
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from usage import find_sessions, read_usage, cost_of  # noqa: E402

for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(encoding="utf-8", errors="replace")

HERE = os.path.dirname(os.path.abspath(__file__))
CONF = os.path.join(HERE, "prices.json")
STOP = os.path.join(HERE, ".BUDGET_STOP")


def load_conf() -> dict:
    if not os.path.exists(CONF):
        return {"input": 2.0, "cache": 0.5, "output": 8.0, "budget": 20.0}
    with open(CONF, encoding="utf-8") as f:
        c = json.load(f)
    return {k: float(v) for k, v in c.items() if not k.startswith("_")}


def check(conf: dict, remain_ratio: float = 0.2) -> tuple[float, float]:
    sessions = find_sessions()
    if not sessions:
        return 0.0, conf["budget"]
    u = read_usage(sessions[0])
    c = cost_of(u["total"], conf)
    spent, budget = c["total"], conf["budget"]
    left = budget - spent
    print(f"[{time.strftime('%H:%M:%S')}] 已用 {spent:.2f} / {budget:.2f} 元，"
          f"剩 {left:.2f} 元（{left / budget * 100:.0f}%）", flush=True)
    if left <= 0:
        with open(STOP, "w", encoding="utf-8") as f:
            f.write(f"预算已超：{spent:.2f} / {budget:.2f}\n")
        print(f"!! 超出预算，已写停止标记：{STOP}", flush=True)
    elif left < budget * remain_ratio and not os.path.exists(STOP):
        print(f"   [提醒] 剩余不足 {remain_ratio:.0%}，建议收尾", flush=True)
    return spent, left


def main() -> int:
    ap = argparse.ArgumentParser(description="预算看门狗")
    ap.add_argument("--watch", action="store_true", help="循环监控")
    ap.add_argument("--interval", type=int, default=30, help="检查间隔秒数")
    ap.add_argument("--budget", type=float, help="覆盖 prices.json 里的预算")
    ap.add_argument("--clear", action="store_true", help="清除停止标记")
    args = ap.parse_args()

    if args.clear and os.path.exists(STOP):
        os.remove(STOP)
        print("已清除停止标记")
        return 0

    conf = load_conf()
    if args.budget:
        conf["budget"] = args.budget

    if not args.watch:
        check(conf)
        return 0

    while True:
        check(conf)
        time.sleep(args.interval)


if __name__ == "__main__":
    raise SystemExit(main())
