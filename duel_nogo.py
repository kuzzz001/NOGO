#!/usr/bin/env python3
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path


BOARD_SIZE = 9
BLACK = 1
WHITE = -1
DIRS = ((1, 0), (-1, 0), (0, 1), (0, -1))


def in_board(x, y):
    return 0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE


def group_and_liberties(board, sx, sy):
    color = board[sx][sy]
    stack = [(sx, sy)]
    visited = {(sx, sy)}
    group = []
    liberties = set()

    while stack:
        x, y = stack.pop()
        group.append((x, y))
        for dx, dy in DIRS:
            nx, ny = x + dx, y + dy
            if not in_board(nx, ny):
                continue
            if board[nx][ny] == 0:
                liberties.add((nx, ny))
            elif board[nx][ny] == color and (nx, ny) not in visited:
                visited.add((nx, ny))
                stack.append((nx, ny))
    return group, liberties


def is_legal_move(board, x, y, color):
    if not in_board(x, y) or board[x][y] != 0:
        return False

    board[x][y] = color
    _, self_liberties = group_and_liberties(board, x, y)
    if not self_liberties:
        board[x][y] = 0
        return False

    for dx, dy in DIRS:
        nx, ny = x + dx, y + dy
        if in_board(nx, ny) and board[nx][ny] == -color:
            _, opp_liberties = group_and_liberties(board, nx, ny)
            if not opp_liberties:
                board[x][y] = 0
                return False

    board[x][y] = 0
    return True


def legal_moves(board, color):
    moves = []
    for x in range(BOARD_SIZE):
        for y in range(BOARD_SIZE):
            if is_legal_move(board, x, y, color):
                moves.append((x, y))
    return moves


def history_to_payload(history, bot_color):
    black_moves = history[0::2]
    white_moves = history[1::2]

    if bot_color == BLACK:
        requests = [{"x": -1, "y": -1}] + [{"x": x, "y": y} for x, y in white_moves]
        responses = [{"x": x, "y": y} for x, y in black_moves]
    else:
        requests = [{"x": x, "y": y} for x, y in black_moves]
        responses = [{"x": x, "y": y} for x, y in white_moves]

    return json.dumps({"requests": requests, "responses": responses}, separators=(",", ":"))


def parse_move(stdout_text):
    lines = [line.strip() for line in stdout_text.splitlines() if line.strip()]
    if not lines:
        raise ValueError("empty output")

    last = lines[-1]
    try:
        obj = json.loads(last)
        response = obj["response"]
        return int(response["x"]), int(response["y"])
    except Exception:
        pass

    match = re.search(r'"x"\s*:\s*(-?\d+).*"y"\s*:\s*(-?\d+)', stdout_text, re.S)
    if match:
        return int(match.group(1)), int(match.group(2))

    match = re.search(r"(-?\d+)\s+(-?\d+)", last)
    if match:
        return int(match.group(1)), int(match.group(2))

    raise ValueError(f"cannot parse move from output: {last}")


def read_source_text(path):
    try:
        return Path(path).read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return ""


def compile_cpp(source_path, output_path, extra_flags):
    source_text = read_source_text(source_path)
    cmd = ["g++", "-std=c++17", "-O2", source_path, "-o", output_path]

    # first.cpp-like bots may still rely on jsoncpp. Keep a best-effort fallback
    # and allow the caller to override with --flags-a/--flags-b.
    if 'jsoncpp/json.h' in source_text:
        cmd.extend([
            "-I/opt/homebrew/include",
            "-I/usr/local/include",
            "-L/opt/homebrew/lib",
            "-L/usr/local/lib",
            "-ljsoncpp",
        ])

    if extra_flags:
        cmd.extend(shlex.split(extra_flags))

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"compile failed for {source_path}\n"
            f"command: {' '.join(cmd)}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def ensure_executable(path):
    st = os.stat(path)
    os.chmod(path, st.st_mode | 0o111)


def prepare_bot(input_path, build_dir, slot_name, extra_flags):
    input_path = os.path.abspath(input_path)
    suffix = Path(input_path).suffix.lower()

    if suffix == ".cpp":
        output_path = os.path.join(build_dir, f"{slot_name}.out")
        compile_cpp(input_path, output_path, extra_flags)
        ensure_executable(output_path)
        return output_path

    if os.path.isfile(input_path):
        ensure_executable(input_path)
        return input_path

    raise FileNotFoundError(f"bot path does not exist: {input_path}")


def call_bot(executable, payload, timeout_sec):
    result = subprocess.run(
        [executable],
        input=payload + "\n",
        capture_output=True,
        text=True,
        timeout=timeout_sec,
    )
    return parse_move(result.stdout), result.stdout, result.stderr


