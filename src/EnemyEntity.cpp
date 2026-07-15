#include "EnemyEntity.h"
#include "EnemyBulletItem.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector2D>
#include <QtMath>

namespace {
QPointF normalizedOrFallback(const QPointF& input) {
    const double len = std::hypot(input.x(), input.y());
    if (len > 0.0001) {
        return QPointF(input.x() / len, input.y() / len);
    }
    return QPointF(1.0, 0.0);
}

QPixmap loadEnemyTexture(const QString& fileName) {
    QString assetPath = QStringLiteral("assets/") + fileName;
    QString appDir = QCoreApplication::applicationDirPath();
    QString resolvedPath = QDir(appDir).filePath(assetPath);

    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../") + assetPath);
    }
    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../../") + assetPath);
    }

    return QPixmap(resolvedPath);
}
} // namespace

EnemyEntity::EnemyEntity(const QPointF& spawnPos,
                         Archetype archetype,
                         const AIConfig& config)
    : m_archetype(archetype)
    , m_config(config)
    , m_hp(archetype == Archetype::Ranged ? 31 : 1) {
    QString textureName = (archetype == Archetype::Melee) ? QStringLiteral("enemy_melee.png") : QStringLiteral("enemy_ranged.png");
    QPixmap pix = loadEnemyTexture(textureName);

    if (!pix.isNull()) {
        setPixmap(pix.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        setPixmap(QPixmap(70, 70));
    }

    setPos(spawnPos);
}

void EnemyEntity::update(double deltaTimeSeconds, const QPointF& targetPos) {
    if (!m_alive) {
        return;
    }

    m_attackCooldownRemaining = qMax(0.0, m_attackCooldownRemaining - deltaTimeSeconds);
    m_retreatRemaining = qMax(0.0, m_retreatRemaining - deltaTimeSeconds);

    const QPointF toTarget = targetPos - sceneBoundingRect().center();
    const double distance = std::hypot(toTarget.x(), toTarget.y());
    const QPointF dirToTarget = normalizedOrFallback(toTarget);
    m_attackDirToPlayer = dirToTarget;

    const bool canDetect = distance <= m_config.detectRadius;

    if (m_state == State::Idle && canDetect) {
        transitionTo(State::Chase, QStringLiteral("player_detected"));
    }

    if (!canDetect && m_state != State::Idle) {
        transitionTo(State::Idle, QStringLiteral("target_lost"));
    }

    if (m_state == State::Chase) {
        bool shouldAttack = false;
        if (m_archetype == Archetype::Melee) {
            shouldAttack = distance <= m_config.attackRange;
        } else {
            shouldAttack = distance <= m_config.attackRange && distance >= 50.0;
        }

        if (shouldAttack) {
            transitionTo(State::Attack, QStringLiteral("in_attack_range"));
        }
    }

    if (m_state == State::Attack) {
        bool leaveAttack = false;
        if (m_archetype == Archetype::Melee) {
            leaveAttack = distance > m_config.attackRange * 1.35;
        } else {
            leaveAttack = distance > m_config.attackRange || distance < 30.0;
        }

        if (leaveAttack) {
            transitionTo(State::Chase, QStringLiteral("attack_range_broken"));
        }
    }

    if (m_state == State::Idle) {
        return;
    }

    if (m_state == State::Chase) {
        QPointF moveDir = dirToTarget;

        if (m_archetype == Archetype::Ranged) {
            if (distance < m_config.preferredRange * 0.90) {
                moveDir = -dirToTarget;
            }
        }

        setPos(pos() + moveDir * (m_config.speed * deltaTimeSeconds));
        return;
    }

    if (m_state == State::Attack) {
        if (m_attackCooldownRemaining <= 0.0) {
            m_attackCooldownRemaining = m_config.attackInterval;
            if (m_archetype == Archetype::Melee) {
                m_pendingAttackDamage += qMax(1, m_config.attackDamage);
            } else {
                m_needsToShoot = true;
            }
        }

        if (m_archetype == Archetype::Ranged) {
            if (distance < m_config.preferredRange * 0.90) {
                setPos(pos() - dirToTarget * (m_config.speed * 0.55 * deltaTimeSeconds));
            } else if (distance > m_config.preferredRange * 1.10) {
                setPos(pos() + dirToTarget * (m_config.speed * 0.45 * deltaTimeSeconds));
            }
        }
    }
}

bool EnemyEntity::isAlive() const {
    return m_alive;
}

void EnemyEntity::kill() {
    m_alive = false;
}

int EnemyEntity::consumePendingAttackDamage() {
    const int damage = m_pendingAttackDamage;
    m_pendingAttackDamage = 0;
    return damage;
}

QPointF EnemyEntity::attackDirectionToPlayer() const {
    return m_attackDirToPlayer;
}

EnemyEntity::Archetype EnemyEntity::archetype() const {
    return m_archetype;
}

EnemyEntity::State EnemyEntity::state() const {
    return m_state;
}

void EnemyEntity::applyDamage(int damage) {
    if (damage <= 0 || !m_alive) {
        return;
    }
    m_hp -= damage;
    if (m_hp <= 0) {
        m_alive = false;
    }
}

int EnemyEntity::hp() const {
    return m_hp;
}

std::vector<std::unique_ptr<EnemyBulletItem>> EnemyEntity::shoot(const QRectF& worldBounds) {
    std::vector<std::unique_ptr<EnemyBulletItem>> bullets;

    if (!m_needsToShoot || m_archetype != Archetype::Ranged) {
        return bullets;
    }

    m_needsToShoot = false;

    const QPointF spawnPos = sceneBoundingRect().center();
    const int bulletCount = 6;
    const double bulletSpeed = 150.0;

    for (int i = 0; i < bulletCount; ++i) {
        const double angle = (2.0 * M_PI / bulletCount) * i;
        const QPointF direction(std::cos(angle), std::sin(angle));
        bullets.push_back(std::make_unique<EnemyBulletItem>(spawnPos, direction, worldBounds, bulletSpeed, 10));
    }

    return bullets;
}

EnemyEntity::AIConfig EnemyEntity::loadConfigFromJson(const QString& path, const AIConfig& fallback) {
    AIConfig cfg = fallback;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return cfg;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return cfg;
    }

    const QJsonObject root = doc.object();
    if (root.contains("detectRadius") && root.value("detectRadius").isDouble()) {
        cfg.detectRadius = root.value("detectRadius").toDouble(cfg.detectRadius);
    }
    if (root.contains("speed") && root.value("speed").isDouble()) {
        cfg.speed = root.value("speed").toDouble(cfg.speed);
    }
    if (root.contains("attackRange") && root.value("attackRange").isDouble()) {
        cfg.attackRange = root.value("attackRange").toDouble(cfg.attackRange);
    }
    if (root.contains("preferredRange") && root.value("preferredRange").isDouble()) {
        cfg.preferredRange = root.value("preferredRange").toDouble(cfg.preferredRange);
    }
    if (root.contains("attackInterval") && root.value("attackInterval").isDouble()) {
        cfg.attackInterval = root.value("attackInterval").toDouble(cfg.attackInterval);
    }
    if (root.contains("attackDamage") && root.value("attackDamage").isDouble()) {
        cfg.attackDamage = root.value("attackDamage").toInt(cfg.attackDamage);
    }
    if (root.contains("retreatHpThreshold") && root.value("retreatHpThreshold").isDouble()) {
        cfg.retreatHpThreshold = root.value("retreatHpThreshold").toInt(cfg.retreatHpThreshold);
    }
    if (root.contains("retreatDuration") && root.value("retreatDuration").isDouble()) {
        cfg.retreatDuration = root.value("retreatDuration").toDouble(cfg.retreatDuration);
    }

    return cfg;
}

void EnemyEntity::transitionTo(State next, const QString& reason) {
    if (next == m_state) {
        return;
    }

    qInfo() << "[EnemyAI]" << (m_archetype == Archetype::Melee ? "Melee" : "Ranged")
            << stateName(m_state) << "->" << stateName(next)
            << "reason=" << reason;

    m_state = next;
    if (m_state == State::Retreat) {
        m_retreatRemaining = qMax(0.1, m_config.retreatDuration);
    }
}

QString EnemyEntity::stateName(State state) {
    switch (state) {
    case State::Idle:
        return QStringLiteral("Idle");
    case State::Chase:
        return QStringLiteral("Chase");
    case State::Attack:
        return QStringLiteral("Attack");
    case State::Retreat:
        return QStringLiteral("Retreat");
    }

    return QStringLiteral("Unknown");
}
