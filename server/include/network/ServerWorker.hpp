#ifndef SERVER_NETWORK_SERVERWORKER_HPP
#define SERVER_NETWORK_SERVERWORKER_HPP

/**
 * @class ServerWorker
 * @brief Worker thread handler for processing individual client RTSP requests
 *
 * @details
 * ServerWorker handles:
 * - Processing RTSP commands (SETUP, PLAY, PAUSE, TEARDOWN) from one client
 * - Sending RTP video packets to client
 * - Managing client session state
 * - Cleanup on disconnect
 */
class ServerWorker
{
    
};
#endif