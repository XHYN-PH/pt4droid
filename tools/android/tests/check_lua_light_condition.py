#!/usr/bin/env python3
"""Reproduce the original-data spelling mismatch without distributing game data."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo / 'tools'))
import fox2


def extract(text, signature):
    start = text.index(signature)
    pos = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}')
        pos += 1
    return text[start:pos]


def fixture():
    entities, literals = [], set()

    def prop(name, kind, values, container='StaticArray'):
        literals.add(name)
        if kind in ('String', 'Path', 'FilePtr'):
            literals.update(values)
        return dict(name=name, type=kind, values=values, container=container)

    def entity(name, kind, static=(), dynamic=()):
        literals.add(kind)
        address = 0x1000 + len(entities) * 0x100
        entities.append(dict(address=address, id=len(entities), classId=0, classVersion=0,
                             **{'class': kind}, static=[prop('name', 'String', [name]), *static], dynamic=list(dynamic)))
        return address

    a = entity('modelA', 'StaticModel', [prop('flags', 'uint32', [7])])
    b = entity('modelB', 'StaticModel', [prop('flags', 'uint32', [7])])
    canonical = entity('canonicalModel', 'StaticModel', [prop('flags', 'uint32', [7])])
    light = entity('sceneLight', 'PointLight', [prop('enable', 'bool', [True])])
    script = '/Assets/sh/level_asset/promotion/pt_2014/script/trapLightEnable.lua'
    callback = entity('callback', 'GeoTrapScriptCallbackDataElement', [prop('scriptFile', 'FilePtr', [script])])
    short = entity('shortCallback', 'GeoTrapScriptCallbackDataElement', [prop('scriptFile', 'FilePtr', ['trapLightEnable.lua'])])
    other = entity('otherCallback', 'GeoTrapScriptCallbackDataElement', [prop('scriptFile', 'FilePtr', [script + '.bak'])])
    wrong = entity('wrongCallbackType', 'OtherCallback', [prop('scriptFile', 'FilePtr', [script])])

    def links(name, addresses):
        return prop(name, 'EntityLink', [dict(packagePath=None, archivePath=None, nameInArchive=None,
                                            entityHandle=x) for x in addresses], 'List')

    def condition(name, callbacks=(callback,), models=(a, b), canonical_models=None, kind='GeoModuleCondition', static_typo=False):
        static = [prop('execCallbackDataElements', 'EntityPtr',
                       [{'__entity_address__': x} for x in callbacks], 'List')]
        dynamic = [links('lightData', [light])]
        if models is not None:
            (static if static_typo else dynamic).append(links('modleData', models))
        if canonical_models is not None:
            dynamic.append(links('modelData', canonical_models))
        entity(name, kind, static, dynamic)

    condition('enable')
    condition('disable', callbacks=(0, callback))
    condition('canonical', canonical_models=(canonical,))
    condition('canonicalEmpty', canonical_models=())
    condition('legacyEmpty', models=())
    condition('shortPath', callbacks=(short,))
    condition('otherScript', callbacks=(other,))
    condition('missingCallback', callbacks=())
    condition('wrongCallback', callbacks=(wrong,))
    condition('wrongClass', kind='ShDemoScript')
    condition('staticTypo', static_typo=True)
    condition('missingList', models=None)
    return fox2.serialize_document(dict(entities=entities, literals=[{'text': x} for x in sorted(literals)]))


with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    data = root / 'hallway.fox2'
    data.write_bytes(fixture())
    lua = repo / 'build/android/_deps/lua51-src/src'
    cc = [os.environ.get('CC', 'gcc'), '-std=c99', '-O2', '-DLUA_USE_POSIX', '-D_POSIX_C_SOURCE=200809L', '-I' + str(lua)]
    objects = []
    for source in sorted(lua.glob('*.c')):
        if source.stem in ('lua', 'luac', 'print'):
            continue
        obj = root / (source.stem + '.o')
        subprocess.run(cc + ['-c', str(source), '-o', str(obj)], check=True, timeout=60)
        objects.append(str(obj))
    source = (repo / 'src/game/stage_manager.cpp').read_text()
    stage = root / 'stage.cpp'
    stage.write_text('#include "game/stage_manager.h"\nnamespace pt::game {\n' + '\n'.join(
        extract(source, x) for x in ('const StageData* Stage::FileOf(', 'BodyState& Stage::Body(')) + '\n}\n')
    includes = ['-I' + str(repo / 'src'), '-I' + str(lua)] + [
        '-I' + str(repo / 'build/android/_deps' / name) for name in
        ('glm-src', 'vulkan_headers-src/include', 'volk-src', 'sdl-src/include', 'vma-src/include')]
    command = [os.environ.get('CXX', 'g++'), '-std=c++20', '-O2', '-pthread'] + shlex.split(
        os.environ.get('PT_TEST_CXX_FLAGS', '')) + includes
    common = [str(repo / x) for x in ('tests/lua_light_condition_test.cpp', 'src/engine/data/fox2.cpp',
                                      'src/engine/script/lua_vm.cpp')] + [str(stage)] + objects + ['-ldl', '-lm']
    binary = root / 'light-conditions'
    subprocess.run(command + common + [str(repo / 'src/game/lua_entity.cpp'), '-o', str(binary)], check=True, timeout=180)
    subprocess.run([str(binary), str(data)], check=True, timeout=30)
