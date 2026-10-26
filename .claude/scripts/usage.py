#!/usr/bin/env python
"""统计当前会话的 token 用量与花费。

数据源：%USERPROFILE%\\.dsh\\sessions\\**\\session.jsonl.zstd
每步的 assistant/chunk 里带 usage: inputTokens / outputTokens / cacheReadTokens / reasoningTokens。

用法：
    python usage.py                 # 统计最近的会话
    python usage.py --budget 20     # 按 20 元预算算剩余
    python usage.py --list          # 列出所有会话
    python usage.py --session <目录或文件>
"""

from __future__ import annotations

import argparse
import glob
import io
import json
import os
import sys

import zstandard

for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(encoding="utf-8", errors="replace")

SESSIONS_ROOT = os.path.join(os.path.expanduser("~"), ".dsh", "sessions")

# 默认单价（元 / 百万 token），按 DeepSeek 风格设的，可命令行覆盖
DEFAULT_PRICE = {
    "input": 2.0,        # 未命中缓存的输入
    "cache": 0.5,        # 命中缓存的输入
    "output": 8.0,       # 输出（含推理）
}


def find_sessions() -> list[str]:
    files = glob.glob(os.path.join(SESSIONS_ROOT, "**", "session.jsonl.zstd"), recursive=True)
    files.sort(key=lambda p: os.path.getmtime(p), reverse=True)
    return files


def read_usage(path: str) -> dict:
    d = zstandard.ZstdDecompressor()
    tot = {"input": 0, "output": 0, "cache": 0, "reasoning": 0}
    turns: dict[tuple, dict] = {}
    steps = 0
    with open(path, "rb") as fh:
        with d.stream_reader(fh) as r:
            for line in io.TextIOWrapper(r, encoding="utf-8", errors="replace"):
                if '"usage"' not in line:
                    continue
                try:
                    o = json.loads(line)
                except Exception:
                    continue
                data = o.get("data") or {}
                u = (data.get("chunk") or {}).get("usage")
                if not u:
                    continue
                steps += 1
                key = (data.get("turn"), data.get("step"))
                cur = turns.setdefault(key, {"input": 0, "output": 0, "cache": 0})
                cur["input"] += u.get("inputTokens", 0)
                cur["output"] += u.get("outputTokens", 0)
                cur["cache"] += u.get("cacheReadTokens", 0)
                tot["input"] += u.get("inputTokens", 0)
                tot["output"] += u.get("outputTokens", 0)
                tot["cache"] += u.get("cacheReadTokens", 0)
                tot["reasoning"] += u.get("reasoningTokens", 0)
    return {"total": tot, "turns": turns, "steps": steps}


def cost_of(tot: dict, price: dict) -> dict:
    m = 1_000_000
    c_in = tot["input"] / m * price["input"]
    c_cache = tot["cache"] / m * price["cache"]
    c_out = tot["output"] / m * price["output"]
    return {"input": c_in, "cache": c_cache, "output": c_out,
            "total": c_in + c_cache + c_out}


def main() -> int:
    ap = argparse.ArgumentParser(description="会话 token 用量与花费")
    ap.add_argument("--session", help="会话目录或 session.jsonl.zstd 路径")
    ap.add_argument("--list", action="store_true", help="列出所有会话文件")
    ap.add_argument("--budget", type=float, help="预算（元），用于计算剩余")
    ap.add_argument("--price-input", type=float, default=DEFAULT_PRICE["input"])
    ap.add_argument("--price-cache", type=float, default=DEFAULT_PRICE["cache"])
    ap.add_argument("--price-output", type=float, default=DEFAULT_PRICE["output"])
    args = ap.parse_args()

    sessions = find_sessions()
    if not sessions:
        print("找不到会话日志")
        return 1

    if args.list:
        for i, p in enumerate(sessions[:20]):
            print(f"{i:>2}  {os.path.getmtime(p):.0f}  {p}")
        return 0

    if args.session:
        p = args.session
        if os.path.isdir(p):
            cand = glob.glob(os.path.join(p, "*.zstd"))
            if not cand:
                print("目录里没有 session.jsonl.zstd")
                return 1
            p = cand[0]
    else:
        p = sessions[0]

    u = read_usage(p)
    tot = u["total"]
    price = {"input": args.price_input, "cache": args.price_cache,
             "output": args.price_output}
    c = cost_of(tot, price)

    print(f"会话：{p}")
    print(f"步数：{u['steps']}    轮次：{len({k[0] for k in u['turns']})}")
    print()
    print(f"{'未缓存输入':<12}{tot['input']:>12,}  token")
    print(f"{'缓存读取':<12}{tot['cache']:>12,}  token")
    print(f"{'输出':<12}{tot['output']:>12,}  token   （含推理 {tot['reasoning']:,}）")
    print(f"{'合计':<12}{tot['input'] + tot['cache'] + tot['output']:>12,}  token")
    print()
    print(f"单价（元/百万）：输入 {price['input']}  缓存 {price['cache']}  输出 {price['output']}")
    print(f"{'输入花费':<10}{c['input']:>8.2f} 元")
    print(f"{'缓存花费':<10}{c['cache']:>8.2f} 元  ← 通常是大头")
    print(f"{'输出花费':<10}{c['output']:>8.2f} 元")
    print(f"{'合计':<10}{c['total']:>8.2f} 元")
    if args.budget:
        left = args.budget - c["total"]
        print()
        print(f"预算 {args.budget:.2f} 元 → 已用 {c['total']:.2f} 元，"
              f"剩余 {left:.2f} 元（{left / args.budget * 100:.0f}%）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
