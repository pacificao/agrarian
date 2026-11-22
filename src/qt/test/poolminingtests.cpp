// Distributed under the MIT software license, see the accompanying file COPYING.

#include "poolminingdialog.h"

#include "chainparams.h"
#include "util.h"

#include <QPlainTextEdit>
#include <QCheckBox>
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
        QCOMPARE(PoolMiningDialog::firstPartyArguments("wallet-owned-address", false, {}, 3),
                 (QStringList{"--pool-cpu", "wallet-owned-address", "3"}));
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
        QCOMPARE(endpoint(CBaseChainParams::MAIN, "12345"), QString("stratum+tls://alltheacronyms.com:3033"));
    }

    void nothingStartsWithoutWalletAndExplicitClick()
    {
        PoolMiningDialog page;
        page.show();
        QVERIFY(!page.isMining());
        for (const auto* button : page.findChildren<QPushButton*>())
            QVERIFY(!button->isEnabled());
        page.close();
        QVERIFY(!page.isMining());
    }

    void miningLogSetsForegroundAndBackground()
    {
        PoolMiningDialog page;
        page.show();
        auto* log = page.findChild<QPlainTextEdit*>(QStringLiteral("poolMiningLog"));
        QVERIFY(log);
        QVERIFY(log->isReadOnly());
        log->ensurePolished();
        QCOMPARE(log->palette().color(QPalette::Base), QColor(QStringLiteral("#111827")));
        QCOMPARE(log->palette().color(QPalette::Text), QColor(QStringLiteral("#f3f4f6")));
    }

    void previewAllowsIndependentCpuAndGpuSelections()
    {
        SelectParams(CBaseChainParams::REGTEST);
        mapArgs["-regtestpoolminingport"] = "16423";
        mapArgs["-regtestfirstpartypoolminer"] = "1";
        bool independent = false;
        QStringList cpuArgs, gpuArgs;
        {
            PoolMiningDialog page;
            page.show();
            auto* cpu = page.findChild<QCheckBox*>(QStringLiteral("mineWithCpu"));
            auto* gpu = page.findChild<QCheckBox*>(QStringLiteral("mineWithGpu"));
            if (cpu && gpu && cpu->isVisible() && gpu->isVisible() && cpu->isChecked() && !gpu->isChecked()) {
                gpu->setChecked(true);
                independent = cpu->isChecked() && gpu->isChecked() && !page.isMining();
            }
            cpuArgs = PoolMiningDialog::firstPartyArguments("payout", false, {});
            gpuArgs = PoolMiningDialog::firstPartyArguments("payout", true, "/pinned/quark-screen.cl");
        }
        SelectParams(CBaseChainParams::MAIN);
        const bool mainnet = PoolMiningDialog::firstPartyPreview();
        const auto blockedArgs = PoolMiningDialog::firstPartyArguments("payout", true, "/pinned/quark-screen.cl");
        mapArgs.erase("-regtestpoolminingport");
        mapArgs.erase("-regtestfirstpartypoolminer");
        QVERIFY(independent);
        QVERIFY(!mainnet);
        QCOMPARE(blockedArgs, (QStringList{"--pool-gpu", "payout", "/pinned/quark-screen.cl"}));
        QCOMPARE(cpuArgs, (QStringList{"--regtest-loopback", "16423", "payout", "1"}));
        QCOMPARE(gpuArgs, (QStringList{"--regtest-opencl", "16423", "payout", "/pinned/quark-screen.cl"}));
    }
};

int runPoolMiningTests()
{
    PoolMiningTests tests;
    return QTest::qExec(&tests);
}

#include "poolminingtests.moc"
