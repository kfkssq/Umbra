"""Compatibility entry point: verify all nineteen fixed-grade startup stones.

The previous forced-random-rarity diagnostic is superseded by the approved grade rule.
"""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("smoke_stone_startup_pie.py")), run_name="__main__")
