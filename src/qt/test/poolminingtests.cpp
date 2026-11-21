// Distributed under the MIT software license, see the accompanying file COPYING.

#include "poolminingdialog.h"

#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTest>

class PoolMiningTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fixedPoolAndNoMinerApi()
    {
        const auto args = PoolMiningDialog::minerArguments("wallet-owned-address", 3);
        QCOMPARE(args, (QStringList{"-a", "quark", "-o", "stratum+tcp://alltheacronyms.com:3032",
                                    "-u", "wallet-owned-address.agrarian-qt", "-p", "x",
                                    "-t", "3", "-b", "0"}));
    }

    void nothingStartsWithoutWalletAndExplicitClick()
    {
        QSettings settings;
        settings.remove("poolMining/cpuThreads");
        PoolMiningDialog page;
        page.show();
        const auto* threads = page.findChild<QSpinBox*>();
        QVERIFY(threads);
        QCOMPARE(threads->value(), 1);
        QVERIFY(!page.isMining());
        for (const auto* button : page.findChildren<QPushButton*>())
            QVERIFY(!button->isEnabled());
        page.close();
        QVERIFY(!page.isMining());
    }
};

int runPoolMiningTests()
{
    PoolMiningTests tests;
    return QTest::qExec(&tests);
}

#include "poolminingtests.moc"
