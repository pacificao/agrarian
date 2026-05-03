// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentserver.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include "base58.h"
#include "chainparams.h"
#include "guiinterface.h"
#include "util.h"
#include "wallet/wallet.h"

#include <cstdlib>

#include <openssl/x509_vfy.h>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileOpenEvent>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTextDocument>
#include <QUrlQuery>

using namespace boost;
using namespace std;

const int BITCOIN_IPC_CONNECT_TIMEOUT = 1000; // milliseconds
const QString BITCOIN_IPC_PREFIX("agrarian:");
// BIP70 max payment request size in bytes (kept for compatibility tests).
const qint64 BIP70_MAX_PAYMENTREQUEST_SIZE = 50000;

struct X509StoreDeleter {
      void operator()(X509_STORE* b) {
          X509_STORE_free(b);
      }
};

struct X509Deleter {
      void operator()(X509* b) { X509_free(b); }
};

namespace // Anon namespace
{

    std::unique_ptr<X509_STORE, X509StoreDeleter> certStore;
}

//
// Create a name that is unique for:
//  testnet / non-testnet
//  data directory
//
static QString ipcServerName()
{
    QString name("AgrarianQt");

    // Append a simple hash of the datadir
    // Note that GetDataDir(true) returns a different path
    // for -testnet versus main net
    QString ddir(QString::fromStdString(GetDataDir(true).string()));
    name.append(QString::number(qHash(ddir)));

    return name;
}

//
// We store payment URIs and requests received before
// the main GUI window is up and ready to ask the user
// to send payment.

static QList<QString> savedPaymentRequests;

static void ReportInvalidCertificate(const QSslCertificate& cert)
{
    qDebug() << QString("%1: Payment server found an invalid certificate: ").arg(__func__) << cert.serialNumber() << cert.subjectInfo(QSslCertificate::CommonName) << cert.subjectInfo(QSslCertificate::DistinguishedNameQualifier) << cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
}

