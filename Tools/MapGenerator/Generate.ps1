# Writes the first version of the chapel map to Assets/Maps/Chapel.map (see ChapelGenerator.cs). Run once: the map is
# then edited in TrenchBroom, and running this again would lose those edits.
# Run from any folder:  powershell -ExecutionPolicy Bypass -File Tools/MapGenerator/Generate.ps1

$repositoryRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Add-Type -Path (Join-Path $PSScriptRoot "ChapelGenerator.cs")
[ChapelGenerator]::Run((Join-Path $repositoryRoot "Assets\Maps\Chapel.map"))
