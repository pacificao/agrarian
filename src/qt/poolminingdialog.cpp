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
#include <QTextDocument>

#include <algorithm>

namespace {
constexpr auto addressSetting = "poolMining/payoutAddress";
}

PoolMiningDialog::PoolMiningDialog(QWidget* parent) : QDialog(parent),
    process(new QProcess(this)), gpuProcess(new QProcess(this)),
    cpuChoice(new QCheckBox(tr("CPU"), this)), gpuChoice(new QCheckBox(tr("GPU"), this)),
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
    form->addRow(tr("Payout address:"), payoutAddress);
    form->addRow(support);
    form->addRow(start, stop);
    form->addRow(status);
    form->addRow(output);
    resize(600, 450);

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

QStringList PoolMiningDialog::firstPartyArguments(const QString& payout, bool gpu, const QString& kernel)
{
    if (gpu && kernel.isEmpty()) return {};
    if (Params().NetworkID() == CBaseChainParams::MAIN)
        return gpu ? QStringList{"--pool-gpu", payout, kernel} : QStringList{"--pool-cpu", payout};
    if (!firstPartyPreview()) return {};
    QStringList args{gpu ? "--regtest-opencl" : "--regtest-loopback",
                     QString::number(GetArg("-regtestpoolminingport", int64_t{0})), payout};
    if (gpu) args << kernel;
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
    payoutAddress->setText(address);
    output->clear();
    if (missingGpuKernel) appendWorkerLog(true, tr("The bundled GPU kernel is missing. Starting only the selected CPU worker."));
    cpuPending.clear();
    gpuPending.clear();
    stopping = false;
    status->setText(tr("Starting selected mining workers…"));
    if (runCpu) {
        process->setWorkingDirectory(miner.absolutePath());
        process->start(miner.absoluteFilePath(), firstPartyArguments(address, false, {}));
    }
    if (runGpu) {
        gpuProcess->setWorkingDirectory(miner.absolutePath());
        gpuProcess->start(miner.absoluteFilePath(), firstPartyArguments(address, true, kernelPath()));
    }
    updateControls();
}

void PoolMiningDialog::stopMining()
{
    if (!isMining()) return;
    stopping = true;
    for (auto* worker : {process, gpuProcess})
        if (worker->state() != QProcess::NotRunning) worker->terminate();
    for (auto* worker : {process, gpuProcess}) {
        if (worker->state() != QProcess::NotRunning && !worker->waitForFinished(350)) {
            worker->kill();
            worker->waitForFinished(1000);
        }
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
        appendWorkerLog(gpu, line);
        if (!stopping && isMining() && line == QStringLiteral("Share accepted"))
            status->setText(tr("The pool accepted a share. Mining is running."));
    }
}
