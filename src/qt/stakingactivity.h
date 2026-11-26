// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_STAKINGACTIVITY_H
#define AGRARIAN_QT_STAKINGACTIVITY_H

#include <QCoreApplication>
#include <QString>

namespace stakingactivity {
inline QString Type()
{
    return QCoreApplication::translate("TransactionTableModel", "Staking reward");
}

inline QString ReceivingAddress(const QString& addressOrLabel)
{
    return QCoreApplication::translate("TransactionTableModel", "Receiving address: %1").arg(addressOrLabel);
}
} // namespace stakingactivity

#endif
