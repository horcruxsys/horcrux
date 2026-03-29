"""Unit tests for the greet module."""

import unittest

from greet import greet


class GreetTest(unittest.TestCase):
    def test_greet_world(self) -> None:
        self.assertEqual(greet("World"), "Hello, World!")

    def test_greet_horcrux(self) -> None:
        self.assertEqual(greet("Horcrux"), "Hello, Horcrux!")

    def test_greet_empty(self) -> None:
        self.assertEqual(greet(""), "Hello, !")


if __name__ == "__main__":
    unittest.main()
