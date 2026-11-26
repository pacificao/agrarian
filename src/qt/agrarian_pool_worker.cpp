// Copyright (c) The Agrarian developers
// Distributed under the MIT software license, see COPYING.
#ifdef HAVE_CONFIG_H
#include "config/agrarian-config.h"
#endif
#include "pooldestination.h"
#include "poolframes.h"
#include "poolopencl.h"
#include "pooltemperatures.h"
#include "poolthermalpolicy.h"
#include "poolwork.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QRandomGenerator>
#include <QSslSocket>
#include <QThread>
#if defined(QT_STATICPLUGIN) && defined(QT_TLS_OPENSSL)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QTlsBackendOpenSSL);
#endif

#include <boost/multiprecision/cpp_int.hpp>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace {
using poolframes::Kind;
using WideInteger = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
    1536, 1536, boost::multiprecision::unsigned_magnitude, boost::multiprecision::checked, void>>;

class RetryablePoolError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void Require(bool valid, const char* error)
{
    if (!valid) throw std::runtime_error(error);
}

bool IsHex(const QByteArray& bytes)
{
    return std::all_of(bytes.begin(), bytes.end(), [](char byte) {
        return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f') || (byte >= 'A' && byte <= 'F');
    });
}

QByteArray Hex(const QJsonValue& value, qsizetype minBytes, qsizetype maxBytes)
{
    Require(value.isString(), "Expected a Stratum hex string");
    const auto bytes = value.toString().toLatin1();
    Require(bytes.size() >= 2 * minBytes && bytes.size() <= 2 * maxBytes &&
            bytes.size() % 2 == 0 && IsHex(bytes), "Invalid Stratum hex string");
    return bytes;
}

poolwork::Target HashBytes(const QJsonValue& value)
{
    poolwork::Target result{};
    const auto bytes = QByteArray::fromHex(Hex(value, 32, 32));
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}

poolwork::Target ShareTarget(double difficulty)
{
    Require(std::isfinite(difficulty) && difficulty > 0, "Invalid share difficulty");
    // Agrarian Miningcore Quark uses Bitcoin's diff1 target and multiplier 1.
    // Decode the double into an integer significand and a power of two so
    // the final 256-bit comparison doesn't lose accuracy at an equality.
    int exponent = 0;
    const auto significand = static_cast<uint64_t>(std::ldexp(std::frexp(difficulty, &exponent), 53));
    exponent -= 53;
    WideInteger numerator = WideInteger(0xffff) << 208;
    WideInteger denominator = significand;
    if (exponent < 0) numerator <<= -exponent;
    else denominator <<= exponent;
    WideInteger target = numerator / denominator;
    const WideInteger maxTarget = (WideInteger(1) << 256) - 1;
    target = std::min(target, maxTarget);
    poolwork::Target result{};
    for (auto& byte : result) {
        byte = static_cast<unsigned char>(target & 255);
        target >>= 8;
    }
    return result;
}

struct Job {
    QString id;
    QByteArray prefix, suffix, time;
    poolwork::Target previous{}, target{};
    std::vector<poolwork::Target> branches;
    uint32_t version{0}, bits{0};
};

bool Advance(QByteArray& extraNonce)
{
    for (auto& byte : extraNonce) {
        const auto next = static_cast<unsigned char>(static_cast<unsigned char>(byte) + 1);
        byte = static_cast<char>(next);
        if (next != 0) return true;
    }
    return false;
}

class ThermalControl {
public:
    ThermalControl(int target, bool fallback, bool proxy, poolopencl::Scanner* gpu, unsigned threads,
                   QString fixture, bool proxyFixture)
        : limit(target), allowFallback(fallback), allowProxy(proxy), maxThreads(threads),
          sensors(gpu != nullptr, gpu ? gpu->Vendor() : 0, gpu ? gpu->PciAddress() : QString{}, std::move(fixture), proxy, proxyFixture)
    {
        clock.start();
    }

