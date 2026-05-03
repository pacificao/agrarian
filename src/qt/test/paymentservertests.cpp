// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentservertests.h"

#include "optionsmodel.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <QFileOpenEvent>
#include <QTemporaryFile>

X509 *parse_b64der_cert(const char* cert_data)
{
    std::vector<unsigned char> data = DecodeBase64(cert_data);
    assert(data.size() > 0);
    const unsigned char* dptr = &data[0];
    X509 *cert = d2i_X509(NULL, &dptr, data.size());
    assert(cert);
    return cert;
}

//
// Test payment request handling
//

static SendCoinsRecipient handleRequest(PaymentServer* server, std::vector<unsigned char>& data)
{
    RecipientCatcher sigCatcher;
    QObject::connect(server, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)),
        &sigCatcher, SLOT(getRecipient(SendCoinsRecipient)));

    // Write data to a temp file:
    QTemporaryFile f;
    f.open();
    f.write((const char*)&data[0], data.size());
    f.close();

    // Create a QObject, install event filter from PaymentServer
    // and send a file open event to the object
    QObject object;
    object.installEventFilter(server);
    QFileOpenEvent event(f.fileName());
    // If sending the event fails, this will cause sigCatcher to be empty,
    // which will lead to a test failure anyway.
    QCoreApplication::sendEvent(&object, &event);

    QObject::disconnect(server, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)),
        &sigCatcher, SLOT(getRecipient(SendCoinsRecipient)));

    // Return results from sigCatcher
    return sigCatcher.recipient;
}

void PaymentServerTests::paymentServerTests()
{
    SelectParams(CBaseChainParams::MAIN);
    OptionsModel optionsModel;
    PaymentServer* server = new PaymentServer(NULL, false);
    server->setOptionsModel(&optionsModel);
    server->uiReady();

    // BIP70 payment requests are intentionally unsupported in Agrarian 2.0.
    // Standard agrarian: URI handling remains covered by uritests.
    std::vector<unsigned char> data(128, 0);
    SendCoinsRecipient r = handleRequest(server, data);
    QCOMPARE(r.paymentRequest.IsInitialized(), false);

    unsigned long lDoSProtectionTrigger = (unsigned long) BIP70_MAX_PAYMENTREQUEST_SIZE + 1;
    std::string randData(lDoSProtectionTrigger, '\0');

    unsigned char* buff = reinterpret_cast<unsigned char *>(&randData[0]);

    // Just get some random data big enough to trigger BIP70 DoS protection
    GetRandBytes(buff, sizeof(buff));
    // Write data to a temp file:
    QTemporaryFile tempFile;
    tempFile.open();
    tempFile.write((const char*)buff, sizeof(buff));
    tempFile.close();
    // Trigger BIP70 DoS protection
    QCOMPARE(PaymentServer::readPaymentRequestFromFile(tempFile.fileName(), r.paymentRequest), false);

    delete server;
}

void RecipientCatcher::getRecipient(SendCoinsRecipient r)
{
    recipient = r;
}