//
// Load OpenSSL's list of root certificate authorities
//
void PaymentServer::LoadRootCAs(X509_STORE* _store)
{
    // Unit tests mostly use this, to pass in fake root CAs:
    if (_store) {
        certStore.reset(_store);
        return;
    }

    // Normal execution, use either -rootcertificates or system certs:
    certStore.reset(X509_STORE_new());

    // Note: use "-system-" default here so that users can pass -rootcertificates=""
    // and get 'I don't like X.509 certificates, don't trust anybody' behavior:
    QString certFile = QString::fromStdString(GetArg("-rootcertificates", "-system-"));

    if (certFile.isEmpty())
        return; // Empty store

    QList<QSslCertificate> certList;

    if (certFile != "-system-") {
        certList = QSslCertificate::fromPath(certFile);
        // Use those certificates when fetching payment requests, too:
        QSslConfiguration sslConfiguration = QSslConfiguration::defaultConfiguration();
        sslConfiguration.setCaCertificates(certList);
        QSslConfiguration::setDefaultConfiguration(sslConfiguration);
    } else
        certList = QSslConfiguration::systemCaCertificates();

    int nRootCerts = 0;
    const QDateTime currentTime = QDateTime::currentDateTime();
    foreach (const QSslCertificate& cert, certList) {
        if (currentTime < cert.effectiveDate() || currentTime > cert.expiryDate()) {
            ReportInvalidCertificate(cert);
            continue;
        }

        // Blacklisted certificate
        if (cert.isBlacklisted()) {
            ReportInvalidCertificate(cert);
            continue;
        }
        QByteArray certData = cert.toDer();
        const unsigned char* data = (const unsigned char*)certData.data();

        std::unique_ptr<X509, X509Deleter> x509(d2i_X509(0, &data, certData.size()));
        if (x509 && X509_STORE_add_cert(certStore.get(), x509.get())) {
            // Note: X509_STORE increases the reference count to the X509 object,
            // we still have to release our reference to it.
            ++nRootCerts;
        } else {
            ReportInvalidCertificate(cert);
            continue;
        }
    }
    qWarning() << "PaymentServer::LoadRootCAs : Loaded " << nRootCerts << " root certificates";

    // Project for another day:
    // Fetch certificate revocation lists, and add them to certStore.
    // Issues to consider:
    //   performance (start a thread to fetch in background?)
    //   privacy (fetch through tor/proxy so IP address isn't revealed)
    //   would it be easier to just use a compiled-in blacklist?
    //    or use Qt's blacklist?
    //   "certificate stapling" with server-side caching is more efficient
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
// Warning: ipcSendCommandLine() is called early in init,
// so don't use "emit message()", but "QMessageBox::"!
//
void PaymentServer::ipcParseCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        QString arg(argv[i]);
        if (arg.startsWith("-"))
            continue;

        // If the agrarian: URI contains a payment request, we are not able to detect the
        // network as that would require fetching and parsing the payment request.
        // That means clicking such an URI which contains a testnet payment request
        // will start a mainnet instance and throw a "wrong network" error.
        if (arg.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // agrarian: URI
        {
            savedPaymentRequests.append(arg);

            SendCoinsRecipient r;
            if (GUIUtil::parseBitcoinURI(arg, &r) && !r.address.isEmpty()) {
                CBitcoinAddress address(r.address.toStdString());

                if (address.IsValid(Params(CBaseChainParams::MAIN))) {
                    SelectParams(CBaseChainParams::MAIN);
                } else if (address.IsValid(Params(CBaseChainParams::TESTNET))) {
                    SelectParams(CBaseChainParams::TESTNET);
                }
            }
        } else if (QFile::exists(arg)) // Filename
        {
            qWarning() << "PaymentServer::ipcSendCommandLine : BIP70 payment request files are no longer supported: " << arg;
        } else {
            // Printing to debug.log is about the best we can do here, the
            // GUI hasn't started yet so we can't pop up a message box.
            qWarning() << "PaymentServer::ipcSendCommandLine : Payment request file does not exist: " << arg;
        }
    }
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
bool PaymentServer::ipcSendCommandLine()
{
    bool fResult = false;
    foreach (const QString& r, savedPaymentRequests) {
        QLocalSocket* socket = new QLocalSocket();
        socket->connectToServer(ipcServerName(), QIODevice::WriteOnly);
        if (!socket->waitForConnected(BITCOIN_IPC_CONNECT_TIMEOUT)) {
            delete socket;
            socket = NULL;
            return false;
        }

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << r;
        out.device()->seek(0);

        socket->write(block);
        socket->flush();
        socket->waitForBytesWritten(BITCOIN_IPC_CONNECT_TIMEOUT);
        socket->disconnectFromServer();

        delete socket;
        socket = NULL;
        fResult = true;
    }

    return fResult;
}

PaymentServer::PaymentServer(QObject* parent, bool startLocalServer) : QObject(parent),
                                                                       saveURIs(true),
                                                                       uriServer(0),
                                                                       netManager(0),
                                                                       optionsModel(0)
{
    // Install global event filter to catch QFileOpenEvents
    // on Mac: sent when you click agrarian: links
    // other OSes: helpful when dealing with payment request files (in the future)
    if (parent)
        parent->installEventFilter(this);

    QString name = ipcServerName();

    // Clean up old socket leftover from a crash:
    QLocalServer::removeServer(name);

    if (startLocalServer) {
        uriServer = new QLocalServer(this);

        if (!uriServer->listen(name)) {
            // constructor is called early in init, so don't use "emit message()" here
            QMessageBox::critical(0, tr("Payment request error"),
                tr("Cannot start agrarian: click-to-pay handler"));
        } else {
            connect(uriServer, SIGNAL(newConnection()), this, SLOT(handleURIConnection()));
        }
    }
}

PaymentServer::~PaymentServer()
{
}

//
// OSX-specific way of handling agrarian: URIs and
// PaymentRequest mime types
//
bool PaymentServer::eventFilter(QObject* object, QEvent* event)
{
    // clicking on agrarian: URIs creates FileOpen events on the Mac
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->file().isEmpty())
            handleURIOrFile(fileEvent->file());
        else if (!fileEvent->url().isEmpty())
            handleURIOrFile(fileEvent->url().toString());

        return true;
    }

    return QObject::eventFilter(object, event);
}

void PaymentServer::initNetManager()
{
    if (!optionsModel)
        return;
    if (netManager != NULL)
        delete netManager;

    // netManager is used to fetch paymentrequests given in agrarian: URIs
    netManager = new QNetworkAccessManager(this);

    QNetworkProxy proxy;

    // Query active SOCKS5 proxy
    if (optionsModel->getProxySettings(proxy)) {
        netManager->setProxy(proxy);

        qDebug() << "PaymentServer::initNetManager : Using SOCKS5 proxy" << proxy.hostName() << ":" << proxy.port();
    } else
        qDebug() << "PaymentServer::initNetManager : No active proxy server found.";

    connect(netManager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(netRequestFinished(QNetworkReply*)));
    connect(netManager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
        this, SLOT(reportSslErrors(QNetworkReply*, const QList<QSslError>&)));
}

