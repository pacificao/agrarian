// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_QT_POOLFRAMES_H
#define AGRARIAN_QT_POOLFRAMES_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace poolframes {
constexpr qsizetype kMaxFrameBytes = 16 * 1024;

enum class Kind { Invalid, Ignore, Reply, Work, Difficulty, ExtraNonce };

// Classify exactly one complete frame. The connection must bound its input
// buffer to kMaxFrameBytes even when the peer never sends a newline.
// There is intentionally no handler for server-initiated redirects or RPCs.
inline Kind Classify(const QByteArray& line)
{
    if (line.isEmpty() || line.size() > kMaxFrameBytes || line.contains('\n') || line.contains('\0'))
        return Kind::Invalid;

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return Kind::Invalid;
    const auto object = doc.object();
    const auto method = object.value(QStringLiteral("method"));
    if (method.isUndefined()) {
        const auto id = object.value(QStringLiteral("id"));
        if (id.toInt(-1) <= 0 || (!object.contains(QStringLiteral("result")) &&
                                  !object.contains(QStringLiteral("error"))))
            return Kind::Invalid;
        return Kind::Reply; // the session must correlate this ID to a pending request
    }
    if (!method.isString() || object.contains(QStringLiteral("result")) ||
        object.contains(QStringLiteral("error")))
        return Kind::Invalid;
    const auto name = method.toString();
    if (name == QStringLiteral("mining.notify") || name == QStringLiteral("mining.set_difficulty") ||
        name == QStringLiteral("mining.set_extranonce")) {
        if (!object.value(QStringLiteral("params")).isArray()) return Kind::Invalid;
        if (name == QStringLiteral("mining.notify")) return Kind::Work;
        if (name == QStringLiteral("mining.set_difficulty")) return Kind::Difficulty;
        return Kind::ExtraNonce;
    }
    return Kind::Ignore;
}
} // namespace poolframes
#endif
