param(
    [Parameter(Mandatory)][string]$ArtifactDirectory,
    [Parameter(Mandatory)][string]$Destination,
    [Parameter(Mandatory)][string]$ExpectedCommit
)

$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath $ArtifactDirectory
try {
    $installers = @(Get-ChildItem -File -Filter '*-win64-setup.exe')
    if ($installers.Count -ne 1) { throw 'Expected one installer' }
    if ((Get-Content -LiteralPath BUILD-INFO.txt -TotalCount 1) -cne "commit=$ExpectedCommit") {
        throw 'Installer artifact was built from the wrong commit'
    }
    $expected = ((Get-Content -LiteralPath SHA256SUMS -Raw).Trim() -split '\s+', 2)[0]
    $installer = $installers[0].FullName
    if ((Get-FileHash -LiteralPath $installer).Hash -ine $expected) { throw 'Installer hash mismatch' }
    if (Test-Path -LiteralPath $Destination) { throw 'Install location is not clean' }

    $result = Start-Process -FilePath $installer -ArgumentList "/S /D=$Destination" -Wait -PassThru
    if ($result.ExitCode -ne 0) { throw "Installer exit code $($result.ExitCode)" }
    $miner = Join-Path $Destination 'miners/agrarian-pool-worker.exe'
    if (!(Test-Path -LiteralPath $miner -PathType Leaf)) { throw 'First-party worker missing' }
    $legacy = Join-Path $Destination 'miners/cpuminer.exe'
    if (Test-Path -LiteralPath $legacy) { throw 'Legacy miner is still packaged' }
    $source = Join-Path $Destination 'miners/quark-screen.cl'
    if ((Get-FileHash -LiteralPath $source).Hash -ine '9387b4e30715e7599843d24bc3bdded5921788d54f2e46dc5cddabf6bc8f5727') {
        throw 'GPU kernel hash mismatch'
    }

    # With the GUI PE subsystem, PowerShell's call operator alone need not
    # wait or report the process's exit code.
    $tls = Start-Process -FilePath $miner -ArgumentList '--check-tls-backend' -Wait -PassThru
    if ($tls.ExitCode -ne 0) { throw "Installed mining worker TLS check exited with $($tls.ExitCode)" }
    python "$PSScriptRoot/first_party_pool_guard.py" "$miner"
    if ($LASTEXITCODE -ne 0) { throw 'First-party worker smoke test failed' }
    Set-Content -LiteralPath $legacy -Value 'Inert fixture representing the old helper'
    $smokeArgs = @('--qt', "$Destination/agrarian-qt.exe", '--daemon', "$Destination/daemon/agrariand.exe", '--installer', $installer)
    python "$PSScriptRoot/desktop_package_smoke.py" @smokeArgs
    if ($LASTEXITCODE -ne 0) { throw 'Installed wallet smoke test failed' }
    if (Test-Path -LiteralPath $legacy) { throw 'Upgrade did not remove the inert legacy helper fixture' }

    $defender = if (Get-Command Get-MpComputerStatus -ErrorAction SilentlyContinue) {
        Get-MpComputerStatus -ErrorAction SilentlyContinue
    }
    if ($defender -and $defender.AntivirusEnabled -and $defender.RealTimeProtectionEnabled) {
        Start-MpScan -ScanType CustomScan -ScanPath $Destination
        Start-MpScan -ScanType CustomScan -ScanPath $installer
        $paths = @($Destination, $installer)
        $detections = @(Get-MpThreatDetection | Where-Object {
            $resources = $_.Resources -join ' '
            @($paths | Where-Object { $resources.Contains($_, [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
        })
        if ($detections.Count) { throw 'Microsoft Defender detected an item in the candidate installer or installed files' }
        Write-Host "Defender custom scans passed (signatures $($defender.AntivirusSignatureVersion))"
    } else {
        Write-Warning 'Defender real-time protection is unavailable on this runner; no Defender verdict was obtained'
    }
} finally {
    Pop-Location
}
