// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "stakingactivity.h"

#include <QFrame>
#include <QLabel>
#include <QTest>

class OverviewPageTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stakeNicknameIsNotThePaymentSource()
    {
        QCOMPARE(stakingactivity::Type(), QStringLiteral("Staking reward"));
        QCOMPARE(stakingactivity::ReceivingAddress(QStringLiteral("Pool mining payouts")),
                 QStringLiteral("Receiving address: Pool mining payouts"));
        QCOMPARE(stakingactivity::ReceivingAddress(QStringLiteral("AGRaddress")),
                 QStringLiteral("Receiving address: AGRaddress"));
    }

    void legacyBalanceVisibility()
    {
        OverviewPage page;
        auto* regular = page.findChild<QFrame*>("frame_Balances");
        auto* legacy = page.findChild<QFrame*>("frame_ZerocoinBalances");
        auto* syncWarning = page.findChild<QLabel*>("labelWalletStatus");
        auto* notice = page.findChild<QLabel*>("legacyZagrNotice");
        QVERIFY(regular && legacy && syncWarning && notice);
        page.show();

        QVERIFY(!page.hasLegacyZagrBalance());
        QVERIFY(regular->isVisible());
        QVERIFY(syncWarning->isVisible());
        QVERIFY(!legacy->isVisible());

        const CAmount one = COIN;
        page.setBalance(one, 0, 0, 0, 0, 0, 0, 0, 0);
        QVERIFY(!legacy->isVisible());

        page.setBalance(one, 0, 0, one, 0, 0, 0, 0, 0);
        QVERIFY(page.hasLegacyZagrBalance());
        QVERIFY(legacy->isVisible());
        QVERIFY(notice->isVisible());
        QVERIFY(regular->isVisible());

        page.setBalance(one, 0, 0, one, one, 0, 0, 0, 0);
        QVERIFY(legacy->isVisible());
        page.setBalance(one, 0, 0, one, 0, one, 0, 0, 0);
        QVERIFY(legacy->isVisible());

        page.setBalance(one, 0, 0, 0, 0, 0, 0, 0, 0);
        QVERIFY(!page.hasLegacyZagrBalance());
        QVERIFY(!legacy->isVisible());
        QVERIFY(regular->isVisible());
        QVERIFY(syncWarning->isVisible());
    }
};

int runOverviewPageTests()
{
    OverviewPageTests tests;
    return QTest::qExec(&tests);
}

#include "overviewpagetests.moc"
