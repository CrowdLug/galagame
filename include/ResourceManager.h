#pragma once

#include <QHash>
#include <QPixmap>
#include <QString>
#include <QStringList>

class ResourceManager final {
public:
    struct Stats {
        int textureCacheHit {0};
        int textureCacheMiss {0};
        int audioCacheHit {0};
        int audioCacheMiss {0};
    };

    ResourceManager();

    QPixmap loadTexture(const QString& path);
    QString loadSfx(const QString& path);
    QString loadBgm(const QString& path);

    void preloadForSceneKey(const QString& sceneKey);
    void releaseForSceneKey(const QString& sceneKey);

    Stats stats() const;
    QString statsSummary() const;

private:
    QPixmap placeholderTexture() const;

    QHash<QString, QPixmap> m_textureCache;
    QHash<QString, QString> m_audioCache;
    Stats m_stats;
};
