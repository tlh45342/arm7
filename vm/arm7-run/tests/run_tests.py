import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: run_tests.py <arm7-run>")
        return 2

    runner = sys.argv[1]

    try:
        proc = subprocess.run(
            [runner],
            input="quit\n",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=10,
        )
    except FileNotFoundError:
        print(f"FAIL: runner not found: {runner}")
        return 1
    except subprocess.TimeoutExpired:
        print("FAIL: arm7-run did not exit after 'quit'")
        return 1

    if proc.returncode != 0:
        print(proc.stdout)
        print(f"FAIL: arm7-run returned {proc.returncode}")
        return 1

    print("arm7-run startup/quit smoke test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
