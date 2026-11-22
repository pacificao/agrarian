// Distributed under the MIT software license, see the accompanying file COPYING.

#ifndef AGRARIAN_QT_POOLMININGDIALOG_H
#define AGRARIAN_QT_POOLMININGDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QStringList>

class QLabel;
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
    static QStringList minerArguments(const QString& payout, int threads);

protected:
    void done(int result) override;

private:
    void startMining();
    void stopMining();
    void updateControls();
    void readOutput();
    QString minerPath() const;

    QPointer<WalletModel> wallet;
    QProcess* process;
    QSpinBox* threadCount;
    QLabel* status;
    QLabel* payoutAddress;
    QPushButton* start;
    QPushButton* stop;
    QPlainTextEdit* output;
    bool stopping{false};
};

#endif
