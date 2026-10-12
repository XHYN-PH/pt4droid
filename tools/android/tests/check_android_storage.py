#!/usr/bin/env python3
"""Execute production native storage selection, including absent and unusable external storage."""
from pathlib import Path
import os, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
source=(repo/'src/engine/platform/android.cpp').read_text()
start=source.index('std::filesystem::path DataDir()');at=source.index('{',start)+1;depth=1
while depth:
    depth+=(source[at]=='{')-(source[at]=='}');at+=1
function=source[start:at]
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary);file=root/'check.cpp'
    file.write_text(r'''#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cassert>
std::string external,internal;const char* SDL_GetAndroidExternalStoragePath(){return external.empty()?nullptr:external.c_str();}
const char* SDL_GetAndroidInternalStoragePath(){return internal.c_str();}
'''+function+r'''
int main(int argc,char** argv){assert(argc==4);external=argv[1];internal=argv[2];auto selected=DataDir();assert(selected==std::filesystem::path(argv[3]));
 assert(DataDir()==selected);if(selected==external)assert(std::filesystem::is_empty(selected));puts("Native app storage check passed");}
''')
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20',str(file),'-o',str(root/'check')],check=True,timeout=30)
    private=root/'internal';private.mkdir();external=root/'external';blocked=root/'blocked';blocked.write_text('not a directory')
    for value,expected in [('',private),(external,external),(blocked,private)]:
        subprocess.run([str(root/'check'),str(value),str(private),str(expected)],check=True,timeout=10)
