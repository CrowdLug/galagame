#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>

#include <memory>
#include <vector>

class BulletItem;

struct WeaponConfig {
    QString name;
    double fireRate {3.0};
    int damage {20};
    double bulletSpeed {760.0};
    double spreadAngle {0.0};
    double recoil {0.0};
    int ammoCost {1};
    int pelletsPerShot {1};
};

class BaseWeapon {
public:
    struct FireResult {
        bool fired {false};
        QString feedbackEvent;
        int ammoConsumed {0};
        std::vector<std::unique_ptr<BulletItem>> bullets;
    };

    explicit BaseWeapon(WeaponConfig config);
    virtual ~BaseWeapon() = default;

    FireResult tryFire(const QPointF& origin,
                      const QPointF& direction,
                      double nowTime,
                      int currentAmmo,
                      const QRectF& worldBounds,
                      double fireRateMultiplier = 1.0,
                      int penetrationBonus = 0);

    const WeaponConfig& config() const;
    QString weaponName() const;

protected:
    virtual int pelletCount() const;

private:
    QPointF rotateByDegrees(const QPointF& dir, double deg) const;

    WeaponConfig m_config;
    double m_lastFireTime {-1000.0};
};

class PistolWeapon final : public BaseWeapon {
public:
    explicit PistolWeapon(const WeaponConfig& config);
};

class RifleWeapon final : public BaseWeapon {
public:
    explicit RifleWeapon(const WeaponConfig& config);
};

class ShotgunWeapon final : public BaseWeapon {
public:
    explicit ShotgunWeapon(const WeaponConfig& config);

protected:
    int pelletCount() const override;
};

class WeaponConfigLoader final {
public:
    static WeaponConfig loadFromJson(const QString& path, const WeaponConfig& fallback);
};
