#include "WeaponSystem.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QtMath>

#include "BulletItem.h"

namespace {
QPointF normalizedOrFallback(const QPointF& input) {
    const double len = std::hypot(input.x(), input.y());
    if (len > 0.0001) {
        return QPointF(input.x() / len, input.y() / len);
    }
    return QPointF(1.0, 0.0);
}
} // namespace

BaseWeapon::BaseWeapon(WeaponConfig config)
    : m_config(std::move(config)) {}

BaseWeapon::FireResult BaseWeapon::tryFire(const QPointF& origin,
                                           const QPointF& direction,
                                           double nowTime,
                                           int currentAmmo,
                                           const QRectF& worldBounds,
                                           double fireRateMultiplier,
                                           int penetrationBonus) {
    FireResult result;

    const double fireRate = qBound(0.01, m_config.fireRate * qBound(0.75, fireRateMultiplier, 1.50), 60.0);
    const int ammoCost = qBound(1, m_config.ammoCost, 999);
    const int pellets = qBound(1, pelletCount(), 24);

    const double minInterval = 1.0 / fireRate;
    if ((nowTime - m_lastFireTime) < minInterval) {
        result.feedbackEvent = QStringLiteral("weapon_on_cooldown");
        return result;
    }

    if (currentAmmo < ammoCost) {
        result.feedbackEvent = QStringLiteral("weapon_no_ammo");
        return result;
    }

    const QPointF normalizedDirection = normalizedOrFallback(direction);
    const double recoilMagnitude = qBound(0.0, m_config.recoil, 2.0) * 6.0;
    const double recoilOffset = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0)
                                * recoilMagnitude;
    const QPointF baseDirection = rotateByDegrees(normalizedDirection, recoilOffset);

    for (int i = 0; i < pellets; ++i) {
        double offsetDeg = 0.0;
        if (pellets == 1) {
            const double u = QRandomGenerator::global()->generateDouble(); // [0, 1)
            offsetDeg = (u * 2.0 - 1.0) * m_config.spreadAngle;
        } else {
            const double t = (pellets == 1) ? 0.5 : static_cast<double>(i) / static_cast<double>(pellets - 1);
            offsetDeg = -m_config.spreadAngle + (2.0 * m_config.spreadAngle * t);
        }

        const QPointF pelletDir = rotateByDegrees(baseDirection, offsetDeg);
        const double bulletSpeed = qBound(100.0, m_config.bulletSpeed, 3000.0);
        const int bulletDamage = qBound(1, m_config.damage + qMax(0, penetrationBonus) * 2, 9999);

        result.bullets.push_back(std::make_unique<BulletItem>(origin,
                                                              pelletDir,
                                                              worldBounds,
                                                              bulletSpeed,
                                                              bulletDamage));
    }

    m_lastFireTime = nowTime;
    result.fired = true;
    result.feedbackEvent = QStringLiteral("weapon_fired");
    result.ammoConsumed = ammoCost;
    return result;
}

const WeaponConfig& BaseWeapon::config() const {
    return m_config;
}

QString BaseWeapon::weaponName() const {
    return m_config.name;
}

int BaseWeapon::pelletCount() const {
    return qMax(1, m_config.pelletsPerShot);
}

QPointF BaseWeapon::rotateByDegrees(const QPointF& dir, double deg) const {
    const double rad = qDegreesToRadians(deg);
    const double cs = std::cos(rad);
    const double sn = std::sin(rad);
    return QPointF(dir.x() * cs - dir.y() * sn,
                   dir.x() * sn + dir.y() * cs);
}

PistolWeapon::PistolWeapon(const WeaponConfig& config)
    : BaseWeapon(config) {}

RifleWeapon::RifleWeapon(const WeaponConfig& config)
    : BaseWeapon(config) {}

ShotgunWeapon::ShotgunWeapon(const WeaponConfig& config)
    : BaseWeapon(config) {}

int ShotgunWeapon::pelletCount() const {
    return qMax(3, config().pelletsPerShot);
}

WeaponConfig WeaponConfigLoader::loadFromJson(const QString& path, const WeaponConfig& fallback) {
    WeaponConfig cfg = fallback;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return cfg;
    }

    const QByteArray bytes = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        return cfg;
    }

    const QJsonObject root = doc.object();
    if (root.contains("name") && root.value("name").isString()) {
        cfg.name = root.value("name").toString();
    }
    if (root.contains("fireRate") && root.value("fireRate").isDouble()) {
        cfg.fireRate = root.value("fireRate").toDouble(cfg.fireRate);
    }
    if (root.contains("damage") && root.value("damage").isDouble()) {
        cfg.damage = root.value("damage").toInt(cfg.damage);
    }
    if (root.contains("bulletSpeed") && root.value("bulletSpeed").isDouble()) {
        cfg.bulletSpeed = root.value("bulletSpeed").toDouble(cfg.bulletSpeed);
    }
    if (root.contains("spreadAngle") && root.value("spreadAngle").isDouble()) {
        cfg.spreadAngle = root.value("spreadAngle").toDouble(cfg.spreadAngle);
    }
    if (root.contains("recoil") && root.value("recoil").isDouble()) {
        cfg.recoil = root.value("recoil").toDouble(cfg.recoil);
    }
    if (root.contains("ammoCost") && root.value("ammoCost").isDouble()) {
        cfg.ammoCost = root.value("ammoCost").toInt(cfg.ammoCost);
    }
    if (root.contains("pelletsPerShot") && root.value("pelletsPerShot").isDouble()) {
        cfg.pelletsPerShot = root.value("pelletsPerShot").toInt(cfg.pelletsPerShot);
    }

    cfg.name = cfg.name.trimmed();
    if (cfg.name.isEmpty()) {
        cfg.name = fallback.name;
    }
    cfg.fireRate = qBound(0.1, cfg.fireRate, 60.0);
    cfg.damage = qBound(1, cfg.damage, 9999);
    cfg.bulletSpeed = qBound(100.0, cfg.bulletSpeed, 3000.0);
    cfg.spreadAngle = qBound(0.0, cfg.spreadAngle, 45.0);
    cfg.recoil = qBound(0.0, cfg.recoil, 2.0);
    cfg.ammoCost = qBound(1, cfg.ammoCost, 100);
    cfg.pelletsPerShot = qBound(1, cfg.pelletsPerShot, 24);

    return cfg;
}
