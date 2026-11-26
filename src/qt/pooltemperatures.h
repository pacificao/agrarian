// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_POOLTEMPERATURES_H
#define AGRARIAN_QT_POOLTEMPERATURES_H

#include <QDir>
#include <QFile>
#include <QLibrary>
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
};

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
    Sensors(bool gpu, unsigned vendor, QString pci, QString fixture = {})
        : isGpu(gpu), vendorId(vendor), pciAddress(std::move(pci)), testFile(std::move(fixture)) {}

    Sensors(const Sensors&) = delete;
    ~Sensors() { if (initialized && shutdown) shutdown(); }

    Reading Read()
    {
        if (!testFile.isEmpty()) return {Millidegrees(testFile), Missing()};
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
