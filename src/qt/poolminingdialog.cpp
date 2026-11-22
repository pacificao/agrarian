// Distributed under the MIT software license, see the accompanying file COPYING.

#include "poolminingdialog.h"

#include "addresstablemodel.h"
#include "chainparams.h"
#include "util.h"
#include "walletmodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QTextDocument>
#include <QThread>

#include <algorithm>

namespace {
constexpr auto poolUrl = "stratum+tcp://alltheacronyms.com:3032";
constexpr auto addressSetting = "poolMining/payoutAddress";
constexpr auto threadSetting = "poolMining/cpuThreads";
}

PoolMiningDialog::PoolMiningDialog(QWidget* parent) : QDialog(parent),
    process(new QProcess(this)), threadCount(new QSpinBox(this)),
    status(new QLabel(this)), payoutAddress(new QLabel(this)),
    start(new QPushButton(tr("&Start pool mining"), this)),
    stop(new QPushButton(tr("S&top mining"), this)), output(new QPlainTextEdit(this))
{
    setWindowTitle(tr("All The Acronyms · Pool mining"));
    auto* description = new QLabel(tr("Mine AGR with your CPU and receive pool payouts in this wallet. Mining uses power and produces heat. Payouts depend on pool blocks and are not guaranteed."), this);
    description->setWordWrap(true);
    auto* support = new QLabel(tr("CPU only in this build. Mining stops when you close this window or quit Agrarian Core."), this);
    support->setWordWrap(true);

    threadCount->setRange(1, std::max(1, QThread::idealThreadCount()));
    threadCount->setValue(QSettings().value(threadSetting, 1).toInt());
    payoutAddress->setTextFormat(Qt::PlainText);
    payoutAddress->setTextInteractionFlags(Qt::TextSelectableByMouse);
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    output->setReadOnly(true);
    output->setMaximumBlockCount(200);

    auto* form = new QFormLayout(this);
    form->addRow(description);
    form->addRow(tr("Pool:"), new QLabel(poolEndpoint(), this));
    form->addRow(tr("CPU &threads:"), threadCount);
    form->addRow(tr("Payout address:"), payoutAddress);
    form->addRow(support);
    form->addRow(start, stop);
    form->addRow(status);
    form->addRow(output);
    resize(600, 450);

    start->setAutoDefault(false);
    stop->setAutoDefault(false);
    process->setProcessChannelMode(QProcess::MergedChannels);
    // No wallet RPC credentials or private keys are passed to the miner.
    process->setStandardInputFile(QProcess::nullDevice());
    connect(start, &QPushButton::clicked, this, &PoolMiningDialog::startMining);
    connect(stop, &QPushButton::clicked, this, &PoolMiningDialog::stopMining);
    connect(process, &QProcess::readyReadStandardOutput, this, &PoolMiningDialog::readOutput);
    connect(process, &QProcess::stateChanged, this, [this] { updateControls(); });
    connect(process, &QProcess::started, this, [this] {
        status->setText(tr("Mining process running; waiting for an accepted share."));
    });
    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            status->setText(tr("Could not start the bundled CPU miner."));
            output->appendPlainText(process->errorString());
        }
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
        readOutput();
        status->setText(stopping ? tr("Mining stopped.") : tr("CPU miner exited (code %1). See the output below.").arg(code));
        updateControls();
    });
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
            : tr("This package does not include the approved CPU miner."));
    }
    updateControls();
}

bool PoolMiningDialog::isMining() const
{
    return process->state() != QProcess::NotRunning;
}

QString PoolMiningDialog::minerPath() const
{
#ifdef Q_OS_WIN
    constexpr auto executable = "cpuminer.exe";
#else
    constexpr auto executable = "cpuminer";
#endif
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("miners/") + executable);
}

QStringList PoolMiningDialog::minerArguments(const QString& payout, int threads)
{
    // No shell, no arbitrary command line, and no local miner API listener.
    return {"-a", "quark", "-o", poolEndpoint(), "-u", payout + ".agrarian-qt",
            "-p", "x", "-t", QString::number(threads), "-b", "0"};
}

QString PoolMiningDialog::poolEndpoint()
{
    if (Params().NetworkID() == CBaseChainParams::MAIN) return poolUrl;
    if (Params().NetworkID() != CBaseChainParams::REGTEST) return {};
    // Installed wallet lifecycle tests can use a disposable local pool.
    // This cannot redirect the mainnet wallet or use an external hostname.
    const int64_t port = GetArg("-regtestpoolminingport", int64_t{0});
    return port > 0 && port <= 65535
        ? QStringLiteral("stratum+tcp://127.0.0.1:%1").arg(port) : QString();
}

void PoolMiningDialog::startMining()
{
    if (!wallet || isMining() || poolEndpoint().isEmpty())
        return;
    if (GetBoolArg("-gen", false)) {
        status->setText(tr("Stop solo mining before starting pool mining."));
        return;
    }
    const QFileInfo miner(minerPath());
    if (!miner.isFile() || !miner.isExecutable() || miner.isSymLink()) {
        status->setText(tr("The approved CPU miner is unavailable in this package."));
        updateControls();
        return;
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
    payoutAddress->setText(address);
    output->clear();
    stopping = false;
    status->setText(tr("Starting CPU miner…"));
    process->setWorkingDirectory(miner.absolutePath());
    process->start(miner.absoluteFilePath(), minerArguments(address, threadCount->value()));
    updateControls();
}

void PoolMiningDialog::stopMining()
{
    if (!isMining()) return;
    stopping = true;
    process->terminate();
    if (!process->waitForFinished(350)) {
        process->kill();
        process->waitForFinished(1000);
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
                      miner.isFile() && miner.isExecutable() && !miner.isSymLink());
    stop->setEnabled(running);
    threadCount->setEnabled(!running);
}

void PoolMiningDialog::readOutput()
{
    static const QRegularExpression colors(QStringLiteral("\\x1b\\[[0-9;]*[A-Za-z]"));
    QString text = QString::fromLocal8Bit(process->readAllStandardOutput().right(16000));
    text.remove(colors);
    if (!text.trimmed().isEmpty())
        output->appendPlainText(text.trimmed());
    if (!stopping && isMining() && text.contains(QRegularExpression(QStringLiteral("accepted:.*(?:yes!|yay!!!)"), QRegularExpression::CaseInsensitiveOption)))
        status->setText(tr("The pool accepted a share. Mining is running."));
}
