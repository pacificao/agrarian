$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Get-Content -Raw (Join-Path $repo 'src/qt/pooltemperatures.h')
$parts = [regex]::Matches($source, '(?s)R"PS\((.*?)\)PS"')
if ($parts.Count -ne 2) { throw 'Could not extract the worker’s actual PowerShell probe' }
$fixture = @'
function Get-CimInstance {
    param([string]$Namespace, [string]$ClassName, [int]$OperationTimeoutSec)
    if ($ClassName -eq 'Sensor' -and $env:AGR_THERMAL_TEST_CASE -eq 'AMD') {
        [pscustomobject]@{Parent='/amdcpu/0';Name='Core (Tdie)';SensorType='Temperature';Value=72.5}
        [pscustomobject]@{Identifier='/amdcpu/0/temperature/1';Name='CCD2 (Tdie)';SensorType='Temperature';Value=76.3}
        [pscustomobject]@{Parent='/amdcpu/0';Name='Core (Tctl)';SensorType='Temperature';Value=91}
        [pscustomobject]@{Identifier='/gpu-nvidia/0/temperature/0';Name='CPU Package';SensorType='Temperature';Value=110}
    }
    if ($ClassName -eq 'Sensor' -and $env:AGR_THERMAL_TEST_CASE -eq 'Intel') {
        [pscustomobject]@{Parent='/intelcpu/0';Name='CPU Package';SensorType='Temperature';Value=65.0}
        [pscustomobject]@{Identifier='/intelcpu/0/temperature/1';Name='CPU Core #3';SensorType='Temperature';Value=72.5}
        [pscustomobject]@{Parent='/intelcpu/0';Name='Core Max';SensorType='Temperature';Value=$null}
    }
    if ($ClassName -eq 'MSAcpi_ThermalZoneTemperature') {
        [pscustomobject]@{InstanceName='ACPI\ThermalZone\BATZ_0';Active=$true;CurrentTemperature=3500}
        [pscustomobject]@{InstanceName='ACPI\ThermalZone\GPUZ_0';Active=$true;CurrentTemperature=3700}
        if ($env:AGR_THERMAL_TEST_CASE -in 'Acpi','Perf') {
            [pscustomobject]@{InstanceName='ACPI\ThermalZone\TZ01_0';Active=$true;CurrentTemperature=3400}
        }
        if ($env:AGR_THERMAL_TEST_CASE -eq 'Acpi') {
            [pscustomobject]@{InstanceName='ACPI\ThermalZone\CPUZ_0';Active=$true;CurrentTemperature=3192}
            [pscustomobject]@{InstanceName='ACPI\ThermalZone\CPUZ_1';Active=$false;CurrentTemperature=3600}
        }
    }
    if ($ClassName -eq 'Win32_PerfFormattedData_Counters_ThermalZoneInformation' -and $env:AGR_THERMAL_TEST_CASE -eq 'Perf') {
        [pscustomobject]@{Name='_TZ.CPUZ';Temperature=319}
    }
}
'@
function Assert-Probe([string]$Case,[string]$Expected,[bool]$Proxy=$true) {
    $env:AGR_THERMAL_TEST_CASE = $Case
    $script = $fixture + "`n" + $parts[0].Groups[1].Value
    if ($Proxy) { $script += $parts[1].Groups[1].Value }
    $script64 = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($script))
    $result = (& "$env:SystemRoot/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile -NonInteractive -EncodedCommand $script64) -join "`n"
    if ($LASTEXITCODE -ne 0 -or $result -notmatch $Expected) {
        throw "CPU temperature fixture $Case returned '$result'"
    }
    Write-Host "Windows CPU temperature fixture $Case passed"
}
Assert-Probe 'AMD' '^CPU 76\.3$'
Assert-Probe 'Intel' '^CPU 72\.5$'
Assert-Probe 'Acpi' '^PROXY 46\.[01]$'
Assert-Probe 'Perf' '^PROXY 45\.[89]$'
Assert-Probe 'Unrelated' '^$'
Assert-Probe 'Acpi' '^$' $false