    void Poll()
    {
        const auto time = clock.elapsed();
        if (time < nextSample) return;
        nextSample = time + 1000;
        auto temperatures = sensors.Read();
        if (!temperatures.fresh) return;
        // Never silently switch to a cooler system proxy if a real sensor
        // disappears during the session. A valid direct reading must return.
        if (temperatures.proxy && (!allowProxy || trustedSeen)) temperatures = {};
        if (!temperatures.proxy && (std::isfinite(temperatures.core) || std::isfinite(temperatures.hotspot))) trustedSeen = true;
        double delta = poolthermal::Missing();
        const auto target = temperatures.proxy ? std::min(60, limit - 10) : limit;
        poolthermal::Maximum(delta, temperatures.core - target);
        poolthermal::Maximum(delta, temperatures.hotspot - limit - poolthermal::kHotspotOffset);
        load = policy.Sample(delta, temperatures.proxy ? 1u : maxThreads, allowFallback && !allowProxy && !trustedSeen);
        if (temperatures.proxy && load.threads) load.delayMs = std::max(250, load.delayMs);
        static const char* names[] = {"normal", "throttling", "cooling", "unavailable", "low-load"};
        const auto display = [](double value) {
            return std::isfinite(value) ? QByteArray::number(value, 'f', 1) : QByteArray("-");
        };
        std::printf("THERMAL %s %s %s %s\n", names[static_cast<int>(load.state)], display(temperatures.core).constData(),
                    display(temperatures.hotspot).constData(), temperatures.proxy ? "proxy" : "direct");
        std::fflush(stdout);
    }

    unsigned Threads() const { return clock.elapsed() >= nextWork ? load.threads : 0; }
    unsigned CpuBatch() const { return load.delayMs >= 250 ? 128 : 1024; }
    void Worked() { nextWork = clock.elapsed() + load.delayMs; }

private:
    int limit;
    bool allowFallback;
    bool allowProxy;
    bool trustedSeen{false};
    unsigned maxThreads;
    poolthermal::Sensors sensors;
    poolthermal::Policy policy;
    poolthermal::Load load;
    QElapsedTimer clock;
    qint64 nextSample{0}, nextWork{0};
};

class Worker {
public:
    Worker(QString payout, poolopencl::Scanner* gpu, unsigned threads, ThermalControl* control)
        : worker(std::move(payout) + (gpu ? QStringLiteral(".agrarian-qt-gpu") :
                                           QStringLiteral(".agrarian-qt-cpu"))), scanner(gpu), cpuThreads(threads), thermal(control) {}

