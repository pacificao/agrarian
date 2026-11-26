// Distributed under the MIT software license, see the accompanying file COPYING.

#include "poolminingdialog.h"
#include "pooldestination.h"

#include "addresstablemodel.h"
#include "chainparams.h"
#include "util.h"
#include "walletmodel.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QTextDocument>
#include <QThread>
#include "poolthermalpolicy.h"

#include <algorithm>

namespace {
constexpr auto addressSetting = "poolMining/payoutAddress";
constexpr auto threadSetting = "poolMining/cpuThreads";
constexpr auto cpuLimitSetting = "poolMining/cpuTemperatureLimit";
constexpr auto gpuLimitSetting = "poolMining/gpuTemperatureLimit";
constexpr auto fallbackSetting = "poolMining/unmonitoredLowLoad";
constexpr auto proxySetting = "poolMining/windowsTemperatureProxy";
}

PoolMiningDialog::PoolMiningDialog(QWidget* parent) : QDialog(parent),
    process(new QProcess(this)), gpuProcess(new QProcess(this)),
    cpuChoice(new QCheckBox(tr("CPU"), this)), gpuChoice(new QCheckBox(tr("GPU"), this)),
    threadCount(new QSpinBox(this)),
    cpuLimit(new QSpinBox(this)), gpuLimit(new QSpinBox(this)),
    unmonitored(new QCheckBox(tr("Allow low-load mining if a temperature sensor is unavailable"), this)),
    windowsProxy(new QCheckBox(tr("Use Windows system temperature as an estimate for low-load CPU mining"), this)),
    cpuThermal(new QLabel(this)), gpuThermal(new QLabel(this)),
    status(new QLabel(this)), payoutAddress(new QLabel(this)),
    start(new QPushButton(tr("&Start pool mining"), this)),
    stop(new QPushButton(tr("S&top mining"), this)), output(new QPlainTextEdit(this))
{
    setWindowTitle(tr("All The Acronyms · Pool mining"));
    auto* description = new QLabel(tr("Mine AGR and receive pool payouts in this wallet. Mining uses power and produces heat. Payouts depend on pool blocks and are not guaranteed."), this);
    description->setWordWrap(true);
    auto* support = new QLabel(firstPartyPreview()
        ? tr("Test mode: choose CPU, GPU, or both. Mining stops when you close this window or quit Agrarian Core.")
        : tr("Choose CPU, GPU, or both. Mining stops when you close this window or quit Agrarian Core."), this);
    support->setWordWrap(true);

    payoutAddress->setTextFormat(Qt::PlainText);
    payoutAddress->setTextInteractionFlags(Qt::TextSelectableByMouse);
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    output->setReadOnly(true);
    output->setMaximumBlockCount(200);
    // Set both colors locally: the shared theme only styles the editor's
    // background, so some desktop palettes otherwise render pale text.
    output->setObjectName(QStringLiteral("poolMiningLog"));
    output->setStyleSheet(QStringLiteral(
        "QPlainTextEdit#poolMiningLog { background-color: #111827; color: #f3f4f6;"
        " selection-background-color: #1d4ed8; selection-color: #ffffff; }"));

    auto* form = new QFormLayout(this);
    form->addRow(description);
    form->addRow(tr("Pool:"), new QLabel(poolEndpoint(), this));
    cpuChoice->setObjectName(QStringLiteral("mineWithCpu"));
    gpuChoice->setObjectName(QStringLiteral("mineWithGpu"));
    cpuChoice->setChecked(true);
    auto* modes = new QHBoxLayout();
    modes->addWidget(cpuChoice);
    modes->addWidget(gpuChoice);
    modes->addStretch();
    form->addRow(tr("Mine with:"), modes);
    threadCount->setObjectName(QStringLiteral("poolCpuThreads"));
    threadCount->setRange(1, std::clamp(QThread::idealThreadCount(), 1, pooldestination::kMaxCpuThreads));
    threadCount->setValue(QSettings().value(threadSetting, 1).toInt());
    form->addRow(tr("CPU &threads:"), threadCount);
    cpuLimit->setObjectName(QStringLiteral("poolCpuTemperatureLimit"));
    gpuLimit->setObjectName(QStringLiteral("poolGpuTemperatureLimit"));
    cpuLimit->setRange(poolthermal::kCpuMin, poolthermal::kCpuMax);
    gpuLimit->setRange(poolthermal::kGpuMin, poolthermal::kGpuMax);
    cpuLimit->setSuffix(tr(" °C"));
    gpuLimit->setSuffix(tr(" °C"));
    cpuLimit->setValue(QSettings().value(cpuLimitSetting, poolthermal::kCpuDefault).toInt());
    gpuLimit->setValue(QSettings().value(gpuLimitSetting, poolthermal::kGpuDefault).toInt());
    cpuThermal->setText(tr("Not running"));
    gpuThermal->setText(tr("Not running"));
    form->addRow(tr("CPU temperature limit:"), cpuLimit);
    form->addRow(tr("GPU core limit:"), gpuLimit);
    const auto thermalHint = tr("Thermal Guard reduces mining near the limit and pauses when too warm. Without a supported sensor it pauses by default. GPU hotspot, when available, is checked separately at 10 °C above the core target.");
    auto* thermalHelp = new QLabel(thermalHint, this);
    thermalHelp->setWordWrap(true);
    form->addRow(thermalHelp);
    unmonitored->setObjectName(QStringLiteral("poolUnmonitoredLowLoad"));
    unmonitored->setChecked(QSettings().value(fallbackSetting, false).toBool());
    unmonitored->setToolTip(tr("Lower load is not temperature protection. Enable only if you can monitor cooling yourself."));
    form->addRow(unmonitored);
#ifdef Q_OS_WIN
    windowsProxy->setObjectName(QStringLiteral("poolWindowsTemperatureProxy"));
    windowsProxy->setChecked(QSettings().value(proxySetting, false).toBool());
    windowsProxy->setToolTip(tr("Approximate system temperature, not CPU temperature. Limits mining to one thread at low load, using a separate target of 60 °C or 10 °C below your CPU limit, whichever is lower. Mining pauses if the reading is unavailable."));
    form->addRow(windowsProxy);
#else
    windowsProxy->hide();
#endif
    form->addRow(tr("CPU Thermal Guard:"), cpuThermal);
    form->addRow(tr("GPU Thermal Guard:"), gpuThermal);
    form->addRow(tr("Payout address:"), payoutAddress);
    form->addRow(support);
    form->addRow(start, stop);
    form->addRow(status);
    form->addRow(output);
    resize(620, 560);

    start->setAutoDefault(false);
    stop->setAutoDefault(false);
    connect(start, &QPushButton::clicked, this, &PoolMiningDialog::startMining);
    connect(stop, &QPushButton::clicked, this, &PoolMiningDialog::stopMining);
    connect(cpuChoice, &QCheckBox::toggled, this, &PoolMiningDialog::updateControls);
    connect(gpuChoice, &QCheckBox::toggled, this, &PoolMiningDialog::updateControls);
    attachWorker(process, false);
    attachWorker(gpuProcess, true);
    updateControls();
}

