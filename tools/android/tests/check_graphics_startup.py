#!/usr/bin/env python3
"""Exercise pending launcher graphics through the real pre/post-init startup code."""
from pathlib import Path
import os
import resource
import shlex
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
revision = os.environ.get("PT_STARTUP_SOURCE_REF")


def source(path):
    if revision:
        return subprocess.check_output(["git", "show", f"{revision}:{path}"], cwd=repo, text=True)
    return (repo / path).read_text()


def extract(path, signature):
    text = source(path)
    start = text.index(signature)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


apply = extract("src/main.cpp", "void ApplyMobileGameGraphicsChanges(")
options = "src/engine/platform/android_options.cpp"
methods = [extract(options, signature) for signature in [
    "std::filesystem::path PrivateFile(", "float ReadNumber(", "void Load()",
    "bool ConsumeGameGraphicsChanges()", "std::filesystem::path GameGraphicsChangesFile()",
]]
if "GameGraphicsStartupFile" in source(options):
    methods.append(extract(options, "std::filesystem::path GameGraphicsStartupFile()"))
fixture = (repo / "tests/graphics_startup_test.cpp.in").read_text()
fixture = fixture.replace("/* ANDROID_OPTIONS */", "\n".join(methods))
fixture = fixture.replace("/* APPLY_GRAPHICS */", apply)
fixture = fixture.replace("/* RAY_SUPPORT */", extract(
    "src/engine/render/scene_renderer.cpp", "bool SceneRenderer::RayTracingSupported("))
fixture = fixture.replace("/* PREPARE_ARGUMENT */", ", true" if "prepare_only" in apply else "")
compiler = [os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-pthread"]
compiler += shlex.split(os.environ.get("PT_TEST_CXX_FLAGS", ""))
compiler += ["-I" + str(repo / "src")]
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    cpp = root / "startup.cpp"
    cpp.write_text(fixture)
    binary = root / "startup"
    subprocess.run(compiler + ["-D__ANDROID__", str(cpp),
        str(repo / "src/engine/platform/settings.cpp"), "-o", str(binary)], check=True, timeout=120)
    for mode in ["pending", "recovered", "none", "save-failure", "newer", "corrupt", "init-failure"]:
        folder = root / mode
        folder.mkdir()
        subprocess.run([str(binary), str(folder), mode], check=True, timeout=30)
