# Makes the normal, roughness and height maps of the textures of Episode 1 from their colors (see MaterialMaps.cs) and a
# page to look at them: Build/MaterialMaps/Episode1.html, every texture flat and with its maps, under a light you move.
# Run from any folder:  powershell -ExecutionPolicy Bypass -File Tools/MaterialMaps/Generate.ps1
#
# By default the maps go to Build/MaterialMaps/Episode1, to be looked at first; -Install writes them next to the colors in
# Assets/Textures/Episode1, where the game finds them (see Renderer::LoadMaterialByFileNames).
# Windows PowerShell 5.1 compiles MaterialMaps.cs on the fly (Add-Type); System.Drawing reads and writes the PNG files.

param(
    [switch]$Install
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$colorDirectory = Join-Path $repositoryRoot "Assets\Textures\Episode1"
$previewDirectory = Join-Path $repositoryRoot "Build\MaterialMaps"
$mapDirectory = if ($Install) { $colorDirectory } else { Join-Path $previewDirectory "Episode1" }

Add-Type -Path (Join-Path $PSScriptRoot "MaterialMaps.cs") -ReferencedAssemblies System.Drawing
$settings = [MaterialMaps]::CreateEpisode1Settings()
[MaterialMaps]::GenerateAll($colorDirectory, $mapDirectory, $settings)
Write-Host "Maps written to $mapDirectory"

# --- The preview page: every image is put into the page itself (base64), so the page can read its pixels ----------------
function Get-DataUri([string]$path)
{
    return "data:image/png;base64," + [Convert]::ToBase64String([IO.File]::ReadAllBytes($path))
}

$entries = foreach ($name in ($settings.Keys | Sort-Object))
{
    $color = Get-DataUri (Join-Path $colorDirectory "$name.png")
    $normal = Get-DataUri (Join-Path $mapDirectory "${name}_Normal.png")
    $metalRough = Get-DataUri (Join-Path $mapDirectory "${name}_MetalRough.png")
    "{name:'$name', color:'$color', normal:'$normal', metalRough:'$metalRough'}"
}
$textures = "[" + ($entries -join ",`n") + "]"

$page = Get-Content -Raw -Encoding UTF8 (Join-Path $PSScriptRoot "Preview.html")
$page = $page.Replace("/*TEXTURES*/[]", $textures)
New-Item -ItemType Directory -Force $previewDirectory | Out-Null
$pagePath = Join-Path $previewDirectory "Episode1.html"
[IO.File]::WriteAllText($pagePath, $page, (New-Object Text.UTF8Encoding $false))
Write-Host "Preview written to $pagePath"
