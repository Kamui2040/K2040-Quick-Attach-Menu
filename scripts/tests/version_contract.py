"""Validate the source, packed native and xmake versions together."""
import re

def assert_version_consistency(testcase, root):
    build = (root / "xmake.lua").read_text()
    match = re.search(r'set_version\("(\d+)\.(\d+)\.(\d+)"\)', build)
    testcase.assertIsNotNone(match)
    major, minor, patch = map(int, match.groups())
    testcase.assertLessEqual(patch, 0xFFFF)
    version = f"{major}.{minor}.{patch}"
    packed = (major << 24) | (minor << 16) | patch
    testcase.assertIn(f"0x{packed:08X}; // {version}", (root / "src/main.cpp").read_text())
    testcase.assertEqual(build.count(version), 3)
