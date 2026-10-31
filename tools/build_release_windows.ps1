# Builds the Windows release binaries: static C++ runtime (no Visual C++ Redistributable
# needed), one archive per target in dist\ (maharajah-<version>-<target>.zip with
# Maharajah.exe and LICENSE).
# Usage (Developer PowerShell for VS):
#   tools\build_release_windows.ps1 windows-x64 windows-x64-avx2 windows-arm64
#
# Each target is built twice: build-rel-<target>-tests with the tests (googletest is
# forced onto the DLL runtime) and build-rel-<target> with the engine alone on the static
# runtime. The tests and the search check run only on a host of the target's architecture.
#
# windows-arm64 on an x64 host is cross-compiled with Ninja inside the vcvarsall x64_arm64
# environment, because the Visual Studio 2026 generator has no ARM64 platform there; it needs
# the "MSVC Build Tools for ARM64/ARM64EC" component.
param(
  # an explicit Position leaves -Generator named only, so every bare argument is a target
  [Parameter(Mandatory, Position = 0, ValueFromRemainingArguments)][string[]]$Targets,
  [string]$Generator = "Visual Studio 18 2026"
)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

function Invoke-Native {
  $command, $arguments = $args
  & $command @arguments
  if ($LASTEXITCODE) { throw "$command $arguments failed with exit code $LASTEXITCODE" }
}

# Imports the vcvarsall environment for a cross build into this process; returns the
# previous variables for Restore-Environment
function Enter-VcEnvironment($vcArch) {
  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  $root = & $vswhere -latest -products * -property installationPath
  if (-not $root) { throw "Visual Studio not found" }
  # the newest toolset that has the ARM64 compiler; the latest one may lack it
  $toolset = Get-ChildItem "$root\VC\Tools\MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending |
    Where-Object { Test-Path "$($_.FullName)\bin\Hostx64\arm64\cl.exe" } | Select-Object -First 1
  if (-not $toolset) { throw "no MSVC toolset with an ARM64 compiler; install the MSVC ARM64/ARM64EC build tools" }
  $saved = @{}
  foreach ($variable in Get-ChildItem Env:) { $saved[$variable.Name] = $variable.Value }
  $lines = cmd /c "`"$root\VC\Auxiliary\Build\vcvarsall.bat`" $vcArch -vcvars_ver=$($toolset.Name) >nul && set"
  if ($LASTEXITCODE) { throw "vcvarsall $vcArch failed" }
  foreach ($line in $lines) {
    if ($line -match '^([^=]+)=(.*)$') { Set-Item "Env:$($Matches[1])" $Matches[2] }
  }
  $saved
}

function Restore-Environment($saved) {
  foreach ($variable in Get-ChildItem Env:) {
    if (-not $saved.ContainsKey($variable.Name)) { Remove-Item "Env:$($variable.Name)" }
  }
  foreach ($name in $saved.Keys) { Set-Item "Env:$name" $saved[$name] }
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
  $ninja = $arch -eq "ARM64" -and -not $native
  $tests = "build-rel-$target-tests"
  $build = "build-rel-$target"
  Remove-Item -Recurse -Force $tests, $build -ErrorAction SilentlyContinue

  # CMake reads CXXFLAGS on the first configure of a build directory, on top of MSVC's defaults
  $env:CXXFLAGS = $flags
  $saved = $null
  try {
    if ($native) {
      Invoke-Native cmake -S . -B $tests -G $Generator -A $arch
      Invoke-Native cmake --build $tests --config Release --parallel
      Invoke-Native ctest --test-dir $tests -C Release --output-on-failure
    }
    if ($ninja) {
      $saved = Enter-VcEnvironment "x64_arm64"
      Invoke-Native cmake -S . -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DMAHARAJAH_BUILD_TESTS=OFF -DMAHARAJAH_BUILD_FFI=OFF
      Invoke-Native cmake --build $build --parallel
    }
    else {
      Invoke-Native cmake -S . -B $build -G $Generator -A $arch -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DMAHARAJAH_BUILD_TESTS=OFF -DMAHARAJAH_BUILD_FFI=OFF
      Invoke-Native cmake --build $build --config Release --parallel
    }
  }
  finally {
    if ($saved) { Restore-Environment $saved }
    $env:CXXFLAGS = $null
  }

  $engine = if ($ninja) { "$build\sources\Maharajah.exe" } else { "$build\sources\Release\Maharajah.exe" }
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
