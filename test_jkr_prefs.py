from __future__ import annotations

import argparse
from pathlib import Path


PREFS_FILE = "jkr_prefs.txt"


def parse_prefs_file(pref_file: str) -> int:
    """Mimic the plugin's setup() file-open behavior and show what path was used."""
    print(f"Requested prefFile: {pref_file}")
    print(f"Current working dir: {Path.cwd()}")

    path = Path(pref_file)
    print(f"Resolved path: {path.resolve(strict=False)}")

    if not path.exists():
        print("OPEN FAILED: file does not exist at the requested path.")
        return 1

    if not path.is_file():
        print("OPEN FAILED: requested path is not a regular file.")
        return 1

    print("OPEN OK")
    print("Parsed records:")

    count = 0
    with path.open("r", encoding="utf-8") as handle:
        for line_no, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line:
                continue

            parts = line.split()
            if len(parts) != 2:
                print(f"  line {line_no}: invalid format -> {raw_line.rstrip()}")
                continue

            pair, surface_energy = parts
            if ":" not in pair:
                print(f"  line {line_no}: missing ':' -> {raw_line.rstrip()}")
                continue

            surf_a, surf_b = pair.split(":", 1)
            print(
                f"  line {line_no}: surfA={surf_a}, surfB={surf_b}, surfaceEnergy={surface_energy}"
            )
            count += 1

    print(f"Total parsed records: {count}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Standalone check for the jkr_prefs.txt lookup used by the JKR plugin."
    )
    parser.add_argument(
        "pref_file",
        nargs="?",
        default=PREFS_FILE,
        help="Path passed to setup(prefFile). Defaults to jkr_prefs.txt.",
    )
    args = parser.parse_args()
    return parse_prefs_file(args.pref_file)


if __name__ == "__main__":
    raise SystemExit(main())
