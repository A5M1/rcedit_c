set -e
TESTDIR=$(cd "$(dirname "$0")" && pwd)
cd "$TESTDIR"

make

make -C ..

../rcedit.exe test_prog.exe --set-file-version 2.3.4.5 --set-requested-execution-level highestAvailable

VAL=$(../rcedit.exe test_prog.exe --get-version-string FileVersion)
if [[ "$VAL" != "2.3.4.5" ]]; then
  echo "Version string mismatch: $VAL"
  exit 1
fi

echo "All tests passed"

