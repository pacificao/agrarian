// Distributed under the MIT software license, see the accompanying file COPYING.

#include "poolminingdialog.h"

#include "chainparams.h"
#include "util.h"

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

    void testPoolCanOnlyUseLoopbackOnRegtest()
    {
        auto endpoint = [](CBaseChainParams::Network network, const char* port) {
            SelectParams(network);
            mapArgs["-regtestpoolminingport"] = port;
            const auto url = PoolMiningDialog::poolEndpoint();
            mapArgs.erase("-regtestpoolminingport");
            SelectParams(CBaseChainParams::MAIN);
            return url;
        };
        QCOMPARE(endpoint(CBaseChainParams::REGTEST, "12345"), QString("stratum+tcp://127.0.0.1:12345"));
        QVERIFY(endpoint(CBaseChainParams::REGTEST, "65536").isEmpty());
        QVERIFY(endpoint(CBaseChainParams::REGTEST, "https://example.org").isEmpty());
        QVERIFY(endpoint(CBaseChainParams::TESTNET, "12345").isEmpty());
        QCOMPARE(endpoint(CBaseChainParams::MAIN, "12345"), QString("stratum+tcp://alltheacronyms.com:3032"));
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