    void Run(quint16 port, bool mainnet)
    {
        socket.setProxy(QNetworkProxy::NoProxy);
        socket.setReadBufferSize(poolframes::kMaxFrameBytes + 2);
        if (mainnet) {
            Require(QSslSocket::supportsSsl(), "TLS backend unavailable; no mining connection was made");
            socket.setProtocol(QSsl::TlsV1_2OrLater);
            socket.setPeerVerifyMode(QSslSocket::VerifyPeer);
            // No verification exception or downgrade is allowed on any retry.
            bool certificateError = false;
            const auto verification = QObject::connect(&socket, &QSslSocket::sslErrors, &socket,
                [&](const QList<QSslError>&) { certificateError = true; }, Qt::DirectConnection);
            socket.connectToHostEncrypted(QString::fromLatin1(pooldestination::kHost), pooldestination::kTlsPort);
            const bool encrypted = socket.waitForEncrypted(10000);
            QObject::disconnect(verification);
            Require(!certificateError && socket.error() != QAbstractSocket::SslHandshakeFailedError,
                    "Cannot establish a verified TLS connection to the Agrarian pool");
            if (!encrypted) throw RetryablePoolError("Pool TLS connection timed out or was interrupted");
        } else {
            socket.connectToHost(QHostAddress::LocalHost, port);
            if (!socket.waitForConnected(5000)) throw RetryablePoolError("Cannot connect to the local test pool");
        }
        Send(1, "mining.subscribe", {"Agrarian-FirstParty/2.1.5"});
        handshake.start();
        sinceLastFrame.start();

        while (socket.state() == QAbstractSocket::ConnectedState) {
            // Read control messages between small CPU batches. Never queue
            // work offline or submit before subscribe AND authorize succeed.
            QCoreApplication::processEvents();
            while (socket.canReadLine()) ReadFrame();
            Require(socket.bytesAvailable() <= poolframes::kMaxFrameBytes, "Oversized incomplete pool frame");
            if (!authorized && handshake.elapsed() >= 15000)
                throw RetryablePoolError("Pool subscription or authorization timed out");
            if (sinceLastFrame.elapsed() >= 120000)
                throw RetryablePoolError("Pool no longer responding");
            if (pendingShare && sinceSubmission.elapsed() >= 30000)
                throw RetryablePoolError("Pool did not acknowledge the last share");
            if (thermal) thermal->Poll();
            const unsigned activeThreads = thermal ? thermal->Threads() : cpuThreads;
            if (subscribed && authorized && haveWork && !pendingShare && activeThreads) {
                const auto result = scanner ? scanner->Scan(header, job.target, nextNonce)
                                            : poolwork::ScanCpuParallel(header, job.target, nextNonce,
                                                                        thermal ? thermal->CpuBatch() : 1024, activeThreads, runningStop);
                if (thermal) thermal->Worked();
                nextNonce = result.nextNonce;
                if (result.found) {
                    Require(nextRequest < 2147483647, "Pool session request limit reached");
                    pendingShare = nextRequest++;
                    sinceSubmission.start();
                    Send(pendingShare, "mining.submit", {worker, job.id, QString::fromLatin1(extraNonce.toHex()),
                                              QString::fromLatin1(job.time),
                                              QStringLiteral("%1").arg(result.nonce, 8, 16, QLatin1Char('0'))});
                } else if (nextNonce == poolwork::kNonceEnd) {
                    haveWork = Advance(extraNonce);
                    if (haveWork) PrepareWork();
                }
            } else {
                socket.waitForReadyRead(thermal ? 50 : 200);
            }
        }
        throw RetryablePoolError("Pool disconnected");
    }

    bool HadStableSession() const { return connectedWork.isValid() && connectedWork.elapsed() >= 60000; }

private:
    QSslSocket socket;
    const QString worker;
    poolopencl::Scanner* const scanner;
    const unsigned cpuThreads;
    ThermalControl* const thermal;
    QElapsedTimer handshake, sinceLastFrame, sinceSubmission, connectedWork;
    std::atomic<bool> runningStop{false}; // prototype stop uses process termination
    bool subscribed{false}, authorized{false}, haveWork{false}, hasJob{false};
    int nextRequest{3}, pendingShare{0};
    QByteArray extraPrefix, extraNonce;
    uint64_t nextNonce{0};
    double nextDifficulty{1};
    Job job;
    poolwork::Header header{};

    void Send(int id, const char* method, const QJsonArray& parameters)
    {
        const auto body = QJsonDocument(QJsonObject{{"id", id}, {"method", method}, {"params", parameters}})
                              .toJson(QJsonDocument::Compact) + '\n';
        if (socket.write(body) != body.size() ||
            (socket.bytesToWrite() && !socket.waitForBytesWritten(5000)))
            throw RetryablePoolError("Pool write failed");
    }