def play_one_game(black_exec, white_exec, timeout_sec, verbose=False):
    board = [[0 for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]
    history = []
    turn = BLACK
    max_turns = BOARD_SIZE * BOARD_SIZE

    for ply in range(max_turns):
        moves = legal_moves(board, turn)
        if not moves:
            winner = -turn
            loser_name = "black" if turn == BLACK else "white"
            return {
                "winner": winner,
                "reason": f"{loser_name}_no_legal_move",
                "history": history[:],
                "final_board": [row[:] for row in board],
            }

        payload = history_to_payload(history, turn)
        exe = black_exec if turn == BLACK else white_exec

        try:
            (x, y), stdout_text, stderr_text = call_bot(exe, payload, timeout_sec)
        except subprocess.TimeoutExpired:
            winner = -turn
            loser_name = "black" if turn == BLACK else "white"
            return {
                "winner": winner,
                "reason": f"{loser_name}_timeout",
                "history": history[:],
                "final_board": [row[:] for row in board],
            }
        except Exception as exc:
            winner = -turn
            loser_name = "black" if turn == BLACK else "white"
            return {
                "winner": winner,
                "reason": f"{loser_name}_bad_output:{exc}",
                "history": history[:],
                "final_board": [row[:] for row in board],
            }

        if not is_legal_move(board, x, y, turn):
            winner = -turn
            loser_name = "black" if turn == BLACK else "white"
            return {
                "winner": winner,
                "reason": f"{loser_name}_illegal_move:{x},{y}",
                "history": history[:],
                "stdout": stdout_text,
                "stderr": stderr_text,
                "final_board": [row[:] for row in board],
            }

        board[x][y] = turn
        history.append((x, y))
        if verbose:
            side = "B" if turn == BLACK else "W"
            print(f"ply {ply + 1:02d}: {side} -> ({x}, {y})")
        turn = -turn

    return {
        "winner": 0,
        "reason": "draw_like_limit_reached",
        "history": history[:],
        "final_board": [row[:] for row in board],
    }


def run_series(exec_a, exec_b, games, timeout_sec, verbose=False):
    stats = {
        "A_wins": 0,
        "B_wins": 0,
        "A_black_wins": 0,
        "A_white_wins": 0,
        "B_black_wins": 0,
        "B_white_wins": 0,
        "details": [],
    }

    for game_index in range(games):
        a_is_black = (game_index % 2 == 0)
        black_exec = exec_a if a_is_black else exec_b
        white_exec = exec_b if a_is_black else exec_a

        result = play_one_game(black_exec, white_exec, timeout_sec, verbose=verbose)
        winner = result["winner"]

        if winner == BLACK:
            winner_name = "A" if a_is_black else "B"
        elif winner == WHITE:
            winner_name = "B" if a_is_black else "A"
        else:
            winner_name = "None"

        if winner_name == "A":
            stats["A_wins"] += 1
            if a_is_black:
                stats["A_black_wins"] += 1
            else:
                stats["A_white_wins"] += 1
        elif winner_name == "B":
            stats["B_wins"] += 1
            if a_is_black:
                stats["B_white_wins"] += 1
            else:
                stats["B_black_wins"] += 1

        record = {
            "game": game_index + 1,
            "A_color": "black" if a_is_black else "white",
            "winner": winner_name,
            "reason": result["reason"],
            "moves": len(result["history"]),
            "history": result["history"],
            "final_board": result.get("final_board"),
        }
        if "stdout" in result:
            record["stdout"] = result["stdout"]
        if "stderr" in result:
            record["stderr"] = result["stderr"]
        stats["details"].append(record)
        print(
            f"Game {game_index + 1:03d} | "
            f"A={record['A_color']:>5s} | "
            f"winner={winner_name:>4s} | "
            f"moves={record['moves']:>2d} | "
            f"{record['reason']}"
        )

    return stats


def main():
    parser = argparse.ArgumentParser(
        description="Run NoGo self-play matches between two cpp programs or executables."
    )
    parser.add_argument("bot_a", help="Path to bot A (.cpp or executable)")
    parser.add_argument("bot_b", help="Path to bot B (.cpp or executable)")
    parser.add_argument("--games", type=int, default=20, help="Number of games to run")
    parser.add_argument("--timeout", type=float, default=1.2, help="Per-move timeout in seconds")
    parser.add_argument("--flags-a", default="", help='Extra compile flags for bot A, e.g. "-ljsoncpp"')
    parser.add_argument("--flags-b", default="", help='Extra compile flags for bot B, e.g. "-ljsoncpp"')
    parser.add_argument("--build-dir", default=".duel_build", help="Where to place compiled binaries")
    parser.add_argument("--verbose", action="store_true", help="Print every move")
    parser.add_argument("--save-log", default="", help="Save full match log to a JSON file")
    args = parser.parse_args()

    build_dir = os.path.abspath(args.build_dir)
    os.makedirs(build_dir, exist_ok=True)

    try:
        exec_a = prepare_bot(args.bot_a, build_dir, "bot_a", args.flags_a)
        exec_b = prepare_bot(args.bot_b, build_dir, "bot_b", args.flags_b)
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 2

    stats = run_series(exec_a, exec_b, args.games, args.timeout, verbose=args.verbose)

    if args.save_log:
        log_path = os.path.abspath(args.save_log)
        payload = {
            "bot_a": os.path.abspath(args.bot_a),
            "bot_b": os.path.abspath(args.bot_b),
            "games": args.games,
            "timeout": args.timeout,
            "flags_a": args.flags_a,
            "flags_b": args.flags_b,
            "summary": {
                "A_wins": stats["A_wins"],
                "B_wins": stats["B_wins"],
                "A_black_wins": stats["A_black_wins"],
                "A_white_wins": stats["A_white_wins"],
                "B_black_wins": stats["B_black_wins"],
                "B_white_wins": stats["B_white_wins"],
            },
            "games_detail": stats["details"],
        }
        Path(log_path).write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"log saved to: {log_path}")

    print("\nSummary")
    print(f"A wins        : {stats['A_wins']}")
    print(f"B wins        : {stats['B_wins']}")
    print(f"A as black    : {stats['A_black_wins']}")
    print(f"A as white    : {stats['A_white_wins']}")
    print(f"B as black    : {stats['B_black_wins']}")
    print(f"B as white    : {stats['B_white_wins']}")

    if stats["A_wins"] + stats["B_wins"] > 0:
        total = stats["A_wins"] + stats["B_wins"]
        print(f"A win rate    : {stats['A_wins'] / total:.2%}")
        print(f"B win rate    : {stats['B_wins'] / total:.2%}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
