#include "ResourceManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPainter>

namespace {
QString resolveAssetPath(const QString& relativePath) {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString direct = QDir(appDir).filePath(relativePath);
    if (QFile::exists(direct)) {
        return direct;
    }

    const QString parent = QDir(appDir).filePath(QStringLiteral("../") + relativePath);
    if (QFile::exists(parent)) {
        return parent;
    }

    const QString parent2 = QDir(appDir).filePath(QStringLiteral("../../") + relativePath);
    if (QFile::exists(parent2)) {
        return parent2;
    }

    return relativePath;
}
} // namespace

ResourceManager::ResourceManager() = default;

QPixmap ResourceManager::loadTexture(const QString& path) {
    const QString key = path.trimmed();
    if (key.isEmpty()) {
        ++m_stats.textureCacheMiss;
        return placeholderTexture();
    }

    if (m_textureCache.contains(key)) {
        ++m_stats.textureCacheHit;
        return m_textureCache.value(key);
    }

    const QString resolvedPath = resolveAssetPath(key);
    QPixmap pix;
    if (!pix.load(resolvedPath)) {
        ++m_stats.textureCacheMiss;
        pix = placeholderTexture();
    } else {
        ++m_stats.textureCacheMiss;
    }

    m_textureCache.insert(key, pix);
    return pix;
}

QString ResourceManager::loadSfx(const QString& path) {
    const QString key = path.trimmed();
    if (key.isEmpty()) {
        ++m_stats.audioCacheMiss;
        return QStringLiteral("silent://sfx");
    }

    if (m_audioCache.contains(key)) {
        ++m_stats.audioCacheHit;
        return m_audioCache.value(key);
    }

    const QString resolvedPath = resolveAssetPath(key);
    const bool exists = QFile::exists(resolvedPath);
    ++m_stats.audioCacheMiss;
    const QString resolved = exists ? resolvedPath : QStringLiteral("silent://sfx");
    m_audioCache.insert(key, resolved);
    return resolved;
}

QString ResourceManager::loadBgm(const QString& path) {
    const QString key = path.trimmed();
    if (key.isEmpty()) {
        ++m_stats.audioCacheMiss;
        return QStringLiteral("silent://bgm");
    }

    if (m_audioCache.contains(key)) {
        ++m_stats.audioCacheHit;
        return m_audioCache.value(key);
    }

    const QString resolvedPath = resolveAssetPath(key);
    const bool exists = QFile::exists(resolvedPath);
    ++m_stats.audioCacheMiss;
    const QString resolved = exists ? resolvedPath : QStringLiteral("silent://bgm");
    m_audioCache.insert(key, resolved);
    return resolved;
}

void ResourceManager::preloadForSceneKey(const QString& sceneKey) {
    if (sceneKey == QStringLiteral("Combat")) {
        if (!m_textureCache.contains(QStringLiteral("assets/player.png"))) {
            loadTexture(QStringLiteral("assets/player.png"));
        }
        if (!m_textureCache.contains(QStringLiteral("assets/enemy_melee.png"))) {
            loadTexture(QStringLiteral("assets/enemy_melee.png"));
        }
        if (!m_textureCache.contains(QStringLiteral("assets/enemy_ranged.png"))) {
            loadTexture(QStringLiteral("assets/enemy_ranged.png"));
        }
        if (!m_audioCache.contains(QStringLiteral("assets/sfx/hit.wav"))) {
            loadSfx(QStringLiteral("assets/sfx/hit.wav"));
        }
        if (!m_audioCache.contains(QStringLiteral("assets/bgm/combat.ogg"))) {
            loadBgm(QStringLiteral("assets/bgm/combat.ogg"));
        }
    } else if (sceneKey == QStringLiteral("Menu")) {
        if (!m_textureCache.contains(QStringLiteral("assets/ui/menu_bg.png"))) {
            loadTexture(QStringLiteral("assets/ui/menu_bg.png"));
        }
        if (!m_audioCache.contains(QStringLiteral("assets/bgm/taffy_menu.mp3"))) {
            loadBgm(QStringLiteral("assets/bgm/taffy_menu.mp3"));
        }
    } else if (sceneKey == QStringLiteral("Result")) {
        if (!m_textureCache.contains(QStringLiteral("assets/ui/taffy_defeat_bg.png"))) {
            loadTexture(QStringLiteral("assets/ui/taffy_defeat_bg.png"));
        }
    }
}

void ResourceManager::releaseForSceneKey(const QString& sceneKey) {
    if (sceneKey == QStringLiteral("Combat")) {
        m_textureCache.remove(QStringLiteral("assets/enemy_melee.png"));
        m_textureCache.remove(QStringLiteral("assets/enemy_ranged.png"));
        m_audioCache.remove(QStringLiteral("assets/sfx/hit.wav"));
    }
}

ResourceManager::Stats ResourceManager::stats() const {
    return m_stats;
}

QString ResourceManager::statsSummary() const {
    return QStringLiteral("ResCache Tex(H/M): %1/%2 Audio(H/M): %3/%4")
        .arg(m_stats.textureCacheHit)
        .arg(m_stats.textureCacheMiss)
        .arg(m_stats.audioCacheHit)
        .arg(m_stats.audioCacheMiss);
}

QPixmap ResourceManager::placeholderTexture() const {
    QPixmap pix(64, 64);
    pix.fill(QColor(255, 0, 255, 220));

    QPainter p(&pix);
    p.setPen(Qt::black);
    p.drawLine(0, 0, 63, 63);
    p.drawLine(63, 0, 0, 63);
    p.end();

    return pix;
}
