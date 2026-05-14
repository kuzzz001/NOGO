#!/usr/bin/env python3
import urllib.request
import json
import subprocess
import time
import os
import sys

PYTHONUNBUFFERED = os.environ.get("PYTHONUNBUFFERED", "1")

BOTZONE_URL = "https://botzone.org.cn/api/69feaaa9fbc31027286297ef/6343940472/localai"
BOT_PATH = os.path.join(os.path.dirname(__file__), ".duel_build", "v2_bot.out")
BOT_WORKING_DIR = os.path.dirname(__file__)


class Match:
    def __init__(self, matchid, first_request):
        self.matchid = matchid
        self.has_request = True
        self.has_response = False
        self.current_request = first_request
        self.current_response = ""

    def __repr__(self):
        return f"Match({self.matchid}, req={self.has_request}, resp={self.has_response})"


def compile_bot():
    src = os.path.join(BOT_WORKING_DIR, "new1_v2.cpp")
    out = BOT_PATH
    os.makedirs(os.path.dirname(out), exist_ok=True)
    print(f"Compiling {src} -> {out}")
    result = subprocess.run(
        ["g++", "-std=c++17", "-O2", src, "-o", out],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print("Compile failed:")
        print(result.stderr)
        sys.exit(1)
    print("Compile OK")


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


def fetch_and_process():
    matches_to_remove = []

    req = urllib.request.Request(BOTZONE_URL)

    for mid, m in list(matches.items()):
        if m.has_response and m.has_request and m.current_response:
            req.add_header(f"X-Match-{mid}", m.current_response)

    while True:
        try:
            res = urllib.request.urlopen(req, timeout=60)
            botzone_input = res.read().decode()
            break
        except Exception as e:
            print(f"Error fetching: {e}, retrying in 5s...")
            time.sleep(5)
            continue

    lines = botzone_input.split("\n")
    if not lines:
        return

    try:
        request_count, result_count = map(int, lines[0].split())
    except ValueError:
        print(f"Failed to parse first line: {lines[0]}")
        return

    print(f"request_count={request_count}, result_count={result_count}")

    idx = 1
    for i in range(request_count):
        if idx >= len(lines):
            break
        matchid = lines[idx].strip()
        idx += 1
        if idx >= len(lines):
            break
        request = lines[idx].strip()
        idx += 1

        if matchid in matches:
            print(f"Request for match [{matchid}]: {request[:80]}...")
            matches[matchid].has_request = True
            matches[matchid].current_request = request
        else:
            print(f"New match [{matchid}]: {request[:80]}...")
            matches[matchid] = Match(matchid, request)

    for i in range(result_count):
        if idx >= len(lines):
            break
        parts = lines[idx].split()
        idx += 1
        if len(parts) < 3:
            continue
        matchid, slot = parts[0], parts[1]
        player_count = int(parts[2])
        scores = parts[3:] if len(parts) > 3 else []
        if player_count == 0:
            print(f"Match [{matchid}] aborted: I'm player {slot}")
        else:
            print(f"Match [{matchid}] finished: I'm player {slot}, scores={scores}")
        if matchid in matches:
            del matches[matchid]

    for mid, m in matches.items():
        if m.has_request and not m.has_response:
            print(f"\n=== Processing match [{mid}] ===")
            print(f"Request: {m.current_request[:200]}")

            resp_text, err = call_bot(m.current_request)

            if err:
                print(f"Bot error: {err}")
            print(f"Bot raw output: {resp_text[:200]}")

            try:
                obj = json.loads(resp_text)
                if "response" in obj:
                    r = obj["response"]
                    if isinstance(r, dict):
                        x, y = r["x"], r["y"]
                    else:
                        x, y = r[0], r[1]
                else:
                    x, y = 0, 0
                m.current_response = f"{x} {y}"
                print(f"Response: {m.current_response}")
            except Exception as e:
                print(f"Failed to parse response: {e}, using 0 0")
                m.current_response = "0 0"

            m.has_response = True
            m.has_request = False
            print(f"=== Done [{mid}] ===\n")


if __name__ == "__main__":
    if not os.path.exists(BOT_PATH):
        print("Bot not compiled, compiling now...")
        compile_bot()
    else:
        print(f"Using existing bot at {BOT_PATH}")

    matches = {}

    print("Starting Botzone local AI loop...")
    print(f"URL: {BOTZONE_URL}")
    print("Waiting for matches...\n")

    while True:
        try:
            fetch_and_process()
        except Exception as e:
            print(f"Error in loop: {e}")
            import traceback
            traceback.print_exc()
            print("Retrying in 10s...")
            time.sleep(10)