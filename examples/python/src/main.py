#!/usr/bin/env python3
# examples/python/src/main.py
"""Entry point for the Horcrux Python example binary."""

from greet import greet


def main() -> None:
    print(greet("World"))


if __name__ == "__main__":
    main()