PoolMiningDialog::~PoolMiningDialog()
{
    stopMining();
}

void PoolMiningDialog::setWallet(WalletModel* model)
{
    if (wallet != model) {
        stopMining();
        wallet = model;
    }
    const QString saved = QSettings().value(addressSetting).toString();
    payoutAddress->setText(wallet && wallet->ownsSpendableAddress(saved) ? saved : tr("Created when you start mining"));
    if (!isMining()) {
        status->setText(QFileInfo(minerPath()).isExecutable()
            ? tr("Ready. Mining will only start when you click Start.")
            : tr("This package does not include the required mining worker."));
    }
    updateControls();
}

bool PoolMiningDialog::isMining() const
{
    return process->state() != QProcess::NotRunning || gpuProcess->state() != QProcess::NotRunning;
}

QString PoolMiningDialog::minerPath() const
{
#ifdef Q_OS_WIN
    constexpr auto executable = "agrarian-pool-worker.exe";
#else
    constexpr auto executable = "agrarian-pool-worker";
#endif
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("miners/") + executable);
}

QString PoolMiningDialog::kernelPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("miners/quark-screen.cl"));
}

bool PoolMiningDialog::firstPartyPreview()
{
    return Params().NetworkID() == CBaseChainParams::REGTEST &&
           GetBoolArg("-regtestfirstpartypoolminer", false) && !poolEndpoint().isEmpty();
}

QStringList PoolMiningDialog::firstPartyArguments(const QString& payout, bool gpu, const QString& kernel, int threads)
{
    if (gpu && kernel.isEmpty()) return {};
    if (Params().NetworkID() == CBaseChainParams::MAIN)
        return gpu ? QStringList{"--pool-gpu", payout, kernel} : QStringList{"--pool-cpu", payout, QString::number(threads)};
    if (!firstPartyPreview()) return {};
    QStringList args{gpu ? "--regtest-opencl" : "--regtest-loopback",
                     QString::number(GetArg("-regtestpoolminingport", int64_t{0})), payout};
    args << (gpu ? kernel : QString::number(threads));
    return args;
}

