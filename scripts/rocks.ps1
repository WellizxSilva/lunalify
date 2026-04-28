param(
    [switch]$Upload # if set, upload the rock to LuaRocks
)
if (-not (Get-Module -ListAvailable -Name PowerShell-Yaml)) {
    Write-Host "Module PowerShell-Yaml not found. Installing..."
    Install-Module -Name PowerShell-Yaml -Force -AllowClobber -Scope CurrentUser
}
Import-Module PowerShell-Yaml

if(!(Test-Path "./project.yaml")) {
    Write-Host "[Error] project.yaml not found!" -ForegroundColor Red
    exit 1
}

try {
    $yamlRaw = Get-Content -Path "project.yaml" -Raw
    $config = $yamlRaw | ConvertFrom-Yaml
} catch {
    Write-Host "[Error] Failed to parse project.yaml in rocks.ps1" -ForegroundColor Red
    exit 1
}

$projectName = $config.project.name
$version     = $config.project.version
$rockRev     = "1" # initial rockspec revision
$fullVersion = "$version-$rockRev"

if ([string]::IsNullOrEmpty($projectName)) {
    Write-Host "[Error] Metadata missing in project.yaml" -ForegroundColor Red
    exit 1
}

Write-Host "--- LuaRocks: $projectName $fullVersion ---" -ForegroundColor Magenta

# update rockspec version and rename
$oldRockspec = Get-ChildItem -Filter "*.rockspec" | Select-Object -First 1
$newRockspecName = "$projectName-$fullVersion.rockspec"

Write-Host "[1/3] Updating rockspec version and filename..." -ForegroundColor Gray
if ($oldRockspec) {
    $content = Get-Content $oldRockspec.FullName -Raw
    $content = $content -replace 'version\s*=\s*".*"', "version = `"$fullVersion`""
    $content | Set-Content $oldRockspec.FullName

    if ($oldRockspec.Name -ne $newRockspecName) {
        Rename-Item -Path $oldRockspec.FullName -NewName $newRockspecName -Force
    }
}

# install locally (required for build binary rock)
Write-Host "[2/3] Installing rock locally..." -ForegroundColor Gray
luarocks make $newRockspecName --local

# pack binary rock
Write-Host "[3/3] Packing binary rock..." -ForegroundColor Yellow
luarocks pack $projectName $fullVersion

if ($Upload) {
    Write-Host "--- Uploading to LuaRocks.org ---" -ForegroundColor Cyan
    luarocks upload $newRockspecName
}

Write-Host "--- LuaRocks Task Completed ---" -ForegroundColor Green