    void ReadFrame()
    {
        auto line = socket.readLine(poolframes::kMaxFrameBytes + 2);
        Require(line.endsWith('\n'), "Oversized pool frame");
        line.chop(1);
        if (line.endsWith('\r')) line.chop(1);
        const auto kind = poolframes::Classify(line);
        Require(kind != Kind::Invalid, "Malformed pool frame");
        sinceLastFrame.restart();
        if (kind == Kind::Ignore) return;
        const auto message = QJsonDocument::fromJson(line).object();
        const auto parameters = message.value("params").toArray();
        switch (kind) {
            case Kind::Reply: HandleReply(message); break;
            case Kind::Difficulty:
                Require(parameters.size() == 1 && parameters[0].isDouble(), "Invalid difficulty frame");
                nextDifficulty = parameters[0].toDouble();
                Require(std::isfinite(nextDifficulty) && nextDifficulty > 0, "Invalid share difficulty");
                break;
            case Kind::ExtraNonce:
                SetExtraNonce(parameters, 0);
                // An old job was built with a different extraNonce1.
                hasJob = haveWork = false;
                break;
            case Kind::Work: HandleJob(parameters); break;
            default: break;
        }
    }

    void SetExtraNonce(const QJsonArray& params, int position)
    {
        Require(params.size() >= position + 2, "Missing pool extraNonce fields");
        extraPrefix = Hex(params[position], 0, 16);
        const int count = params[position + 1].toInt(-1);
        Require(count >= 1 && count <= 16, "Unsupported pool extraNonce width");
        extraNonce = QByteArray(count, '\0');
    }

    void HandleReply(const QJsonObject& message)
    {
        const int id = message.value("id").toInt();
        const auto error = message.value("error");
        const auto result = message.value("result");
        const bool good = error.isUndefined() || error.isNull();
        if (id == 1 && !subscribed) {
            Require(good && result.isArray(), "Pool rejected subscription");
            SetExtraNonce(result.toArray(), 1);
            subscribed = true;
            if (hasJob) PrepareWork();
            Send(2, "mining.authorize", {worker, "x"});
        } else if (id == 2 && subscribed && !authorized) {
            Require(good && result.isBool() && result.toBool(), "Pool rejected authorization");
            authorized = true;
            if (haveWork) connectedWork.start();
            std::puts("Pool authorized; waiting for shares");
            std::fflush(stdout);
        } else if (pendingShare && id == pendingShare) {
            pendingShare = 0;
            std::puts(good && result.isBool() && result.toBool() ? "Share accepted" : "Share rejected");
            std::fflush(stdout);
        }
    }

    void HandleJob(const QJsonArray& params)
    {
        Require(params.size() == 9 && params[0].isString() && params[4].isArray() &&
                params[8].isBool(), "Invalid Miningcore job");
        Job next;
        next.id = params[0].toString();
        Require(!next.id.isEmpty() && next.id.size() <= 64, "Invalid job ID length");
        next.previous = HashBytes(params[1]);
        next.prefix = Hex(params[2], 0, poolwork::kMaxCoinbaseBytes);
        next.suffix = Hex(params[3], 0, poolwork::kMaxCoinbaseBytes);
        const auto branches = params[4].toArray();
        Require(branches.size() <= poolwork::kMaxMerkleDepth, "Too many Merkle branches");
        for (const auto value : branches) next.branches.push_back(HashBytes(value));
        next.version = Hex(params[5], 4, 4).toUInt(nullptr, 16);
        next.bits = Hex(params[6], 4, 4).toUInt(nullptr, 16);
        next.time = Hex(params[7], 4, 4);
        next.target = ShareTarget(nextDifficulty);
        if (hasJob && job.id == next.id && job.previous == next.previous &&
            job.prefix == next.prefix && job.suffix == next.suffix && job.branches == next.branches &&
            job.version == next.version && job.bits == next.bits && job.time == next.time) {
            job.target = next.target;
            return; // job rebroadcast must never rewind the nonce cursor
        }
        job = std::move(next);
        hasJob = true;
        if (subscribed) {
            extraNonce.fill('\0');
            PrepareWork();
        }
    }

    void PrepareWork()
    {
        const auto coinbase = QByteArray::fromHex(job.prefix + extraPrefix + extraNonce.toHex() + job.suffix);
        auto raw = reinterpret_cast<const unsigned char*>(coinbase.constData());
        Require(poolwork::BuildHeader(job.previous, {raw, raw + coinbase.size()}, job.branches, job.version,
                                      job.time.toUInt(nullptr, 16), job.bits, header), "Invalid coinbase or Merkle branch");
        nextNonce = 0;
        haveWork = true;
        if (authorized && !connectedWork.isValid()) connectedWork.start();
    }
};

