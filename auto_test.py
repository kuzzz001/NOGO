#!/usr/bin/env python3
import subprocess
import sys
import os
import time
import json
import re
from pathlib import Path

BUILD_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".duel_build")
BOT_V2 = os.path.join(BUILD_DIR, "v2_bot.out")
BOT_TEST = os.path.join(BUILD_DIR, "bot_b.out")
V2_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "new1_v2.cpp")
TEST_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "new1.cpp")

GAMES_PER_ROUND = 10
TIMEOUT_PER_MOVE = 1.2

stats = {"v2_wins": 0, "v2_losses": 0, "draws": 0, "errors": 0}
round_num = 0


def compile():
    os.makedirs(BUILD_DIR, exist_ok=True)
    print(f"[COMPILE] new1_v2.cpp")
    r1 = subprocess.run(["g++", "-std=c++17", "-O2", V2_SRC, "-o", BOT_V2], capture_output=True, text=True)
    if r1.returncode != 0:
        print(f"[COMPILE ERROR v2] {r1.stderr}")
        sys.exit(1)
    print(f"[COMPILE] new1.cpp")
    r2 = subprocess.run(["g++", "-std=c++17", "-O2", TEST_SRC, "-o", BOT_TEST], capture_output=True, text=True)
    if r2.returncode != 0:
        print(f"[COMPILE ERROR test] {r2.stderr}")
        sys.exit(1)
    print(f"[COMPILE OK]\n")


def run_match():
    try:
        result = subprocess.run(
            ["python3", os.path.join(os.path.dirname(__file__), "duel_nogo.py"),
             BOT_V2, BOT_TEST,
             "--games", str(GAMES_PER_ROUND),
             "--timeout", str(TIMEOUT_PER_MOVE)],
            capture_output=True,
            text=True,
            timeout=600,
        )
        return result.stdout + "\n" + result.stderr
    except subprocess.TimeoutExpired:
        return "[TIMEOUT] Match took too long\n"
    except Exception as e:
        return f"[ERROR] {e}\n"


def parse_stats(output):
    m = re.search(r"running A:(\d+)% \((\d+)-(\d+)\)", output)
    if m:
        return int(m.group(2)), int(m.group(3))
    m2 = re.search(r"A:(\d+)W-(\d+)L", output)
    if m2:
        return int(m2.group(1)), int(m2.group(2))
    return None, None


def main():
    global round_num
    print(f"{'='*70}")
    print(f"  NoGo Botzone Auto Tester (v2 vs new1)")
    print(f"{'='*70}")
    compile()

    while True:
        round_num += 1
        ts = time.strftime("%Y-%m-%d %H:%M:%S")
        print(f"\n{'='*70}")
        print(f"  Round {round_num} | {ts}")
        print(f"{'='*70}")
        sys.stdout.flush()

        output = run_match()
        lines = output.splitlines()
        for l in lines:
            print(l)
        sys.stdout.flush()

        aw, al = parse_stats(output)
        if aw is not None:
            stats["v2_wins"] += aw
            stats["v2_losses"] += al
            total = aw + al
            rate = f"{aw/total*100:.1f}%" if total > 0 else "-"
            print(f"\n[CUMULATIVE] v2: {stats['v2_wins']}W-{stats['v2_losses']}L | this round: {aw}W-{al}L | win rate: {rate}")
        else:
            print(f"\n[PARSE ERROR] Could not parse round result")

        sys.stdout.flush()
        time.sleep(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[STOPPED]")
        total = stats["v2_wins"] + stats["v2_losses"]
        rate = f"{stats['v2_wins']/total*100:.1f}%" if total > 0 else "-"
        print(f"[FINAL] v2: {stats['v2_wins']}W-{stats['v2_losses']}L | win rate: {rate}")