// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentrequestplus.h"

#include <QDebug>

bool PaymentRequestPlus::parse(const QByteArray& data)
{
    Q_UNUSED(data);
    qWarning() << "PaymentRequestPlus::parse : BIP70 payment requests are no longer supported";
    return false;
}

bool PaymentRequestPlus::SerializeToString(std::string* output) const
{
    if (output)
        output->clear();
    return false;
}

bool PaymentRequestPlus::IsInitialized() const
{
    return false;
}

QString PaymentRequestPlus::getPKIType() const
{
    return QString("none");
}

bool PaymentRequestPlus::getMerchant(X509_STORE* certStore, QString& merchant) const
{
    Q_UNUSED(certStore);
    merchant.clear();
    return false;
}

QList<std::pair<CScript, CAmount> > PaymentRequestPlus::getPayTo() const
{
    return QList<std::pair<CScript, CAmount> >();
}