void RunWithReconnect(quint16 port, bool mainnet, const QString& payout, poolopencl::Scanner* scanner,
                      unsigned threads, ThermalControl* thermal)
{
    unsigned delay = 1;
    for (;;) {
        // Use a fresh Stratum session each time. In particular, never carry
        // over the last job, nonce, authorization, or unacknowledged share.
        bool stable = false;
        try {
            Worker session(payout, scanner, threads, thermal);
            try {
                session.Run(port, mainnet);
                return;
            } catch (const RetryablePoolError&) {
                stable = session.HadStableSession();
                throw;
            }
        } catch (const RetryablePoolError& error) {
            if (stable) delay = 1;
            const unsigned wait = delay * 1000 + QRandomGenerator::global()->bounded(250u);
            std::printf("Pool reconnecting in %.1f seconds: %s\n", wait / 1000.0, error.what());
            std::fflush(stdout);
            // This worker process belongs to one opt-in CPU or GPU session.
            // Stop/close terminates it even when it is asleep between retries.
            QThread::msleep(wait);
            delay = std::min(delay * 2, 60u);
        }
    }
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    auto args = app.arguments();
#ifdef Q_OS_WIN
    if (args.size() == 2 && args[1] == QStringLiteral("--probe-windows-cpu-thermal")) {
        poolthermal::Sensors sensors(false, 0, {}, {}, true);
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 7000) {
            QCoreApplication::processEvents();
            const auto sample = sensors.Read();
            if (std::isfinite(sample.core)) {
                std::printf("%s %s\n", sample.proxy ? "PROXY" : "CPU", QByteArray::number(sample.core, 'f', 1).constData());
                return 0;
            }
            QThread::msleep(50);
        }
        std::puts("UNAVAILABLE");
        return 0;
    }
