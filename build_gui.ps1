param(
    [ValidateSet("gui", "terminal")]
    [string]$Target = "gui",

    [ValidateSet("raylib", "sdl2")]
    [string]$Backend = "raylib",

    [ValidateSet("build", "run")]
    [string]$Mode = "build"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

$commonSources = @(
    "src/process/process.c",
    "src/os/syscalls.c",
    "src/os/os_core.c",
    "src/synchronization/mutex.c",
    "src/memory/memory.c",
    "src/scheduler/queue.c",
    "src/scheduler/mlfq.c",
    "src/scheduler/hrrn.c",
    "src/scheduler/scheduler.c",
    "src/scheduler/rr.c",
    "src/interpreter/parser.c",
    "src/interpreter/interpreter.c"
)

$includeDirs = @(
    "-Isrc",
    "-Isrc/os",
    "-Isrc/process",
    "-Isrc/memory",
    "-Isrc/interpreter",
    "-Isrc/scheduler",
    "-Isrc/synchronization",
    "-Isrc/gui"
)

$ucrtGcc = "C:\msys64\ucrt64\bin\gcc.exe"
$compiler = if (Test-Path $ucrtGcc) { $ucrtGcc } else { "gcc" }
$outExe = $null
$sources = @()
$backendLibs = @()
$extraDefines = @()

if ($Target -eq "terminal") {
    $outExe = Join-Path $buildDir "os_terminal.exe"
    $sources = @("src/main.c") + $commonSources | ForEach-Object { Join-Path $root $_ }
    $extraDefines = @("-DOS_FORCE_TERMINAL")
    Write-Host "Building terminal simulator"
} else {
    $outExe = if ($Backend -eq "raylib") { Join-Path $buildDir "os_gui_raylib.exe" } else { Join-Path $buildDir "os_gui_sdl2.exe" }
    $guiSource = if ($Backend -eq "raylib") { "src/gui/gui_raylib.c" } else { "src/gui/gui_sdl2.c" }
    $sources = @($guiSource) + $commonSources | ForEach-Object { Join-Path $root $_ }
    $backendLibs = if ($Backend -eq "raylib") {
        @("-lraylib", "-lopengl32", "-lgdi32", "-lwinmm")
    } else {
        @("-lmingw32", "-lSDL2main", "-lSDL2", "-lSDL2_ttf")
    }
    Write-Host "Building GUI backend: $Backend"
}

$compileArgs = @("-g") + $extraDefines + $includeDirs + $sources + @("-o", $outExe) + $backendLibs + @("-pthread")
& $compiler @compileArgs

if ($LASTEXITCODE -ne 0) {
    if ($Target -eq "terminal") {
        Write-Host "Hint: verify GCC and include paths are available on your PATH."
    } else {
        if ($Backend -eq "raylib") {
            Write-Host "Hint: Raylib headers/libs not found."
            Write-Host "If you use MSYS2 UCRT64, install with: pacman -S --needed mingw-w64-ucrt-x86_64-raylib"
        } else {
            Write-Host "Hint: SDL2 headers/libs not found."
            Write-Host "If you use MSYS2 UCRT64, install with: pacman -S --needed mingw-w64-ucrt-x86_64-SDL2"
        }
    }
    throw "Build failed for target '$Target'."
}

Write-Host "Built: $outExe"

if ($Mode -eq "run") {
    $ucrtBin = "C:\msys64\ucrt64\bin"
    if (Test-Path $ucrtBin) {
        $env:PATH = "$ucrtBin;" + $env:PATH
    }
    & $outExe
}
