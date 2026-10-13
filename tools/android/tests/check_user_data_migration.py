#!/usr/bin/env python3
"""Reproduce the reported startup error and execute the real Android migration fix."""
from pathlib import Path
import errno, os, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    baseline = root / 'baseline.cpp'
    baseline.write_bytes(subprocess.check_output(
        ['git', 'show', 'pt4droid-v0.2.1-test.8:src/engine/platform/user_data.cpp'], cwd=repo))
    compiler = [os.environ.get('CXX', 'g++')] + shlex.split(os.environ.get('PT_TEST_CXX_FLAGS', ''))
    for name, source in [('baseline', baseline), ('fixed', repo / 'src/engine/platform/user_data.cpp')]:
        binary = root / name
        subprocess.run(compiler + ['-std=c++20', '-O2', '-D__ANDROID__', '-I' + str(repo / 'src'),
                       str(repo / 'tests/android_user_data_test.cpp'), str(source),
                       '-Wl,--wrap=syscall', '-Wl,--wrap=link', '-o', str(binary)], check=True, timeout=60)
        for error in [errno.ENOSYS, errno.EINVAL, errno.EPERM]:
            folder = root / (name + '-' + str(error)); folder.mkdir()
            subprocess.run([str(binary), str(folder), name, str(error)], check=True, timeout=15)
