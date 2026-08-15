#!/usr/bin/env python3
import os
import re

def find_and_bump_version(root_dir=".", filename="version.h"):
    version_pattern = re.compile(r'#define\s+VERSION\s+"(\d+)\.(\d+)\.(\d+)"')

    for dirpath, _, files in os.walk(root_dir):
        if filename in files:
            filepath = os.path.join(dirpath, filename)
            with open(filepath, "r") as f:
                lines = f.readlines()

            new_lines = []
            bumped = False
            for line in lines:
                match = version_pattern.search(line)
                if match:
                    major, minor, patch = map(int, match.groups())
                    patch += 1
                    new_version = f'#define VERSION "{major}.{minor}.{patch}"\n'
                    new_lines.append(new_version)
                    bumped = True
                    print(f"Bumped version in {filepath} to {major}.{minor}.{patch}")
                else:
                    new_lines.append(line)

            if bumped:
                with open(filepath, "w") as f:
                    f.writelines(new_lines)
                return

    print(f"No version string found in any '{filename}' under '{root_dir}'.")

if __name__ == "__main__":
    find_and_bump_version()