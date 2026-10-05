# Measures the game with the Tracy profiler in a repeatable scene and prints what a frame costs.
# Run from any folder:  powershell -ExecutionPolicy Bypass -File Tools/Profiling/Capture.ps1 -Configuration Debug -Label before
#
# 1. Builds the game (Visual Studio's own CMake, the presets of the repository), unless -NoBuild.
# 2. Starts tracy-capture, which waits for the game and writes everything it sends into a .tracy file.
# 3. Runs the game with --benchmark: the start map, nobody at the controls, an invulnerable player the dogs attack; the game
#    closes itself after 20 s of game time (see Application/LaunchOptions.h), and the capture ends with it.
# 4. tracy-csvexport turns the capture into tables; the summary shows the frame rate and the cost of every zone per frame.
#
# Everything goes to Build/Profiles/<date>-<time>-<configuration>-<label>.*: the .tracy file opens in the Tracy profiler,
# the .csv files keep every number, the -summary.txt file is what the console shows.
# The Tracy tools must have the version of ThirdParty/Tracy (0.14.1). Only one program can be connected to the game: close
# the Tracy profiler window before a capture.

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    # A word for the file names, to tell captures apart: "before", "after-traces".
    [string]$Label = "capture",

    # The folder with tracy-capture.exe and tracy-csvexport.exe (the Windows archive of the Tracy release).
    [string]$TracyDirectory = (Join-Path $env:USERPROFILE "Downloads\windows-0.14.1"),

    # How many zones the summary lists, the most expensive first.
    [int]$ZoneCount = 30,

    # Measure the game as it is built now.
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$captureTool = Join-Path $TracyDirectory "tracy-capture.exe"
$exportTool = Join-Path $TracyDirectory "tracy-csvexport.exe"
foreach ($tool in @($captureTool, $exportTool))
{
    if (-not (Test-Path $tool))
    {
        throw "$tool not found. Download the Windows archive of Tracy 0.14.1 from https://github.com/wolfpld/tracy/releases " +
              "and pass its folder with -TracyDirectory."
    }
}

# --- 1. Build -------------------------------------------------------------------------------------------------------------
if (-not $NoBuild)
{
    # The CMake that comes with Visual Studio, the one Visual Studio itself configures the Build folder with: another
    # version on PATH would configure it differently.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $visualStudio = & $vswhere -latest -prerelease -property installationPath
    $cmake = Join-Path $visualStudio "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

    Push-Location $repositoryRoot
    try
    {
        & $cmake --build --preset $Configuration.ToLower()
        if ($LASTEXITCODE -ne 0) { throw "The build failed (exit code $LASTEXITCODE)." }
    }
    finally
    {
        Pop-Location
    }
}

$game = Join-Path $repositoryRoot "Build\windows-msvc\Binaries\$Configuration\Abomination.exe"
if (-not (Test-Path $game)) { throw "$game not found: build the $Configuration configuration first." }

# --- 2 and 3. Capture while the benchmark runs ---------------------------------------------------------------------------
$profilesDirectory = Join-Path $repositoryRoot "Build\Profiles"
New-Item -ItemType Directory -Force $profilesDirectory | Out-Null
$baseName = "{0}-{1}-{2}" -f (Get-Date -Format "yyyy-MM-dd-HHmm"), $Configuration.ToLower(), $Label
$tracePath = Join-Path $profilesDirectory "$baseName.tracy"

