/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "session_consolekit.h"
#include "utils/common.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusUnixFileDescriptor>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if __has_include(<sys/sysmacros.h>)
#include <sys/sysmacros.h>
#endif

// Note that ConsoleKit's session api is not fully compatible with logind's session api.

struct DBusConsoleKitSeat
{
    QString id;
    QDBusObjectPath path;
};

QDBusArgument &operator<<(QDBusArgument &argument, const DBusConsoleKitSeat &seat)
{
    argument.beginStructure();
    argument << seat.id << seat.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusConsoleKitSeat &seat)
{
    argument.beginStructure();
    argument >> seat.id >> seat.path;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(DBusConsoleKitSeat)

namespace KWin
{

static const QString s_serviceName = QStringLiteral("org.freedesktop.ConsoleKit");
static const QString s_propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
static const QString s_sessionInterface = QStringLiteral("org.freedesktop.ConsoleKit.Session");
static const QString s_seatInterface = QStringLiteral("org.freedesktop.ConsoleKit.Seat");
static const QString s_managerInterface = QStringLiteral("org.freedesktop.ConsoleKit.Manager");
static const QString s_managerPath = QStringLiteral("/org/freedesktop/ConsoleKit/Manager");

static QString findProcessSessionPath()
{
    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_managerPath,
                                                          s_managerInterface,
                                                          QStringLiteral("GetSessionByPID"));
    message.setArguments({uint32_t(QCoreApplication::applicationPid())});

    const QDBusMessage reply = QDBusConnection::systemBus().call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        return QString();
    }

    return reply.arguments().constFirst().value<QDBusObjectPath>().path();
}

static bool takeControl(const QString &sessionPath)
{
    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, sessionPath,
                                                          s_sessionInterface,
                                                          QStringLiteral("TakeControl"));
    message.setArguments({false});

    const QDBusMessage reply = QDBusConnection::systemBus().call(message);

    return reply.type() != QDBusMessage::ErrorMessage;
}

static void releaseControl(const QString &sessionPath)
{
    const QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, sessionPath,
                                                                s_sessionInterface,
                                                                QStringLiteral("ReleaseControl"));

    QDBusConnection::systemBus().asyncCall(message);
}

static bool activate(const QString &sessionPath)
{
    const QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, sessionPath,
                                                                s_sessionInterface,
                                                                QStringLiteral("Activate"));

    const QDBusMessage reply = QDBusConnection::systemBus().call(message);

    return reply.type() != QDBusMessage::ErrorMessage;
}

ConsoleKitSession *ConsoleKitSession::create(QObject *parent)
{
    if (!QDBusConnection::systemBus().interface()->isServiceRegistered(s_serviceName)) {
        return nullptr;
    }

    const QString sessionPath = findProcessSessionPath();
    if (sessionPath.isEmpty()) {
        qCWarning(KWIN_CORE) << "Could not determine the active graphical session";
        return nullptr;
    }

    if (!activate(sessionPath)) {
        qCWarning(KWIN_CORE, "Failed to activate %s session. Maybe another compositor is running?",
                  qPrintable(sessionPath));
        return nullptr;
    }

    if (!takeControl(sessionPath)) {
        qCWarning(KWIN_CORE, "Failed to take control of %s session. Maybe another compositor is running?",
                  qPrintable(sessionPath));
        return nullptr;
    }

    ConsoleKitSession *session = new ConsoleKitSession(sessionPath, parent);
    if (session->initialize()) {
        return session;
    }

    delete session;
    return nullptr;
}

bool ConsoleKitSession::isActive() const
{
    return m_isActive;
}

ConsoleKitSession::Capabilities ConsoleKitSession::capabilities() const
{
    return Capability::SwitchTerminal;
}

QString ConsoleKitSession::seat() const
{
    return m_seatId;
}

uint ConsoleKitSession::terminal() const
{
    return m_terminal;
}

int ConsoleKitSession::openRestricted(const QString &fileName)
{
    struct stat st;
    if (stat(fileName.toUtf8(), &st) < 0) {
        return -1;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                          s_sessionInterface,
                                                          QStringLiteral("TakeDevice"));
    // major() and minor() macros return ints on FreeBSD instead of uints.
    message.setArguments({uint(major(st.st_rdev)), uint(minor(st.st_rdev))});

    const QDBusMessage reply = QDBusConnection::systemBus().call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCDebug(KWIN_CORE, "Failed to open %s device (%s)",
                qPrintable(fileName), qPrintable(reply.errorMessage()));
        return -1;
    }

    const QDBusUnixFileDescriptor descriptor = reply.arguments().constFirst().value<QDBusUnixFileDescriptor>();
    if (!descriptor.isValid()) {
        return -1;
    }

    return fcntl(descriptor.fileDescriptor(), F_DUPFD_CLOEXEC, 0);
}

void ConsoleKitSession::closeRestricted(int fileDescriptor)
{
    struct stat st;
    if (fstat(fileDescriptor, &st) < 0) {
        close(fileDescriptor);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                          s_sessionInterface,
                                                          QStringLiteral("ReleaseDevice"));
    // major() and minor() macros return ints on FreeBSD instead of uints.
    message.setArguments({uint(major(st.st_rdev)), uint(minor(st.st_rdev))});

    QDBusConnection::systemBus().asyncCall(message);

    close(fileDescriptor);
}

