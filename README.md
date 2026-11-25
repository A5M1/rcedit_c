# rcedit (C port)

A C port of [rcedit](https://github.com/electron/rcedit): a command-line tool for editing resources in Windows PE files (`.exe`, `.dll`).

## Features

- Set file and product versions (fixed version info and version strings)
- Get and set arbitrary version strings
- Replace the application icon
- Set the requested execution level in the manifest
- Replace the manifest with an external file
- Get and set `RT_STRING` resources
- Replace existing `RT_RCDATA` resources

## Requirements

- Windows
- One of:
  - MinGW-w64 (`x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-windres`) and GNU Make
  - MSVC (Visual Studio Build Tools / Visual Studio)

## Build

### MinGW-w64

```sh
make
```

This produces `rcedit.exe` in the project root. Object files are written to `build/obj/`.

```sh
make clean
```

### MSVC

From a Developer Command Prompt:

```bat
mkdir build\obj
rc /nologo /fo build\obj\rcedit.res src\rcedit.rc
cl /nologo /O2 /W4 /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS ^
  /Fo:build\obj\ ^
  src\main.c src\containers.c src\version.c src\resources.c src\utils.c ^
  build\obj\rcedit.res ^
  /Fe:rcedit.exe /link /SUBSYSTEM:CONSOLE user32.lib version.lib
```

## Usage

```text
rcedit.exe <filename> [options...]
```

Run with `-h` or `--help` to print the built-in help.

### Options

| Option | Alias | Arguments | Description |
| --- | --- | --- | --- |
| `--set-file-version` | `-sfv` | `<version>` | Set the file version and the `FileVersion` string |
| `--set-product-version` | `-spv` | `<version>` | Set the product version and the `ProductVersion` string |
| `--set-version-string` | `-svs` | `<key> <value>` | Set a version string; updates fixed info for `FileVersion`/`ProductVersion` |
| `--get-version-string` | `-gvs` | `<key>` | Print a version string |
| `--set-icon` | `-si` | `<path>` | Replace the application icon |
| `--set-requested-execution-level` | `-srel` | `<level>` | Set `requestedExecutionLevel` |
| `--application-manifest` | `-am` | `<path>` | Replace the application manifest |
| `--set-resource-string` | `--srs` | `<id> <value>` | Set an `RT_STRING` entry |
| `--get-resource-string` | `-grs` | `<id>` | Print an `RT_STRING` entry |
| `--set-rcdata` | | `<id> <path>` | Set or replace an `RT_RCDATA` resource |

`--set-requested-execution-level` accepts `asInvoker`, `highestAvailable`, or `requireAdministrator`.

### Examples

Set both file and product versions:

```sh
rcedit.exe app.exe --set-file-version 1.2.3.4 --set-product-version 1.2.3.4
```

Update a version string:

```sh
rcedit.exe app.exe --set-version-string CompanyName "Example Corp"
rcedit.exe app.exe --get-version-string CompanyName
```

Set the icon and require administrator privileges:

```sh
rcedit.exe app.exe --set-icon app.ico --set-requested-execution-level requireAdministrator
```

Replace the manifest:

```sh
rcedit.exe app.exe --application-manifest app.manifest
```

Update a string table entry and an RCDATA resource:

```sh
rcedit.exe app.exe --set-resource-string 7 "New string"
rcedit.exe app.exe --set-rcdata 101 payload.bin
```

## Tests

```sh
cd tests
make
cd ..
make
cd tests
bash run_tests.sh
```

On Windows, the equivalent batch script is:

```bat
tests\run_tests.bat
```

The tests build `tests/test_prog.exe`, modify its version and manifest, then verify the `FileVersion` string.

## Project layout

```text
.
├── Makefile
├── src
│   ├── containers.c     ByteVec and resource map containers
│   ├── main.c           Command-line parsing and dispatch
│   ├── rcedit.h         Shared types and API declarations
│   ├── rcedit.rc        Version resource for rcedit.exe
│   ├── resources.c      ResourceUpdater load/get/set/commit logic
│   ├── utils.c          File and string helpers
│   └── version.c        VS_VERSIONINFO parsing and serialization
└── tests
    ├── Makefile
    ├── manifest.xml
    ├── run_tests.bat
    ├── run_tests.sh
    ├── test_prog.c
    └── test_prog.rc
```

## Notes

- Existing version resource IDs, manifest IDs, and manifest languages are preserved.
- New version resources use ID `1`; new manifests use ID `1` and language `1033` (en-US).
- `--set-rcdata` replaces an existing resource or creates a new one under the file's default language.
- Get options can be combined with set options. If only get options are used, the file is not modified.
- `--set-version-string` updates the fixed file/product version when the key is `FileVersion` or `ProductVersion` and the value parses as a version.

## License

Derived from rcedit, which is released under the MIT license.
