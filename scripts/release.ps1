# Requires the Powershell-Yaml module

if (-not (Get-Module -ListAvailable -Name PowerShell-Yaml)) {
    Write-Host "Module PowerShell-Yaml not found. Installing..."
    Install-Module -Name PowerShell-Yaml -Force -AllowClobber -Scope CurrentUser
}

Import-Module Powershell-Yaml

if(!(Test-Path "./project.yaml")) {
    Write-Host "[Error] project.yaml not found!" -ForegroundColor Red
    exit 1a
}

# Parsing Configuration
try {
    $yamlRaw = Get-Content -Path "project.yaml" -Raw
    $config = $yamlRaw | ConvertFrom-Yaml
} catch {
    Write-Host "[Error] Failed to parse project.yaml. Check Yaml syntax!" -ForegroundColor Red
    exit 1
}

# Project Metadata
$projectName    = $config.project.name
$version        = $config.project.version
$releaseName    = $config.project.release_name

# Build & Paths
$targetDll      = $config.build.target
$binDir         = $config.paths.bin
$luaSource      = $config.paths.lua
$docsSource     = $config.paths.docs

# Release Settings
$distDir        = Join-Path (Get-Location) $config.release.output
$archiveFormat  = $config.release.archive_format
$packageDir     = Join-Path $distDir $projectName

$zipPath = Join-Path (Get-Location) "$releaseName.zip"

Write-Host "--- Generating Release: $releaseName ---" -ForegroundColor Cyan

# Cleanup old release artifacts
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
if (Test-Path $zipPath) { Remove-Item $zipPath }

# Creating Directory Structure
Write-Host "[1/4] Creating structure..." -ForegroundColor Gray
New-Item -ItemType Directory -Path "$packageDir/bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$packageDir/docs" -Force | Out-Null

# Copying Files
Write-Host "[2/4] Copying binaries..." -ForegroundColor Gray
$dllPath = Join-Path $binDir $targetDll
if (Test-Path $dllPath) {
    Copy-Item $dllPath "$packageDir/bin/" -Force
} else {
    Write-Host "[Warning] Binary $dllPath not found! Run 'make' first." -ForegroundColor Yellow
}

Write-Host "[3/4] Copying Lua library and docs..." -ForegroundColor Gray
# Copy the contents of the configured lua path
if (Test-Path $luaSource) {
    Copy-Item "$luaSource*" "$packageDir/" -Recurse -Force
}

if (Test-Path $docsSource) {
    Copy-Item "$docsSource*" "$packageDir/docs/" -Recurse -Force
}

# Copy Metadata from Root
$metaFiles = @("README.md", "LICENSE", "CHANGELOG.md", "project.yaml")
foreach ($file in $metaFiles) {
    if (Test-Path $file) { Copy-Item $file "$packageDir/" -Force }
}

# Archiving
Write-Host "[4/4] Compressing release into $archiveFormat..." -ForegroundColor Yellow
    Push-Location $distDir
try {
    Compress-Archive -Path $projectName -DestinationPath $zipPath -Force
} finally {
    Pop-Location
}

# Final Cleanup
Write-Host "Cleaning up temporary directory..." -ForegroundColor Gray
Remove-Item $distDir -Recurse -Force

Write-Host "--- Release generated successfully: $zipPath ---" -ForegroundColor Green
