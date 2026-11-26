// Distributed under the MIT software license, see the accompanying file COPYING.

#ifndef AGRARIAN_QT_POOLMININGDIALOG_H
#define AGRARIAN_QT_POOLMININGDIALOG_H

#include <QDialog>
#include <QByteArray>
#include <QPointer>
#include <QStringList>

class QLabel;
class QCheckBox;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QSpinBox;
class WalletModel;

class PoolMiningDialog : public QDialog
{
public:
    explicit PoolMiningDialog(QWidget* parent = nullptr);
    ~PoolMiningDialog() override;

    void setWallet(WalletModel* model);
    bool isMining() const;
    static QString poolEndpoint();
    static bool firstPartyPreview();
    static QStringList firstPartyArguments(const QString& payout, bool gpu, const QString& kernel, int threads = 1);

protected:
    void done(int result) override;

private:
    void startMining();
    void stopMining();
    void updateControls();
    void attachWorker(QProcess* worker, bool gpu);
    void readOutput(QProcess* worker, bool gpu, bool flush = false);
    void appendWorkerLog(bool gpu, const QString& line);
    QString minerPath() const;
    QString kernelPath() const;

    QPointer<WalletModel> wallet;
    QProcess* process;
    QProcess* gpuProcess;
    QCheckBox* cpuChoice;
    QCheckBox* gpuChoice;
    QSpinBox* threadCount;
    QSpinBox* cpuLimit;
    QSpinBox* gpuLimit;
    QCheckBox* unmonitored;
    QCheckBox* windowsProxy;
    QCheckBox* windowsProxyOverride;
    QLabel* cpuThermal;
    QLabel* gpuThermal;
    QLabel* status;
    QLabel* payoutAddress;
    QPushButton* start;
    QPushButton* stop;
    QPlainTextEdit* output;
    QByteArray cpuPending, gpuPending;
    bool stopping{false};
};

#endif
