#!/usr/bin/env python3
"""Package staged Android resources/libraries using the official SDK tools.

For environments where Gradle cannot download dependencies. Native libraries and
assets must first be staged by build_native.sh. Signing passwords come from the
same environment variables as the Gradle build, never command-line literals.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ANDROID = 'http://schemas.android.com/apk/res/android'

def run(*command):
    subprocess.run([str(x) for x in command], check=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sdk', default=os.environ.get('ANDROID_HOME'))
    parser.add_argument('--java-home', default=os.environ.get('JAVA_HOME'))
    parser.add_argument('--version', default=os.environ.get('PT_VERSION', '0.2.1-test.17'))
    parser.add_argument('--version-code', type=int, default=int(os.environ.get('PT_VERSION_CODE', '59')))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--native-build', type=Path, default=Path(os.environ.get('BUILD_DIR', ROOT/'build/android')))
    args = parser.parse_args()
    if not args.sdk or not args.java_home:
        parser.error('Set ANDROID_HOME and JAVA_HOME, or pass --sdk and --java-home')
    sdk, java = Path(args.sdk), Path(args.java_home)
    build = sdk / 'build-tools/35.0.0'
    android = sdk / 'platforms/android-35/android.jar'
    app = ROOT / 'android/app'
    args.output = args.output.resolve()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, JAVA_HOME=str(java), PATH=str(java/'bin')+os.pathsep+os.environ['PATH'])
    with tempfile.TemporaryDirectory(prefix='sdk-package-', dir=ROOT/'build') as temp:
        work = Path(temp)
        tree = ET.parse(app/'src/main/AndroidManifest.xml')
        manifest = tree.getroot()
        manifest.set('package', 'com.ptport.app.test')
        for node in manifest.iter():
            for key, value in list(node.attrib.items()):
                value = value.replace('${applicationId}', 'com.ptport.app.test').replace('${ptAppLabel}', 'pt4droid')
                if key == '{'+ANDROID+'}name' and value.startswith('.'):
                    value = 'com.ptport.app'+value
                node.set(key, value)
        ET.register_namespace('android', ANDROID)
        tree.write(work/'AndroidManifest.xml', encoding='utf-8', xml_declaration=True)
        run(build/'aapt2', 'compile', '--dir', app/'src/main/res', '-o', work/'resources.zip')
        command = [build/'aapt2', 'link', '-o', work/'resources.apk', '--manifest', work/'AndroidManifest.xml', '-I', android,
                   '-R', work/'resources.zip', '--java', work/'generated', '--custom-package', 'com.ptport.app',
                   '--min-sdk-version', '29', '--target-sdk-version', '35', '--version-code', args.version_code,
                   '--version-name', args.version, '--auto-add-overlay']
        if not args.compile_only:
            if not (app/'assets/pt_assets.txt').is_file():
                raise RuntimeError('Stage native assets with build_native.sh first')
            command += ['-A', app/'assets']
        run(*command)
        classes = work/'classes'
        classes.mkdir()
        # The repository includes the SDL Java sources with its own logging policy.
        sources = sorted((app/'src/main/java').rglob('*.java')) + sorted((work/'generated').rglob('*.java'))
        boot = os.pathsep.join(str(x) for x in [android, build/'core-lambda-stubs.jar'])
        run(java/'bin/javac', '-encoding', 'UTF-8', '-source', '8', '-target', '8', '-bootclasspath', boot, '-d', classes, *sources)
        dex = work/'dex'
        dex.mkdir()
        class_list = work/'classes.txt'
        class_list.write_text(''.join(str(f)+'\n' for f in sorted(classes.rglob('*.class'))))
        swappy=args.native_build/'_deps/pt_swappy_archive-src/classes.jar'
        if not swappy.is_file():raise RuntimeError('Swappy Java runtime is missing; configure the native build first')
        run(java/'bin/java', '-Xmx1g', '-cp', build/'lib/d8.jar', 'com.android.tools.r8.D8', '--lib', android,
            '--min-api', '29', '--output', dex, '@'+str(class_list), swappy)
        if args.compile_only:
            print('Android Java and DEX compilation passed')
            return
        shutil.copyfile(work/'resources.apk', work/'unsigned.apk')
        with zipfile.ZipFile(work/'unsigned.apk', 'a', compression=zipfile.ZIP_DEFLATED) as apk:
            for file in sorted(dex.glob('*.dex')):
                apk.write(file, file.name)
            libraries = sorted((app/'libs/arm64-v8a').glob('*.so'))
            if not any(f.name == 'libmain.so' for f in libraries):
                raise RuntimeError('Native libraries have not been staged')
            for file in libraries:
                apk.write(file, 'lib/arm64-v8a/'+file.name)
        run(build/'zipalign', '-P', '16', '-f', '4', work/'unsigned.apk', work/'aligned.apk')
        key = os.environ['PT_KEYSTORE']
        alias = os.environ.get('PT_KEY_ALIAS', 'pt-test')
        # Pass env references to apksigner; do not expose credentials in process arguments.
        if 'PT_KEY_PASSWORD' not in env:
            env['PT_KEY_PASSWORD'] = env['PT_STORE_PASSWORD']
        subprocess.run([str(java/'bin/java'), '-jar', str(build/'lib/apksigner.jar'), 'sign', '--ks', key,
                        '--ks-key-alias', alias, '--ks-pass', 'env:PT_STORE_PASSWORD', '--key-pass', 'env:PT_KEY_PASSWORD',
                        '--out', str(args.output), str(work/'aligned.apk')], env=env, check=True)
        run(java/'bin/java', '-jar', build/'lib/apksigner.jar', 'verify', '--verbose', '--print-certs', args.output)
        run(build/'zipalign', '-c', '-P', '16', '4', args.output)
        print(args.output)

if __name__ == '__main__':
    main()
