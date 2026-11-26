// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_STAKINGACTIVITY_H
#define AGRARIAN_QT_STAKINGACTIVITY_H

#include <QCoreApplication>
#include <QString>
#include <string>

namespace stakingactivity {
inline QString Type()
{
    return QCoreApplication::translate("TransactionTableModel", "Staking reward");
}

// Stake activity should never pass an address-book nickname here. A payout
// nickname identifies a wallet address, not the source of the stake reward.
inline QString ReceivingAddress(const std::string& address)
{
    return QCoreApplication::translate("TransactionTableModel", "Receiving address: %1")
        .arg(QString::fromStdString(address));
}
} // namespace stakingactivity

#endif