# tracy-capture keeps trying to connect until the game is up, and stops when the game closes. -f overwrites the file.
$capture = Start-Process -FilePath $captureTool -ArgumentList @("-a", "127.0.0.1", "-o", "`"$tracePath`"", "-f") `
                         -PassThru -WindowStyle Hidden
Write-Host "Running the benchmark ($Configuration, 20 s). Do not close the game window."
$benchmark = Start-Process -FilePath $game -ArgumentList "--benchmark" -WorkingDirectory (Split-Path $game) -PassThru -Wait
if ($benchmark.ExitCode -ne 0) { throw "The game exited with code $($benchmark.ExitCode); see Abomination.log next to it." }

# The capture still saves the file after the game is gone.
if (-not $capture.WaitForExit(60000))
{
    $capture.Kill()
    throw "tracy-capture did not finish; was the Tracy profiler window connected to the game?"
}
if (-not (Test-Path $tracePath)) { throw "No capture was written: tracy-capture could not connect to the game." }

# --- 4. Tables and the summary -------------------------------------------------------------------------------------------
$zonesPath = Join-Path $profilesDirectory "$baseName-zones.csv"
$selfPath = Join-Path $profilesDirectory "$baseName-self.csv"
$GPUPath = Join-Path $profilesDirectory "$baseName-gpu.csv"
$framesPath = Join-Path $profilesDirectory "$baseName-frames.csv"
& $exportTool $tracePath | Out-File -Encoding utf8 $zonesPath           # every zone: calls, total and mean times
& $exportTool -e $tracePath | Out-File -Encoding utf8 $selfPath         # the same without the time of the zones inside
& $exportTool -g $tracePath | Out-File -Encoding utf8 $GPUPath          # every GPU zone of every frame
& $exportTool -u -f "Application::Render" $tracePath | Out-File -Encoding utf8 $framesPath  # when every frame was drawn

# Zone names without the namespaces everyone has: "Gameplay::MoveDog", not
# "Abomination::Gameplay::`anonymous-namespace'::MoveDog".
function Get-ShortName([string]$name)
{
    return $name.Replace("Abomination::", "").Replace("``anonymous-namespace'::", "")
}

# The frames: Render runs once per frame, so the time between its first and last start divided by the frames between them
# is the average frame. The frames before the game connected and the loading are not in it.
$frames = @(Import-Csv $framesPath)
$frameCount = $frames.Count
if ($frameCount -lt 2) { throw "The capture has fewer than two frames." }
$firstStart = [double]$frames[0].ns_since_start
$lastStart = [double]$frames[$frameCount - 1].ns_since_start
$frameMilliseconds = ($lastStart - $firstStart) / ($frameCount - 1) / 1e6

$self = @{}
foreach ($row in Import-Csv $selfPath) { $self[$row.name] = [double]$row.total_ns }

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("Abomination $Configuration, $Label - $baseName")
$lines.Add(("{0} frames, {1:N2} ms per frame, {2:N0} FPS" -f $frameCount, $frameMilliseconds, (1000.0 / $frameMilliseconds)))
$lines.Add("")
$lines.Add("CPU zones, per frame: total = with the zones inside, self = without them")
$lines.Add(("{0,-52} {1,8} {2,10} {3,10}" -f "Zone", "Calls", "Total ms", "Self ms"))
Import-Csv $zonesPath |
    Sort-Object { [double]$_.total_ns } -Descending |
    Select-Object -First $ZoneCount |
    ForEach-Object {
        $lines.Add(("{0,-52} {1,8:N1} {2,10:N3} {3,10:N3}" -f (Get-ShortName $_.name), ([double]$_.counts / $frameCount),
                    ([double]$_.total_ns / $frameCount / 1e6), ($self[$_.name] / $frameCount / 1e6)))
    }

# GPU zones: the time the GPU itself spent on the drawing commands of each zone, added up per name.
$lines.Add("")
$lines.Add("GPU zones, per frame")
$lines.Add(("{0,-52} {1,8} {2,10}" -f "Zone", "Calls", "GPU ms"))
Import-Csv $GPUPath |
    Group-Object name |
    ForEach-Object {
        $total = ($_.Group | Measure-Object -Property "GPU execution time" -Sum).Sum
        [pscustomobject]@{ Name = $_.Name; Count = $_.Count; Total = $total }
    } |
    Sort-Object Total -Descending |
    ForEach-Object {
        $lines.Add(("{0,-52} {1,8:N1} {2,10:N3}" -f $_.Name, ($_.Count / $frameCount), ($_.Total / $frameCount / 1e6)))
    }

$summaryPath = Join-Path $profilesDirectory "$baseName-summary.txt"
$lines | Out-File -Encoding utf8 $summaryPath
$lines | ForEach-Object { Write-Host $_ }
Write-Host ""
Write-Host "Written to $profilesDirectory\$baseName.*"
