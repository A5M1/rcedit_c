@echo off
pushd %~dp0

make

pushd ..
make
popd

..\rcedit.exe test_prog.exe --set-file-version 2.3.4.5 --set-requested-execution-level highestAvailable

for /f "usebackq tokens=*" %%a in (`..\rcedit.exe test_prog.exe --get-version-string FileVersion`) do set VAL=%%a

if "%VAL%"=="2.3.4.5" (
    echo All tests passed
) else (
    echo Version string mismatch: %VAL%
    exit /b 1
)

popd

