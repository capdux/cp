# CP Language Toolchain Production Installer (Windows PowerShell)
#
# Usage:
#   irm https://raw.githubusercontent.com/cp-lang/cp/main/install.ps1 | iex
#
# Optional environment variables or parameters:
#   CP_VERSION / -Version         Release tag (default: latest, e.g. v1.0.0)
#   CP_INSTALL_DIR / -InstallDir  Install directory (default: $env:USERPROFILE\.cap)
#   CP_REPO / -Repo               GitHub repo slug (default: cp-lang/cp)
#   -NoModifyPath                 Skip adding $InstallDir\bin to User Path

[CmdletBinding()]
param(
    [string]$Version = $(if ($env:CP_VERSION) { $env:CP_VERSION } else { "latest" }),
    [string]$InstallDir = $(if ($env:CP_INSTALL_DIR) { $env:CP_INSTALL_DIR } else { Join-Path $env:USERPROFILE ".cap" }),
    [string]$Repo = $(if ($env:CP_REPO) { $env:CP_REPO } else { "cp-lang/cp" }),
    [switch]$NoModifyPath
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Write-Info([string]$Message) {
    Write-Host "[cp-install] $Message"
}

function Resolve-ReleaseTag([string]$RequestedVersion, [string]$Repository) {
    if ($RequestedVersion -eq "latest") {
        $apiUrl = "https://api.github.com/repos/$Repository/releases/latest"
        $headers = @{ "User-Agent" = "cp-installer" }
        $release = Invoke-RestMethod -Uri $apiUrl -Headers $headers -UseBasicParsing
        if (-not $release.tag_name) {
            throw "Failed to resolve latest release tag from $apiUrl. Specify -Version v1.0.0 explicitly."
        }
        return [string]$release.tag_name
    }
    if ($RequestedVersion.StartsWith("v")) {
        return $RequestedVersion
    }
    return "v$RequestedVersion"
}

function Verify-Sha256([string]$ArchivePath, [string]$ArchiveName, [string]$SumsPath) {
    $expected = $null
    foreach ($line in Get-Content -Path $SumsPath) {
        $trimmed = $line.Trim()
        if (-not $trimmed) { continue }
        $parts = $trimmed -split '\s+', 2
        if ($parts.Count -eq 2) {
            $name = $parts[1].TrimStart('*')
            if ($name -eq $ArchiveName) {
                $expected = $parts[0].ToLowerInvariant()
                break
            }
        }
    }
    if (-not $expected) {
        throw "Checksum entry for '$ArchiveName' not found in SHA256SUMS.txt."
    }

    $actual = (Get-FileHash -Path $ArchivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($expected -ne $actual) {
        throw "SHA-256 verification failed for $ArchiveName!`n  expected: $expected`n  actual:   $actual"
    }
    Write-Info ("SHA-256 verified ({0}...)." -f $actual.Substring(0, 16))
}

function Register-UserPath([string]$BinDir) {
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $entries = @()
    if ($userPath) {
        $entries = $userPath.Split(';') | Where-Object { $_ -and $_.Trim() -ne "" }
    }

    $normalizedTarget = $BinDir.TrimEnd('\').ToLowerInvariant()
    $alreadyPresent = $false
    foreach ($entry in $entries) {
        if ($entry.TrimEnd('\').ToLowerInvariant() -eq $normalizedTarget) {
            $alreadyPresent = $true
            break
        }
    }

    if (-not $alreadyPresent) {
        $newUserPath = if ($entries.Count -gt 0) { ($entries + $BinDir) -join ';' } else { $BinDir }
        [Environment]::SetEnvironmentVariable("Path", $newUserPath, "User")
        Write-Info "Added '$BinDir' to User Path environment variable."
    } else {
        Write-Info "'$BinDir' is already registered in User Path."
    }

    if ($env:Path -notlike "*$BinDir*") {
        $env:Path = "$BinDir;$env:Path"
    }
}

if (-not [Environment]::Is64BitOperatingSystem) {
    throw "Unsupported Windows architecture: 64-bit (x86_64) Windows is required."
}

$tag = Resolve-ReleaseTag -RequestedVersion $Version -Repository $Repo
$archiveName = "cp-$tag-windows-x86_64.zip"
$baseUrl = "https://github.com/$Repo/releases/download/$tag"
$tmpDir = Join-Path $env:TEMP ("cp-install-" + [guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null
try {
    $archivePath = Join-Path $tmpDir $archiveName
    $sumsPath = Join-Path $tmpDir "SHA256SUMS.txt"
    $extractDir = Join-Path $tmpDir "extract"

    Write-Info "Installing CP $tag (windows-x86_64) into $InstallDir..."
    Write-Info "Downloading $archiveName..."
    Invoke-WebRequest -Uri "$baseUrl/$archiveName" -OutFile $archivePath -UseBasicParsing
    Write-Info "Downloading SHA256SUMS.txt..."
    Invoke-WebRequest -Uri "$baseUrl/SHA256SUMS.txt" -OutFile $sumsPath -UseBasicParsing

    Verify-Sha256 -ArchivePath $archivePath -ArchiveName $archiveName -SumsPath $sumsPath

    Expand-Archive -Path $archivePath -DestinationPath $extractDir -Force

    $cpcSrc = Join-Path $extractDir "bin\cpc.exe"
    $capSrc = Join-Path $extractDir "bin\cap.exe"
    if (-not (Test-Path $cpcSrc) -or -not (Test-Path $capSrc)) {
        throw "Release archive is missing bin\cpc.exe or bin\cap.exe."
    }

    $binDir = Join-Path $InstallDir "bin"
    $libDir = Join-Path $InstallDir "lib"
    New-Item -ItemType Directory -Force -Path $binDir | Out-Null

    Copy-Item -Force $cpcSrc (Join-Path $binDir "cpc.exe")
    Copy-Item -Force $capSrc (Join-Path $binDir "cap.exe")

    $libSrc = Join-Path $extractDir "lib"
    if (Test-Path $libSrc) {
        if (Test-Path $libDir) {
            Remove-Item -Recurse -Force $libDir
        }
        Copy-Item -Recurse -Force $libSrc $libDir
    }

    foreach ($doc in @("LICENSE-MIT", "LICENSE-APACHE", "THIRD_PARTY_NOTICES.md", "README.md")) {
        $docSrc = Join-Path $extractDir $doc
        if (Test-Path $docSrc) {
            Copy-Item -Force $docSrc (Join-Path $InstallDir $doc)
        }
    }

    if (-not $NoModifyPath) {
        Register-UserPath -BinDir $binDir
    } else {
        Write-Info "Skipped Path registration (-NoModifyPath). Add '$binDir' to your Path manually."
    }

    Write-Info "Successfully installed CP $tag!"
    Write-Info "  cpc: $binDir\cpc.exe"
    Write-Info "  cap: $binDir\cap.exe"
    Write-Info "  lib: $libDir"
    Write-Info "Restart your terminal to use 'cpc' and 'cap' in new sessions."
} finally {
    Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
}