QString PoolMiningDialog::poolEndpoint()
{
    if (Params().NetworkID() == CBaseChainParams::MAIN)
        return QStringLiteral("stratum+tls://%1:%2").arg(QString::fromLatin1(pooldestination::kHost))
            .arg(pooldestination::kTlsPort);
    if (Params().NetworkID() != CBaseChainParams::REGTEST) return {};
    // Installed wallet lifecycle tests can use a disposable local pool.
    // This cannot redirect the mainnet wallet or use an external hostname.
    const int64_t port = GetArg("-regtestpoolminingport", int64_t{0});
    return port > 0 && port <= 65535
        ? QStringLiteral("stratum+tcp://127.0.0.1:%1").arg(port) : QString();
}

void PoolMiningDialog::startMining()
{
    if (!wallet || isMining() || poolEndpoint().isEmpty() ||
        (Params().NetworkID() != CBaseChainParams::MAIN && !firstPartyPreview()))
        return;
    if (GetBoolArg("-gen", false)) {
        status->setText(tr("Stop solo mining before starting pool mining."));
        return;
    }
    const QFileInfo miner(minerPath());
    if (!miner.isFile() || !miner.isExecutable() || miner.isSymLink()) {
        status->setText(tr("The required mining worker is unavailable in this package."));
        updateControls();
        return;
    }
    const bool runCpu = cpuChoice->isChecked();
    bool runGpu = gpuChoice->isChecked();
    if (!runCpu && !runGpu) return;
    bool missingGpuKernel = false;
    if (runGpu) {
        const QFileInfo kernel(kernelPath());
        if (!kernel.isFile() || !kernel.isReadable() || kernel.isSymLink()) {
            if (!runCpu) {
                status->setText(tr("The bundled GPU kernel is missing. CPU mining is still available."));
                return;
            }
            missingGpuKernel = true;
            runGpu = false;
        }
    }

    QSettings settings;
    QString address = settings.value(addressSetting).toString();
    if (!wallet->ownsSpendableAddress(address)) {
        address = wallet->getAddressTableModel()->addRow(AddressTableModel::Receive, tr("Pool mining payouts"), QString());
    }
    if (!wallet || !wallet->ownsSpendableAddress(address)) {
        status->setText(tr("Could not create a payout address controlled by this wallet. Mining was not started."));
        return;
    }
    settings.setValue(addressSetting, address);
    settings.setValue(threadSetting, threadCount->value());
    settings.setValue(cpuLimitSetting, cpuLimit->value());
    settings.setValue(gpuLimitSetting, gpuLimit->value());
    settings.setValue(fallbackSetting, unmonitored->isChecked());
    settings.setValue(proxySetting, windowsProxy->isChecked());
    payoutAddress->setText(address);
    output->clear();
    if (missingGpuKernel) appendWorkerLog(true, tr("The bundled GPU kernel is missing. Starting only the selected CPU worker."));
    cpuPending.clear();
    gpuPending.clear();
    stopping = false;
    status->setText(tr("Starting selected mining workers…"));
    auto withThermal = [&](QStringList arguments, int limit, bool cpu) {
        // Regtest preview has no sensor requirement unless the worker test
        // explicitly supplies one. This never applies to mainnet mining.
        if (Params().NetworkID() == CBaseChainParams::MAIN) {
            arguments << QStringLiteral("--temperature-limit") << QString::number(limit);
            if (unmonitored->isChecked()) arguments << QStringLiteral("--allow-unmonitored-low-load");
            if (cpu && windowsProxy->isChecked()) arguments << QStringLiteral("--allow-windows-thermal-proxy");
        }
        return arguments;
    };
    if (runCpu) {
        cpuThermal->setText(tr("Waiting for temperature…"));
        process->setWorkingDirectory(miner.absolutePath());
        process->start(miner.absoluteFilePath(), withThermal(firstPartyArguments(address, false, {}, threadCount->value()), cpuLimit->value(), true));
    }
    if (runGpu) {
        gpuThermal->setText(tr("Waiting for temperature…"));
        gpuProcess->setWorkingDirectory(miner.absolutePath());
        gpuProcess->start(miner.absoluteFilePath(), withThermal(firstPartyArguments(address, true, kernelPath()), gpuLimit->value(), false));
    }
    updateControls();
}

void PoolMiningDialog::stopMining()
{
    if (!isMining()) return;
    stopping = true;
    // Workers do not write wallet data and can be stopped even during backoff.
    for (auto* worker : {process, gpuProcess})
        if (worker->state() != QProcess::NotRunning) worker->kill();
    for (auto* worker : {process, gpuProcess}) {
        if (worker->state() != QProcess::NotRunning)
            worker->waitForFinished(1000);
    }
    updateControls();
}

void PoolMiningDialog::done(int result)
{
    stopMining();
    QDialog::done(result);
}

