#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
: "${PT_KEYSTORE:?set PT_KEYSTORE to the persistent test signing key}"
: "${PT_STORE_PASSWORD:?set PT_STORE_PASSWORD}"
export PT_VERSION="${PT_VERSION:-0.2.1-test.2}"
PT_TEST_CODE="${PT_VERSION_CODE:-44}"
bash "$ROOT/tools/android/build_native.sh"
cd "$ROOT/android"
./gradlew :app:assembleRelease -PptApplicationId=com.ptport.app.test '-PptAppLabel=P.T. Test' \
  "-PptVersionName=$PT_VERSION" "-PptVersionCode=$PT_TEST_CODE" --no-daemon --max-workers=2
