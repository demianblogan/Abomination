# Generates the game textures into Assets/Textures, the HUD icons into Assets/UI/Icons, preview sheets into Build/TexturePreviews and the image of the
# episode palettes into Documentation/Images.
# Run from any folder:  powershell -ExecutionPolicy Bypass -File Tools/TextureGenerator/Generate.ps1
# Windows PowerShell 5.1 compiles TextureGenerator.cs on the fly (Add-Type); System.Drawing writes the PNG files.

$repositoryRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
New-Item -ItemType Directory -Force (Join-Path $repositoryRoot "Documentation\Images") | Out-Null
Add-Type -Path (Join-Path $PSScriptRoot "TextureGenerator.cs") -ReferencedAssemblies System.Drawing
[TextureGen]::Run((Join-Path $repositoryRoot "Assets\Textures"),
                  (Join-Path $repositoryRoot "Build\TexturePreviews"),
                  (Join-Path $repositoryRoot "Documentation\Images\EpisodePalettes.png"))
[TextureGen]::RunIcons((Join-Path $repositoryRoot "Assets\UI\Icons"),
                       (Join-Path $repositoryRoot "Build\TexturePreviews\HUDIcons.png"))
[TextureGen]::RunHUDEffects((Join-Path $repositoryRoot "Assets\UI\Images"),
                            (Join-Path $repositoryRoot "Assets\Textures\Episode1\Wall_MossyBrick.png"),
                            (Join-Path $repositoryRoot "Build\TexturePreviews\HUDEffects.png"))
Write-Host "Textures written to Assets\Textures, HUD icons and effects to Assets\UI, previews to Build\TexturePreviews"
