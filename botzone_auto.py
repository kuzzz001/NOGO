#!/usr/bin/env python3
import urllib.request
import json
import subprocess
import time
import os
import sys

BOTZONE_API = "https://botzone.org.cn/api/69feaaa9fbc31027286297ef/6343940472"
LOCALAI_URL = f"{BOTZONE_API}/localai"
RUNMATCH_URL = f"{BOTZONE_API}/runmatch"
BOT_WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
BOT_PATH = os.path.join(BOT_WORKING_DIR, ".duel_build", "v2_bot.out")

GAME_NAME = "NoGo"
OPPONENT_BOT = "6a02c4f1fad6c36646932067"

matches = {}
stats = {"wins": 0, "losses": 0, "draws": 0, "errors": 0}
round_num = 0


def compile_bot():
    src = os.path.join(BOT_WORKING_DIR, "new1_v2.cpp")
    out = BOT_PATH
    os.makedirs(os.path.dirname(out), exist_ok=True)
    print(f"[COMPILE] {src}")
    result = subprocess.run(["g++", "-std=c++17", "-O2", src, "-o", out], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[COMPILE ERROR] {result.stderr}")
        sys.exit(1)
    print(f"[COMPILE OK]")


def call_bot(request_json):
    try:
        result = subprocess.run(
            [BOT_PATH],
            input=request_json + "\n",
            capture_output=True,
            text=True,
            timeout=5,
            cwd=BOT_WORKING_DIR,
        )
        return result.stdout.strip(), result.stderr.strip()
    except subprocess.TimeoutExpired:
        return "", "TIMEOUT"
    except Exception as e:
        return "", str(e)


def create_match():
    req = urllib.request.Request(RUNMATCH_URL)
    req.add_header("X-Game", GAME_NAME)
    req.add_header("X-Player-0", "me")
    req.add_header("X-Player-1", OPPONENT_BOT)
    try:
        res = urllib.request.urlopen(req, timeout=15)
        resp = res.read().decode().strip()
        obj = json.loads(resp)
        if "matchid" in obj:
            mid = obj["matchid"]
            print(f"[CREATE] match {mid}")
            matches[mid] = {"has_request": False, "has_response": False, "current_request": "", "current_response": ""}
            return mid
        print(f"[CREATE] no matchid: {resp[:100]}")
    except urllib.error.HTTPError as e:
        body = e.read().decode()[:100]
        print(f"[CREATE] HTTP {e.code}: {body}")
        if e.code == 403 and "300 seconds" in body:
            return "rate_limited"
    except Exception as e:
        print(f"[CREATE] error: {e}")
    return None


def process_match_result(mid, slot, scores):
    if not scores or len(scores) < 2:
        print(f"[RESULT] {mid} slot={slot} scores={scores} (incomplete)")
        return
    try:
        my_score = int(scores[int(slot)])
        opp_score = int(scores[1 - int(slot)])
        if my_score > opp_score:
            stats["wins"] += 1
            result = "WIN"
        elif my_score < opp_score:
            stats["losses"] += 1
            result = "LOSE"
        else:
            stats["draws"] += 1
            result = "DRAW"
        total = stats["wins"] + stats["losses"] + stats["draws"]
        print(f"[RESULT] {mid} {result} {my_score}-{opp_score} | stats: {stats['wins']}W-{stats['losses']}L-{stats['draws']}D (total={total})")
    except Exception as e:
        print(f"[RESULT] {mid} slot={slot} scores={scores} ({e})")


def fetch_and_process():
    if not matches:
        req = urllib.request.Request(LOCALAI_URL)
        try:
            res = urllib.request.urlopen(req, timeout=10)
            text = res.read().decode()
        except Exception as e:
            print(f"[FETCH] error: {e}")
            return
        lines = text.split("\n")
        if not lines:
            return
        try:
            request_count, result_count = map(int, lines[0].split())
        except:
            return
        idx = 1
        for i in range(request_count):
            if idx + 1 >= len(lines):
                break
            mid = lines[idx].strip()
            req_text = lines[idx + 1].strip()
            idx += 2
            if mid not in matches:
                matches[mid] = {"has_request": False, "has_response": False, "current_request": "", "current_response": ""}
            matches[mid]["has_request"] = True
            matches[mid]["current_request"] = req_text
        for i in range(result_count):
            if idx >= len(lines):
                break
            parts = lines[idx].split()
            idx += 1
            if len(parts) < 4:
                continue
            mid, slot = parts[0], parts[1]
            player_count = int(parts[2])
            scores = parts[3:]
            if mid in matches:
                del matches[mid]
            if player_count > 0:
                process_match_result(mid, slot, scores)
            else:
                print(f"[ABORT] {mid}")
        return

    req = urllib.request.Request(LOCALAI_URL)
    for mid, m in matches.items():
        if m.get("has_response") and m.get("has_request") and m.get("current_response"):
            req.add_header(f"X-Match-{mid}", m["current_response"])

    try:
        res = urllib.request.urlopen(req, timeout=30)
        text = res.read().decode()
    except Exception as e:
        print(f"[FETCH] error: {e}")
        return

    lines = text.split("\n")
    if not lines:
        return

    try:
        request_count, result_count = map(int, lines[0].split())
    except:
        return

    idx = 1
    for i in range(request_count):
        if idx + 1 >= len(lines):
            break
        mid = lines[idx].strip()
        req_text = lines[idx + 1].strip()
        idx += 2
        if mid not in matches:
            matches[mid] = {"has_request": False, "has_response": False, "current_request": "", "current_response": ""}
        matches[mid]["has_request"] = True
        matches[mid]["current_request"] = req_text
        print(f"[REQUEST] {mid}: {req_text[:80]}")

    for i in range(result_count):
        if idx >= len(lines):
            break
        parts = lines[idx].split()
        idx += 1
        if len(parts) < 4:
            continue
        mid, slot = parts[0], parts[1]
        player_count = int(parts[2])
        scores = parts[3:]
        if mid in matches:
            del matches[mid]
        if player_count > 0:
            process_match_result(mid, slot, scores)
        else:
            print(f"[ABORT] {mid}")

    for mid, m in matches.items():
        if m.get("has_request") and not m.get("has_response"):
            resp_text, err = call_bot(m["current_request"])
            if err:
                print(f"[BOT ERROR] {mid}: {err}")
                stats["errors"] += 1
                m["current_response"] = "0 0"
            else:
                try:
                    obj = json.loads(resp_text)
                    r = obj.get("response", {})
                    if isinstance(r, dict):
                        x, y = r.get("x", 0), r.get("y", 0)
                    else:
                        x, y = r[0], r[1]
                    m["current_response"] = f"{x} {y}"
                    print(f"[BOT] {mid} -> {x},{y}")
                except Exception as e:
                    print(f"[PARSE ERROR] {mid}: {e} -> {resp_text[:80]}")
                    m["current_response"] = "0 0"
            m["has_response"] = True
            m["has_request"] = False


def main():
    global round_num
    if not os.path.exists(BOT_PATH):
        print("[INIT] Bot not found, compiling...")
        compile_bot()
    else:
        print(f"[INIT] Using bot: {BOT_PATH}")

    print(f"[INIT] Opponent: {OPPONENT_BOT}")
    print(f"[INIT] Game: {GAME_NAME}")
    print()

    last_create_try = 0
    rate_limited_until = 0

    while True:
        now = time.time()
        round_num += 1
        print(f"\n==== Round {round_num} ({time.strftime('%H:%M:%S')}) ====")

        if now >= rate_limited_until:
            mid = create_match()
            if mid == "rate_limited":
                rate_limited_until = now + 300
                print(f"[RATE LIMIT] blocked until {time.strftime('%H:%M:%S', time.localtime(rate_limited_until))}")
                mid = None
            last_create_try = now
        else:
            remaining = int(rate_limited_until - now)
            print(f"[WAIT] rate limit: {remaining}s remaining")

        time.sleep(2)

        for poll in range(30):
            fetch_and_process()
            time.sleep(2)
            if matches:
                break

        total = stats["wins"] + stats["losses"] + stats["draws"]
        print(f"[STATS] {stats['wins']}W-{stats['losses']}L-{stats['draws']}D | errors={stats['errors']} | total={total}")
        print(f"[LOG] {'='*60}")
        print(f"[LOG] 详细日志: tail -f /Users/kuzzz/NOGO2026/botzone_auto.log")
        print(f"[LOG] {'='*60}")

        time.sleep(3)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[STOPPED]")
        total = stats["wins"] + stats["losses"] + stats["draws"]
        print(f"[FINAL] {stats['wins']}W-{stats['losses']}L-{stats['draws']}D | errors={stats['errors']} | total={total}")