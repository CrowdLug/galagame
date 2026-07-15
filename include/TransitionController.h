#pragma once

#include <QObject>
#include <QString>

class TransitionController final : public QObject {
    Q_OBJECT

public:
    explicit TransitionController(QObject* parent = nullptr);
    ~TransitionController() override;

    void setAudioFadeEnabled(bool enabled);
    bool isAudioFadeEnabled() const;

    void beginTransition(const QString& fromScene,
                         const QString& toScene,
                         const QString& reason);
    void endTransition();

private:
    bool m_audioFadeEnabled {false};
    bool m_transitionInFlight {false};
};