void PoolMiningDialog::updateControls()
{
    const bool running = isMining();
    const QFileInfo miner(minerPath());
    start->setEnabled(wallet && !running && !poolEndpoint().isEmpty() &&
                      (Params().NetworkID() == CBaseChainParams::MAIN || firstPartyPreview()) &&
                      (cpuChoice->isChecked() || gpuChoice->isChecked()) &&
                      miner.isFile() && miner.isExecutable() && !miner.isSymLink());
    stop->setEnabled(running);
    cpuChoice->setEnabled(!running);
    gpuChoice->setEnabled(!running);
    threadCount->setEnabled(!running && cpuChoice->isChecked());
    cpuLimit->setEnabled(!running && cpuChoice->isChecked());
    gpuLimit->setEnabled(!running && gpuChoice->isChecked());
    unmonitored->setEnabled(!running);
    windowsProxy->setEnabled(!running && cpuChoice->isChecked());
}

void PoolMiningDialog::attachWorker(QProcess* worker, bool gpu)
{
    worker->setProcessChannelMode(QProcess::MergedChannels);
    worker->setStandardInputFile(QProcess::nullDevice());
    connect(worker, &QProcess::readyReadStandardOutput, this, [this, worker, gpu] { readOutput(worker, gpu); });
    connect(worker, &QProcess::stateChanged, this, [this] { updateControls(); });
    connect(worker, &QProcess::started, this, [this, gpu] {
        appendWorkerLog(gpu, tr("Worker started."));
        status->setText(tr("Mining worker running; waiting for an accepted share."));
    });
    connect(worker, &QProcess::errorOccurred, this, [this, worker, gpu](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            appendWorkerLog(gpu, tr("Could not start: %1").arg(worker->errorString()));
            if (!isMining()) status->setText(tr("Mining could not start. See the log."));
        }
    });
    connect(worker, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, worker, gpu](int code, QProcess::ExitStatus) {
        readOutput(worker, gpu, true);
        (gpu ? gpuThermal : cpuThermal)->setText(tr("Not running"));
        appendWorkerLog(gpu, stopping ? tr("Worker stopped.") : tr("Worker exited (code %1).").arg(code));
        if (!isMining()) status->setText(stopping ? tr("Mining stopped.") : tr("Mining ended. See the log."));
        updateControls();
    });
}

void PoolMiningDialog::appendWorkerLog(bool gpu, const QString& line)
{
    output->appendPlainText((gpu ? QStringLiteral("[GPU] ") : QStringLiteral("[CPU] ")) + line);
}

void PoolMiningDialog::readOutput(QProcess* worker, bool gpu, bool flush)
{
    static const QRegularExpression colors(QStringLiteral("\\x1b\\[[0-9;]*[A-Za-z]"));
    auto& pending = gpu ? gpuPending : cpuPending;
    pending = (pending + worker->readAllStandardOutput().right(16000)).right(16000);
    if (flush && !pending.endsWith('\n')) pending += '\n';
    while (pending.contains('\n')) {
        const auto end = pending.indexOf('\n');
        auto line = QString::fromLocal8Bit(pending.first(end));
        pending.remove(0, end + 1);
        line.remove(colors);
        line = line.trimmed();
        if (line.isEmpty()) continue;
        if (line.startsWith(QStringLiteral("THERMAL "))) {
            const auto values = line.split(QLatin1Char(' '));
            if (values.size() != 5) continue;
            const bool proxy = values[4] == QStringLiteral("proxy");
            const auto state = values[1];
            QString summary;
            if (state == QStringLiteral("normal")) summary = proxy ? tr("Low load") : tr("Normal");
            else if (state == QStringLiteral("throttling")) summary = tr("Throttling");
            else if (state == QStringLiteral("cooling")) summary = tr("Paused · cooling");
            else if (state == QStringLiteral("unavailable")) summary = tr("Paused · temperature unavailable");
            else if (state == QStringLiteral("low-load")) summary = tr("Low load · temperature unavailable");
            else continue;
            if (values[2] != QStringLiteral("-")) {
                if (proxy) summary += tr(" · System temperature (estimate): %1 °C").arg(values[2]);
                else summary += gpu ? tr(" · Core %1 °C").arg(values[2]) : tr(" · CPU sensor: %1 °C").arg(values[2]);
            }
            if (values[3] != QStringLiteral("-"))
                summary += tr(" · Hotspot %1 °C").arg(values[3]);
            (gpu ? gpuThermal : cpuThermal)->setText(summary);
            continue;
        }
        appendWorkerLog(gpu, line);
        if (stopping || !isMining()) continue;
        if (line.startsWith(QStringLiteral("Pool reconnecting in ")))
            status->setText(gpu ? tr("GPU miner reconnecting to the pool…") : tr("CPU miner reconnecting to the pool…"));
        else if (line == QStringLiteral("Pool authorized; waiting for shares"))
            status->setText(gpu ? tr("GPU miner connected; waiting for shares.") : tr("CPU miner connected; waiting for shares."));
        else if (line == QStringLiteral("Share accepted"))
            status->setText(tr("The pool accepted a share. Mining is running."));
    }
}
