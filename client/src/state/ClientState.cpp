#include "state/ClientState.hpp"

bool RTSPClient::InitState::handleSetup(const std::string& videoFile, int clientRTPort) {
    (void)videoFile;
    (void)clientRTPort;
    return true;
}

bool RTSPClient::ReadyState::handlePlay() {
    return true;
}

bool RTSPClient::ReadyState::handleTearDown() {
    return true;
}

bool RTSPClient::PlayingState::handlePause() {
    return true;
}

bool RTSPClient::PlayingState::handleTearDown() {
    return true;
}