void PaymentServer::uiReady()
{
    initNetManager();

    saveURIs = false;
    foreach (const QString& s, savedPaymentRequests) {
        handleURIOrFile(s);
    }
    savedPaymentRequests.clear();
}

void PaymentServer::handleURIOrFile(const QString& s)
{
    if (saveURIs) {
        savedPaymentRequests.append(s);
        return;
    }

    if (s.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // agrarian: URI
    {
        QUrlQuery uri((QUrl(s)));
        if (uri.hasQueryItem("r")) // payment request URI
        {
            qWarning() << "PaymentServer::handleURIOrFile : BIP70 payment request URLs are no longer supported.";
            emit message(tr("URI handling"),
                tr("BIP70 payment request URLs are no longer supported. Use a standard agrarian: URI with address and amount."),
                CClientUIInterface::ICON_WARNING);
            return;
        } else // normal URI
        {
            SendCoinsRecipient recipient;
            if (GUIUtil::parseBitcoinURI(s, &recipient)) {
                CBitcoinAddress address(recipient.address.toStdString());
                if (!address.IsValid()) {
                    emit message(tr("URI handling"), tr("Invalid payment address %1").arg(recipient.address),
                        CClientUIInterface::MSG_ERROR);
                } else
                    emit receivedPaymentRequest(recipient);
            } else
                emit message(tr("URI handling"),
                    tr("URI cannot be parsed! This can be caused by an invalid Agrarian address or malformed URI parameters."),
                    CClientUIInterface::ICON_WARNING);

            return;
        }
    }

    if (QFile::exists(s)) // payment request file
    {
        qWarning() << "PaymentServer::handleURIOrFile : BIP70 payment request files are no longer supported: " << s;
        emit message(tr("Payment request file handling"),
            tr("BIP70 payment request files are no longer supported. Use a standard agrarian: URI instead."),
            CClientUIInterface::ICON_WARNING);
        return;
    }
}

void PaymentServer::handleURIConnection()
{
    QLocalSocket* clientConnection = uriServer->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();

    connect(clientConnection, SIGNAL(disconnected()),
        clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_4_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString msg;
    in >> msg;

    handleURIOrFile(msg);
}

//
// Warning: readPaymentRequestFromFile() is used in ipcSendCommandLine()
// so don't use "emit message()", but "QMessageBox::"!
//
bool PaymentServer::readPaymentRequestFromFile(const QString& filename, PaymentRequestPlus& request)
{
    Q_UNUSED(filename);
    Q_UNUSED(request);

    qWarning() << "PaymentServer::readPaymentRequestFromFile : BIP70 payment request files are no longer supported.";
    return false;
}

bool PaymentServer::processPaymentRequest(PaymentRequestPlus& request, SendCoinsRecipient& recipient)
{
    Q_UNUSED(request);
    Q_UNUSED(recipient);

    emit message(tr("Payment request rejected"),
        tr("BIP70 payment requests are no longer supported."),
        CClientUIInterface::ICON_WARNING);
    return false;
}

void PaymentServer::fetchRequest(const QUrl& url)
{
    Q_UNUSED(url);

    emit message(tr("Payment request rejected"),
        tr("BIP70 payment request URLs are no longer supported."),
        CClientUIInterface::ICON_WARNING);
}

void PaymentServer::fetchPaymentACK(CWallet* wallet, SendCoinsRecipient recipient, QByteArray transaction)
{
    Q_UNUSED(wallet);
    Q_UNUSED(recipient);
    Q_UNUSED(transaction);
}

void PaymentServer::netRequestFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    qWarning() << "PaymentServer::netRequestFinished : Ignoring unexpected network reply after BIP70 removal.";
}

void PaymentServer::reportSslErrors(QNetworkReply* reply, const QList<QSslError>& errs)
{
    Q_UNUSED(reply);

    QString errString;
    foreach (const QSslError& err, errs) {
        qWarning() << "PaymentServer::reportSslErrors : " << err;
        errString += err.errorString() + "\n";
    }
    emit message(tr("Network request error"), errString, CClientUIInterface::MSG_ERROR);
}

void PaymentServer::setOptionsModel(OptionsModel* optionsModel)
{
    this->optionsModel = optionsModel;
}

void PaymentServer::handlePaymentACK(const QString& paymentACKMsg)
{
    // currently we don't futher process or store the paymentACK message
    emit message(tr("Payment acknowledged"), paymentACKMsg, CClientUIInterface::ICON_INFORMATION | CClientUIInterface::MODAL);
}

X509_STORE* PaymentServer::getCertStore()
{
    return certStore.get();
}
