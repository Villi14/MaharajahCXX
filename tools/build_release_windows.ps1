# Builds the Windows release binaries: static C++ runtime (no Visual C++ Redistributable
# needed), one archive per target in dist\ (maharajah-<version>-<target>.zip with
# Maharajah.exe and LICENSE).
# Usage (Developer PowerShell for VS):
#   tools\build_release_windows.ps1 windows-x64 windows-x64-avx2 windows-arm64
#
# Each target is built twice: build-rel-<target>-tests with the tests (googletest is
# forced onto the DLL runtime) and build-rel-<target> with the engine alone on the static
# runtime. The tests and the search check run only on a host of the target's architecture.
param(
  [Parameter(Mandatory, ValueFromRemainingArguments)][string[]]$Targets,
  [string]$Generator = "Visual Studio 18 2026"
)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

function Invoke-Native {
  $command, $arguments = $args
  & $command @arguments
  if ($LASTEXITCODE) { throw "$command $arguments failed with exit code $LASTEXITCODE" }
}

$version = (Select-String -Path CMakeLists.txt -Pattern 'project\(Maharajah VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$hostArch = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") { "ARM64" } else { "x64" }
New-Item -ItemType Directory -Force dist | Out-Null

foreach ($target in $Targets) {
  switch ($target) {
    "windows-x64"      { $arch = "x64";   $flags = "" }
    "windows-x64-avx2" { $arch = "x64";   $flags = "/arch:AVX2" }
    "windows-arm64"    { $arch = "ARM64"; $flags = "" }
    default { throw "unknown target: $target" }
  }
  $native = $arch -eq $hostArch
  $tests = "build-rel-$target-tests"
  $build = "build-rel-$target"
  Remove-Item -Recurse -Force $tests, $build -ErrorAction SilentlyContinue

  # CMake reads CXXFLAGS on the first configure of a build directory, on top of MSVC's defaults
  $env:CXXFLAGS = $flags
  try {
    if ($native) {
      Invoke-Native cmake -S . -B $tests -G $Generator -A $arch
      Invoke-Native cmake --build $tests --config Release --parallel
      Invoke-Native ctest --test-dir $tests -C Release --output-on-failure
    }
    Invoke-Native cmake -S . -B $build -G $Generator -A $arch -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
      -DMAHARAJAH_BUILD_TESTS=OFF -DMAHARAJAH_BUILD_FFI=OFF
    Invoke-Native cmake --build $build --config Release --parallel
  }
  finally {
    $env:CXXFLAGS = $null
  }

  $engine = "$build\sources\Release\Maharajah.exe"
  if (Get-Command dumpbin -ErrorAction SilentlyContinue) {
    # only system DLLs (KERNEL32 etc.), no VCRUNTIME140 or MSVCP140
    dumpbin /dependents $engine | Select-String "\.dll"
  }
  if ($native) {
    # a fixed-depth search runs to completion; every build must report the same node count
    "uci", "position startpos", "go depth 18", "quit" | & $engine | Select-String "^id name| depth 18 "
  }
  else {
    "${target}: built on $hostArch, tests not run; run them on an $arch machine"
  }

  $name = "maharajah-$version-$target"
  Remove-Item -Recurse -Force "dist\$name" -ErrorAction SilentlyContinue
  New-Item -ItemType Directory "dist\$name" | Out-Null
  Copy-Item $engine, LICENSE "dist\$name"
  Compress-Archive -Force "dist\$name" "dist\$name.zip"
  Remove-Item -Recurse -Force "dist\$name"
  "built dist\$name.zip"
}
