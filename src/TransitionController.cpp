#include "TransitionController.h"

#include <QDebug>

TransitionController::TransitionController(QObject* parent)
    : QObject(parent) {
    qInfo() << "[TransitionController] Constructed";
}

TransitionController::~TransitionController() {
    qInfo() << "[TransitionController] Destroyed";
}

void TransitionController::setAudioFadeEnabled(bool enabled) {
    m_audioFadeEnabled = enabled;
    qInfo() << "[TransitionController] Audio fade toggled" << "enabled=" << m_audioFadeEnabled;
}

bool TransitionController::isAudioFadeEnabled() const {
    return m_audioFadeEnabled;
}

void TransitionController::beginTransition(const QString& fromScene,
                                           const QString& toScene,
                                           const QString& reason) {
    if (m_transitionInFlight) {
        qWarning() << "[TransitionController] beginTransition ignored: transition already in flight"
                   << "from=" << fromScene
                   << "to=" << toScene
                   << "reason=" << reason;
        return;
    }

    m_transitionInFlight = true;

    qInfo() << "[TransitionController] Fade-out started"
            << "from=" << fromScene
            << "to=" << toScene
            << "reason=" << reason
            << "audioFadeEnabled=" << m_audioFadeEnabled;
}

void TransitionController::endTransition() {
    if (!m_transitionInFlight) {
        qWarning() << "[TransitionController] endTransition ignored: no transition in flight";
        return;
    }

    qInfo() << "[TransitionController] Fade-in completed"
            << "audioFadeEnabled=" << m_audioFadeEnabled;

    m_transitionInFlight = false;
}
