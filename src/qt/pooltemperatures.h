// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_POOLTEMPERATURES_H
#define AGRARIAN_QT_POOLTEMPERATURES_H

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLibrary>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace poolthermal {
inline double Missing() { return std::numeric_limits<double>::quiet_NaN(); }

struct Reading {
    double core{Missing()};
    double hotspot{Missing()};
    bool proxy{false};
    bool fresh{true};
};

inline Reading ParseWindowsReading(const QByteArray& output, bool allowProxy)
{
    const auto fields = output.trimmed().split(' ');
    if (fields.size() != 2 || (fields[0] != "CPU" && (fields[0] != "PROXY" || !allowProxy))) return {};
    static const QRegularExpression numeric(QStringLiteral("^[0-9]{1,3}(\\.[0-9]{1,2})?$"));
    if (!numeric.match(QString::fromLatin1(fields[1])).hasMatch()) return {};
    const auto temperature = fields[1].toDouble();
    if (temperature < 5 || temperature > 120) return {};
    return {temperature, Missing(), fields[0] == "PROXY", true};
}

#ifdef Q_OS_WIN
class WindowsCpuSensor {
public:
    WindowsCpuSensor()
    {
        timer.start();
        process.setStandardErrorFile(QProcess::nullDevice());
        process.setStandardInputFile(QProcess::nullDevice());
        process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
            args->flags &= ~CREATE_NEW_CONSOLE;
            args->flags |= CREATE_NO_WINDOW;
        });
        wchar_t folder[1024]{};
        const auto length = GetSystemDirectoryW(folder, 1024);
        if (length && length < 1024) {
            process.setProgram(QString::fromWCharArray(folder, length) + QStringLiteral("/WindowsPowerShell/v1.0/powershell.exe"));
        }
    }

    ~WindowsCpuSensor()
    {
        if (process.state() != QProcess::NotRunning) {
            process.kill();
            process.waitForFinished(1000);
        }
    }

    Reading Read(bool allowProxy)
    {
        if (process.program().isEmpty()) return {};
        const auto now = timer.elapsed();
        if (running && process.state() == QProcess::NotRunning) {
            running = false;
            const auto result = process.readAllStandardOutput();
            cached = !killed && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0 && result.size() <= 100
                ? ParseWindowsReading(result, allowProxy) : Reading{};
            stamped = now;
            return cached;
        }
        if (running && !killed && now - began >= 7000) {
            killed = true;
            cached = {};
            process.kill();
        }
        if (!running && now >= next) {
            began = now;
            next = now + 7500;
            killed = false;
            running = true;
            process.setArguments({QStringLiteral("-NoLogo"), QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                                  QStringLiteral("-Command"), Script(allowProxy)});
            process.start();
        }
        if (std::isfinite(cached.core) && now - stamped <= 10000) {
            auto previous = cached;
            previous.fresh = false; // A cached sample never counts toward cooldown.
            return previous;
        }
        return {};
    }

private:
    QProcess process;
    QElapsedTimer timer;
    Reading cached;
    qint64 began{0}, next{0}, stamped{0};
    bool running{false}, killed{false};

    static QString Script(bool allowProxy)
    {
        const auto body = QString::fromLatin1(R"PS(
$ErrorActionPreference='Stop'
$culture=[Globalization.CultureInfo]::InvariantCulture
function Emit($kind,$samples) {
    $valid=@($samples | Where-Object {$_ -ge 5 -and $_ -le 120})
    if($valid.Count) {
        $max=($valid | Measure-Object -Maximum).Maximum
        [Console]::WriteLine($kind+' '+([double]$max).ToString('F1',$culture))
        exit 0
    }
}
try {
    $cpu = Get-CimInstance -Namespace 'root\LibreHardwareMonitor' -ClassName Sensor -OperationTimeoutSec 2 |
        Where-Object {
            $_.SensorType -eq 'Temperature' -and $null -ne $_.Value -and
            ($_.Parent -match '^/(intelcpu|amdcpu)/[0-9]+$' -or $_.Identifier -match '^/(intelcpu|amdcpu)/[0-9]+/temperature/') -and
            $_.Name -match '^(CPU Package|Core Max|(?:CPU|Core) \(T(?:die|ctl/Tdie)\)|CCD(?:[0-9]+|s Max) \(Tdie\)|(?:CPU )?(?:(?:P|E)-)?Core\s*#?\s*[0-9]+\b.*)$'
        } | ForEach-Object {[double]$_.Value}
    Emit 'CPU' $cpu
} catch {}
)PS");
        if (!allowProxy) return body;
        return body + QString::fromLatin1(R"PS(
function PlausibleZone($name, $celsius) {
    if (!$name -or $celsius -lt 5 -or $celsius -gt 120 -or
        $name -match '(^|[^A-Za-z0-9])(BAT[A-Z0-9]*|GFX[A-Z0-9]*|GPU[A-Z0-9]*|SKN[A-Z0-9]*|CHG[A-Z0-9]*|FAN[A-Z0-9]*)([^A-Za-z0-9]|$)') {
        return
    }
    [pscustomobject]@{C=$celsius; Cpu=($name -match '(^|[^A-Za-z0-9])(CPU[A-Z0-9]*|DTS[A-Z0-9]*|PKG[A-Z0-9]*|PROCESSOR)([^A-Za-z0-9]|$)')}
}
$zones=@()
try {
    $zones += @(Get-CimInstance -Namespace 'root\wmi' -ClassName MSAcpi_ThermalZoneTemperature -OperationTimeoutSec 2 |
        Where-Object {$_.Active -ne $false -and $null -ne $_.CurrentTemperature} |
        ForEach-Object {PlausibleZone $_.InstanceName (([double]$_.CurrentTemperature / 10.0) - 273.15)})
    Emit 'PROXY' ($zones | Where-Object Cpu | ForEach-Object C)
} catch {}
try {
    $zones += @(Get-CimInstance -Namespace 'root\cimv2' -ClassName Win32_PerfFormattedData_Counters_ThermalZoneInformation -OperationTimeoutSec 2 |
        Where-Object {$null -ne $_.Temperature} |
        ForEach-Object {PlausibleZone $_.Name ([double]$_.Temperature - 273.15)})
} catch {}
Emit 'PROXY' ($zones | Where-Object Cpu | ForEach-Object C)
Emit 'PROXY' ($zones | ForEach-Object C)
)PS");
    }
};
#endif

