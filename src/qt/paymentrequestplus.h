// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PAYMENTREQUESTPLUS_H
#define BITCOIN_QT_PAYMENTREQUESTPLUS_H

#include "amount.h"
#include "script/script.h"

#include <string>
#include <utility>

#include <openssl/x509.h>

#include <QByteArray>
#include <QList>
#include <QString>

// BIP70 payment requests were removed from the wallet to eliminate the
// obsolete protobuf dependency. The class stays as a compatibility shim for
// serialized wallet/order-form code paths that check for payment requests.
class PaymentRequestPlus
{
public:
    PaymentRequestPlus() {}

    bool parse(const QByteArray& data);
    bool SerializeToString(std::string* output) const;
    bool IsInitialized() const;
    QString getPKIType() const;
    bool getMerchant(X509_STORE* certStore, QString& merchant) const;
    QList<std::pair<CScript, CAmount> > getPayTo() const;
};

#endif // BITCOIN_QT_PAYMENTREQUESTPLUS_H