void ConsoleKitSession::switchTo(uint terminal)
{
    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, m_seatPath,
                                                          s_seatInterface,
                                                          QStringLiteral("SwitchTo"));
    message.setArguments({terminal});

    QDBusConnection::systemBus().asyncCall(message);
}

ConsoleKitSession::ConsoleKitSession(const QString &sessionPath, QObject *parent)
    : Session(parent)
    , m_sessionPath(sessionPath)
{
    qDBusRegisterMetaType<DBusConsoleKitSeat>();
}

ConsoleKitSession::~ConsoleKitSession()
{
    releaseControl(m_sessionPath);
}

bool ConsoleKitSession::initialize()
{
    QDBusMessage activeMessage = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                                s_propertiesInterface,
                                                                QStringLiteral("Get"));
    activeMessage.setArguments({s_sessionInterface, QStringLiteral("active")});

    QDBusMessage seatMessage = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                              s_propertiesInterface,
                                                              QStringLiteral("Get"));
    seatMessage.setArguments({s_sessionInterface, QStringLiteral("Seat")});

    QDBusMessage terminalMessage = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                                  s_propertiesInterface,
                                                                  QStringLiteral("Get"));
    terminalMessage.setArguments({s_sessionInterface, QStringLiteral("VTNr")});

    QDBusPendingReply<QVariant> activeReply =
        QDBusConnection::systemBus().asyncCall(activeMessage);
    QDBusPendingReply<QVariant> terminalReply =
        QDBusConnection::systemBus().asyncCall(terminalMessage);
    QDBusPendingReply<QVariant> seatReply =
        QDBusConnection::systemBus().asyncCall(seatMessage);

    // We must wait until all replies have been received because the drm backend needs a
    // valid seat name to properly select gpu devices, this also simplifies startup code.
    activeReply.waitForFinished();
    terminalReply.waitForFinished();
    seatReply.waitForFinished();

    if (activeReply.isError()) {
        qCWarning(KWIN_CORE) << "Failed to query active session property:" << activeReply.error();
        return false;
    }
    if (terminalReply.isError()) {
        qCWarning(KWIN_CORE) << "Failed to query VTNr session property:" << terminalReply.error();
        return false;
    }
    if (seatReply.isError()) {
        qCWarning(KWIN_CORE) << "Failed to query Seat session property:" << seatReply.error();
        return false;
    }

    m_isActive = activeReply.value().toBool();
    m_terminal = terminalReply.value().toUInt();

    const DBusConsoleKitSeat seat = qdbus_cast<DBusConsoleKitSeat>(seatReply.value().value<QDBusArgument>());
    m_seatId = seat.id;
    m_seatPath = seat.path.path();

    QDBusConnection::systemBus().connect(s_serviceName, s_managerPath, s_managerInterface,
                                         QStringLiteral("PrepareForSleep"),
                                         this,
                                         SLOT(handlePrepareForSleep(bool)));

    QDBusConnection::systemBus().connect(s_serviceName, m_sessionPath, s_sessionInterface,
                                         QStringLiteral("PauseDevice"),
                                         this,
                                         SLOT(handlePauseDevice(uint, uint, QString)));

    QDBusConnection::systemBus().connect(s_serviceName, m_sessionPath, s_sessionInterface,
                                         QStringLiteral("ResumeDevice"),
                                         this,
                                         SLOT(handleResumeDevice(uint, uint, QDBusUnixFileDescriptor)));

    QDBusConnection::systemBus().connect(s_serviceName, m_sessionPath, s_propertiesInterface,
                                         QStringLiteral("PropertiesChanged"),
                                         this,
                                         SLOT(handlePropertiesChanged(QString, QVariantMap)));

    return true;
}

void ConsoleKitSession::updateActive(bool active)
{
    if (m_isActive != active) {
        m_isActive = active;
        Q_EMIT activeChanged(active);
    }
}

void ConsoleKitSession::handlePauseDevice(uint major, uint minor, const QString &type)
{
    Q_EMIT devicePaused(makedev(major, minor));

    if (type == QLatin1String("pause")) {
        QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, m_sessionPath,
                                                              s_sessionInterface,
                                                              QStringLiteral("PauseDeviceComplete"));
        message.setArguments({major, minor});

        QDBusConnection::systemBus().asyncCall(message);
    }
}

void ConsoleKitSession::handleResumeDevice(uint major, uint minor, QDBusUnixFileDescriptor fileDescriptor)
{
    // We don't care about the file descriptor as the libinput backend will re-open input devices
    // and the drm file descriptors remain valid after pausing gpus.
    Q_UNUSED(fileDescriptor)

    Q_EMIT deviceResumed(makedev(major, minor));
}

void ConsoleKitSession::handlePropertiesChanged(const QString &interfaceName, const QVariantMap &properties)
{
    if (interfaceName == s_sessionInterface) {
        const QVariant active = properties.value(QStringLiteral("active"));
        if (active.isValid()) {
            updateActive(active.toBool());
        }
    }
}

void ConsoleKitSession::handlePrepareForSleep(bool sleep)
{
    if (!sleep) {
        Q_EMIT awoke();
    }
}

} // namespace KWin