#endif
    if (args.size() == 2 && args[1] == QStringLiteral("--check-tls-backend")) {
        if (!QSslSocket::supportsSsl()) {
            std::fputs("TLS backend unavailable\n", stderr);
            return 1;
        }
        std::puts("TLS backend available");
        return 0;
    }
    int thermalLimit = 0;
    bool overrideLimit = false, allowFallback = false, allowProxy = false, proxyFixture = false;
    QString fixture;
    for (int i = 2; i < args.size();) {
        const auto option = args[i];
        if (option == QStringLiteral("--allow-unmonitored-low-load")) {
            if (allowFallback) return 2;
            allowFallback = true;
            args.removeAt(i);
        } else if (option == QStringLiteral("--allow-windows-thermal-proxy")) {
            if (allowProxy) return 2;
            allowProxy = true;
            args.removeAt(i);
        } else if (option == QStringLiteral("--regtest-thermal-proxy")) {
            if (proxyFixture) return 2;
            proxyFixture = true;
            args.removeAt(i);
        } else if (option == QStringLiteral("--temperature-limit")) {
            bool parsed = false;
            if (overrideLimit || i + 1 >= args.size()) return 2;
            thermalLimit = args[i + 1].toInt(&parsed);
            if (!parsed) return 2;
            overrideLimit = true;
            args.removeAt(i + 1);
            args.removeAt(i);
        } else if (option == QStringLiteral("--regtest-thermal-sensor")) {
            if (!fixture.isEmpty() || i + 1 >= args.size() || args[i + 1].isEmpty()) return 2;
            fixture = args[i + 1];
            args.removeAt(i + 1);
            args.removeAt(i);
        } else {
            ++i;
        }
    }
    const bool mainCpu = (args.size() == 3 || args.size() == 4) && args[1] == QStringLiteral("--pool-cpu");
    const bool mainGpu = args.size() == 4 && args[1] == QStringLiteral("--pool-gpu");
    const bool mainnet = mainCpu || mainGpu;
    const bool gpu = mainGpu || (args.size() == 5 && args[1] == QStringLiteral("--regtest-opencl"));
    const bool software = args.size() == 5 && args[1] == QStringLiteral("--regtest-opencl-software");
    const bool localCpu = (args.size() == 4 || args.size() == 5) && args[1] == QStringLiteral("--regtest-loopback");
    if ((mainnet && (!fixture.isEmpty() || proxyFixture)) || (!mainnet && !fixture.isEmpty() && !overrideLimit)
        || (proxyFixture && (fixture.isEmpty() || !allowProxy)) || (allowProxy && !(mainCpu || localCpu))) return 2;
    if (!mainnet && !gpu && !software && !localCpu) {
        std::fputs("Usage: agrarian-pool-worker --pool-cpu PUBLIC_PAYOUT [THREADS]\n"
                   "       agrarian-pool-worker --pool-gpu PUBLIC_PAYOUT PINNED_KERNEL\n"
                   "       agrarian-pool-worker --regtest-loopback PORT PUBLIC_PAYOUT [THREADS]\n"
                   "       agrarian-pool-worker --regtest-opencl PORT PUBLIC_PAYOUT PINNED_KERNEL\n", stderr);
        return 2;
    }
    const bool gpuLoad = gpu || software;
    if (!overrideLimit) thermalLimit = gpuLoad ? poolthermal::kGpuDefault : poolthermal::kCpuDefault;
    if (thermalLimit < (gpuLoad ? poolthermal::kGpuMin : poolthermal::kCpuMin) ||
        thermalLimit > (gpuLoad ? poolthermal::kGpuMax : poolthermal::kCpuMax)) {
        std::fputs("Thermal limit is outside the supported range\n", stderr);
        return 2;
    }
    bool ok = mainnet;
    const auto port = mainnet ? quint16(pooldestination::kTlsPort) : args[2].toUShort(&ok);
    const auto payout = args[mainnet ? 2 : 3];
    const QString alphabet = QStringLiteral("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
    if (!ok || !port || payout.size() < 26 || payout.size() > 40 ||
        !std::all_of(payout.begin(), payout.end(), [&](QChar c) { return alphabet.contains(c); }))
        return 2;
    int threads = 1;
    if ((mainCpu && args.size() == 4) || (localCpu && args.size() == 5)) {
        threads = args.last().toInt(&ok);
        const auto maxThreads = std::clamp(QThread::idealThreadCount(), 1, pooldestination::kMaxCpuThreads);
        if (!ok || threads < 1 || threads > maxThreads) {
            std::fputs("Invalid CPU thread count\n", stderr);
            return 2;
        }
    }
    try {
        std::unique_ptr<poolopencl::Scanner> scanner;
        if (gpu || software) {
            QFile kernel(args[mainnet ? 3 : 4]);
            Require(kernel.open(QIODevice::ReadOnly), "Cannot read the staged OpenCL kernel");
            Require(kernel.size() == 186550, "Unexpected OpenCL kernel size");
            const auto data = kernel.read(186551);
            Require(data.size() == 186550 && QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() ==
                    "9387b4e30715e7599843d24bc3bdded5921788d54f2e46dc5cddabf6bc8f5727",
                    "OpenCL kernel does not match the pinned source");
            scanner = std::make_unique<poolopencl::Scanner>(data, software);
            std::puts(software ? "Software OpenCL self-test passed (regtest only)" : "GPU Quark self-test passed");
            std::fflush(stdout);
        }
        if (!gpu && !software) {
            std::printf("CPU worker configured for %d threads\n", threads);
            std::fflush(stdout);
        }
        std::unique_ptr<ThermalControl> thermal;
        if (mainnet || overrideLimit)
            thermal = std::make_unique<ThermalControl>(thermalLimit, allowFallback, allowProxy, scanner.get(), threads, fixture, proxyFixture);
        RunWithReconnect(port, mainnet, payout, scanner.get(), threads, thermal.get());
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
