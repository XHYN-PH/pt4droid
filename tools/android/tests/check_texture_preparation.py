#!/usr/bin/env python3
"""Exercise real FTEX and texture-loading code without game files or a GPU."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
def extract(text, signature):
    start = text.index(signature); pos = text.index('{', start) + 1; depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}'); pos += 1
    return text[start:pos]

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    source = (repo / 'src/engine/render/texture_manager.cpp').read_text()
    cpp = root / 'preparation.cpp'
    cpp.write_text((repo / 'tests/texture_preparation_test.cpp').read_text() + '\nnamespace pt {\n' +
                   '\n'.join(extract(source, name) for name in ['uint32_t FormatBlockBytes(', 'uint32_t TextureManager::LoadFox(',
                   'bool TextureManager::AdoptDecoded(', 'void TextureManager::DropDecoded(',
                   'void TextureManager::DecodeAhead(', 'uint32_t TextureManager::PumpDecoded(', 'void TextureManager::StopDecodeWorkers(']) + '\n}\n')
    includes = ['-I' + str(repo / 'src'), '-I' + str(repo / 'build/android/_deps/glm-src'),
                '-I' + str(repo / 'build/android/_deps/volk-src'), '-I' + str(repo / 'build/android/_deps/vulkan_headers-src/include')]
    compiler = [os.environ.get('CXX', 'g++')] + shlex.split(os.environ.get('PT_TEST_CXX_FLAGS', ''))
    command = compiler + ['-std=c++20', '-O2', '-pthread', '-DVK_NO_PROTOTYPES'] + includes + [str(cpp),
              str(repo / 'src/engine/assets/ftex.cpp'), str(repo / 'src/engine/core/pathcode.cpp'),
              str(repo / 'src/engine/assets/compatible_texture.cpp'), str(repo / 'src/engine/assets/bc_texture.cpp'),
              str(repo / 'src/engine/core/strcode.cpp'), '-lz', '-o', str(root / 'checks')]
    subprocess.run(command, check=True, timeout=120)
    subprocess.run([str(root / 'checks')], check=True, timeout=30)
