#include "PlayerEntity.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QtGlobal>

PlayerEntity::PlayerEntity(const QRectF& bounds)
    : m_worldBounds(bounds) {
    QString assetPath = QStringLiteral("assets/player.png");
    QString appDir = QCoreApplication::applicationDirPath();
    QString resolvedPath = QDir(appDir).filePath(assetPath);

    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../") + assetPath);
    }
    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../../") + assetPath);
    }

    QPixmap pix(resolvedPath);
    if (!pix.isNull()) {
        setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        setPixmap(QPixmap(80, 80));
    }

    double startX = bounds.center().x() - boundingRect().width() / 2.0;
    double startY = bounds.center().y() - boundingRect().height() / 2.0;
    setPos(startX, startY);
}

void PlayerEntity::setMovementInput(bool up, bool down, bool left, bool right) {
    m_up = up;
    m_down = down;
    m_left = left;
    m_right = right;
}

void PlayerEntity::update(double deltaTimeSeconds) {
    if (m_trapped) {
        m_trapRemaining -= deltaTimeSeconds;
        if (m_trapRemaining <= 0.0) {
            m_trapped = false;
            m_trapRemaining = 0.0;
        }
        return;
    }

    qreal axisX = 0.0;
    qreal axisY = 0.0;

    if (m_left) {
        axisX -= 1.0;
    }
    if (m_right) {
        axisX += 1.0;
    }
    if (m_up) {
        axisY -= 1.0;
    }
    if (m_down) {
        axisY += 1.0;
    }

    const QPointF delta(axisX * m_moveSpeed * deltaTimeSeconds,
                        axisY * m_moveSpeed * deltaTimeSeconds);

    QPointF nextPos = pos() + delta;

    const qreal minX = m_worldBounds.left();
    const qreal minY = m_worldBounds.top();
    const qreal maxX = m_worldBounds.right() - boundingRect().width();
    const qreal maxY = m_worldBounds.bottom() - boundingRect().height();

    nextPos.setX(qBound(minX, nextPos.x(), maxX));
    nextPos.setY(qBound(minY, nextPos.y(), maxY));

    setPos(nextPos);
}

int PlayerEntity::hp() const {
    return m_hp;
}

void PlayerEntity::applyDamage(int damage) {
    if (damage == 0) {
        return;
    }

    const int nextHp = m_hp - damage;
    m_hp = qBound(0, nextHp, 100);
}

void PlayerEntity::applyTrapDamage(int damage) {
    if (damage == 0 || m_trapped) {
        return;
    }

    applyDamage(damage);
    m_trapped = true;
    m_trapRemaining = 3.0;
}

bool PlayerEntity::isTrapped() const {
    return m_trapped;
}

double PlayerEntity::trapRemainingSeconds() const {
    return m_trapRemaining;
}