inline QByteArray Text(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.read(128).trimmed();
}

inline double Millidegrees(const QString& path)
{
    bool ok = false;
    const int value = Text(path).toInt(&ok);
    return ok && value >= -20000 && value <= 130000 ? value / 1000.0 : Missing();
}

inline void Maximum(double& value, double next)
{
    if (std::isfinite(next) && (!std::isfinite(value) || next > value)) value = next;
}

class Sensors {
public:
    Sensors(bool gpu, unsigned vendor, QString pci, QString fixture = {}, bool allowProxy = false, bool proxyFixture = false)
        : isGpu(gpu), vendorId(vendor), pciAddress(std::move(pci)), testFile(std::move(fixture)),
          allowWindowsProxy(allowProxy), testIsProxy(proxyFixture) {}

    Sensors(const Sensors&) = delete;
    ~Sensors() { if (initialized && shutdown) shutdown(); }

    Reading Read()
    {
        if (!testFile.isEmpty()) return {Millidegrees(testFile), Missing(), testIsProxy, true};
#ifdef Q_OS_WIN
        if (!isGpu) return windowsCpu.Read(allowWindowsProxy);
#endif
        if (isGpu && pciAddress.isEmpty()) return {};
        if (isGpu && vendorId == 0x10de) {
            const auto reading = Nvidia();
            if (std::isfinite(reading.core)) return reading;
        }
#ifdef Q_OS_LINUX
        const QDir root(isGpu ? QStringLiteral("/sys/bus/pci/devices/") + pciAddress + QStringLiteral("/hwmon")
                              : QStringLiteral("/sys/class/hwmon"));
        static const QRegularExpression channel(QStringLiteral("^temp[0-9]+_input$"));
        Reading result;
        for (const auto& entry : root.entryList({QStringLiteral("hwmon*")}, QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QDir device(root.filePath(entry));
            const auto name = Text(device.filePath(QStringLiteral("name")));
            if (!isGpu && name == "cpu_thermal") {
                Maximum(result.core, Millidegrees(device.filePath(QStringLiteral("temp1_input"))));
                continue;
            }
            if (isGpu && name != "amdgpu" && name != "nouveau") continue;
            if (!isGpu && name != "coretemp" && name != "k10temp" && name != "zenpower") continue;
            for (const auto& input : device.entryList({QStringLiteral("temp*_input")}, QDir::Files)) {
                if (!channel.match(input).hasMatch()) continue;
                const auto label = Text(device.filePath(input.chopped(6) + QStringLiteral("_label")));
                if (isGpu) {
                    if (label == "junction" || label == "hotspot")
                        Maximum(result.hotspot, Millidegrees(device.filePath(input)));
                    else if (label == "edge" || (name == "nouveau" && input == QStringLiteral("temp1_input")))
                        Maximum(result.core, Millidegrees(device.filePath(input)));
                } else if (label.startsWith("Package id ") || label.startsWith("Core ") ||
                           label == "Tdie" || label.startsWith("Tccd")) {
                    Maximum(result.core, Millidegrees(device.filePath(input)));
                }
            }
        }
        return result;
#else
        return {};
#endif
    }

private:
    bool isGpu;
    unsigned vendorId;
    QString pciAddress, testFile;
    bool allowWindowsProxy, testIsProxy;
#ifdef Q_OS_WIN
    WindowsCpuSensor windowsCpu;
#endif
    QLibrary nvml;
    bool attempted{false}, initialized{false};
    void* device{nullptr};
    int (*shutdown)(){nullptr};
    int (*temperature)(void*, unsigned, unsigned*){nullptr};

    Reading Nvidia()
    {
        if (!attempted) {
            attempted = true;
#ifdef Q_OS_WIN
            wchar_t folder[1024]{};
            const auto chars = GetSystemDirectoryW(folder, 1024);
            if (!chars || chars >= 1024) return {};
            nvml.setFileName(QString::fromWCharArray(folder, chars) + QStringLiteral("/nvml.dll"));
#elif defined(Q_OS_LINUX)
            nvml.setFileName(QStringLiteral("libnvidia-ml.so.1"));
#else
            return {};
#endif
            if (!nvml.load()) return {};
            auto init = reinterpret_cast<int (*)()>(nvml.resolve("nvmlInit_v2"));
            shutdown = reinterpret_cast<int (*)()>(nvml.resolve("nvmlShutdown"));
            auto handle = reinterpret_cast<int (*)(const char*, void**)>(nvml.resolve("nvmlDeviceGetHandleByPciBusId_v2"));
            temperature = reinterpret_cast<int (*)(void*, unsigned, unsigned*)>(nvml.resolve("nvmlDeviceGetTemperature"));
            if (!(init && shutdown && handle && temperature) || init() != 0) return {};
            initialized = true;
            if (handle(pciAddress.toLatin1().constData(), &device) != 0) device = nullptr;
        }
        unsigned current = 0;
        if (!device || temperature(device, 0, &current) != 0 || current > 130) return {};
        return {static_cast<double>(current), Missing()};
    }
};
} // namespace poolthermal
#endif
