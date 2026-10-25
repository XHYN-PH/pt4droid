#!/usr/bin/env python3
"""Exercise stable templates, conservative light coverage, pacing and log policy."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
includes = ['src', 'build/android/_deps/glm-src', 'build/android/_deps/volk-src',
            'build/android/_deps/vulkan_headers-src/include']
compiler = [os.environ.get('CXX', 'g++'), '-std=c++20', '-O2', '-DVK_NO_PROTOTYPES']
compiler += shlex.split(os.environ.get('PT_TEST_CXX_FLAGS', ''))
compiler += ['-I' + str(repo / path) for path in includes]
with tempfile.TemporaryDirectory() as directory:
    binary = Path(directory) / 'checks'
    subprocess.run(compiler + [str(repo / 'tests/mobile_refinements24_test.cpp'), '-o', str(binary)],
                   check=True, timeout=120)
    subprocess.run([str(binary)], check=True, timeout=30)
