#!/usr/bin/env python3
"""Build with an explicitly supplied signing kit without printing its passwords."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("signing_kit", type=Path, help="Extracted signing-kit directory")
    parser.add_argument("--version", default="0.2.1-test.26")
    parser.add_argument("--version-code", type=int, default=69)
    parser.add_argument("--verify-only", action="store_true", help="Verify the keystore and print only its certificate fingerprint")
    args = parser.parse_args()
    if args.version_code <= 0:
        parser.error("version code must be positive")
    kit = args.signing_kit.resolve()
    keystore = kit / "pt-android-test.jks"
    try:
        credentials = json.loads((kit / "credentials.json").read_text(encoding="utf-8"))
        password, alias = credentials["store_password"], credentials["alias"]
        key_password = credentials.get("key_password", password)
        if not keystore.is_file() or not all(isinstance(value, str) and value for value in (password, alias, key_password)):
            raise ValueError("invalid kit")
    except (OSError, ValueError, KeyError, TypeError):
        parser.error("cannot load the signing kit; check its keystore and credentials.json")
    env = os.environ.copy()
    env.update(PT_KEYSTORE=str(keystore), PT_STORE_PASSWORD=password,
               PT_KEY_PASSWORD=key_password, PT_KEY_ALIAS=alias,
               PT_VERSION=args.version, PT_VERSION_CODE=str(args.version_code))
    if args.verify_only:
        keytool = shutil.which("keytool")
        if not keytool:
            parser.error("keytool is missing; install JDK 17 and put its bin directory on PATH")
        result = subprocess.run([keytool, "-J-Duser.language=en", "-J-Duser.country=US",
                                 "-list", "-v", "-keystore", str(keystore),
                                 "-storepass:env", "PT_STORE_PASSWORD", "-alias", alias],
                                env=env, capture_output=True, text=True)
        fingerprint = re.search(r"SHA256:\s*([0-9A-Fa-f:]+)", result.stdout)
        if result.returncode or not fingerprint:
            parser.error("keytool could not verify this signing kit; check the supplied credentials")
        print("Signing certificate SHA-256: " + fingerprint.group(1).replace(":", "").lower())
        return 0
    repo = Path(__file__).resolve().parents[2]
    return subprocess.run(["bash", str(repo / "tools/android/build_test_release.sh")],
                          cwd=repo, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
