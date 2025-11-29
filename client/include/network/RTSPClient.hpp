#ifndef CLIENT_NETWORK_RTSPCLIENT_HPP
#define CLIENT_NETWORK_RTSPCLIENT_HPP

#include <map>
#include <memory>
#include <string>
#include "network/Socket.hpp"

/**
 * @class RTSPClient
 * @brief RTSP protocol client implementation
 *
 * @details
 * RTSPClient implements RTSP (Real-Time Streaming Protocol) client-side:
 * - Sends RTSP commands to server (SETUP, PLAY, PAUSE, TEARDOWN)
 * - Parses RTSP responses from server
 * - Manages RTSP session state machine
 * - Maintains session ID and sequence numbers
 *
 * RTSP State Machine:
 * @code
 * INIT ──[SETUP OK]──► READY ──[PLAY OK]──► PLAYING
 *                        ▲         │
 *                        │         │
 *                        └─[PAUSE]─┘
 *                        │
 *                        └─[TEARDOWN]──► INIT
 * @endcode
 *
 * RTSP Request Format:
 * @code
 * METHOD URL RTSP/1.0\r\n
 * CSeq: <sequence_number>\r\n
 * Session: <session_id>\r\n (if applicable)
 * <other_headers>\r\n
 * \r\n
 * @endcode
 *
 * RTSP Response Format:
 * @code
 * RTSP/1.0 <status_code> <reason>\r\n
 * CSeq: <sequence_number>\r\n
 * Session: <session_id>\r\n (if applicable)
 * <other_headers>\r\n
 * \r\n
 * @endcode
 *
 * @example Basic Usage:
 * @code
 * // Create client
 * RTSPClient client("127.0.0.1", 8554);
 *
 * // Setup session
 * if (client.sendSetup("movie.Mjpeg", 25000)) {
 *     std::cout << "SETUP successful, Session ID: "
 *               << client.getSessionId() << std::endl;
 * }
 *
 * // Start playback
 * if (client.sendPlay()) {
 *     std::cout << "Playing video..." << std::endl;
 * }
 *
 * // Pause
 * if (client.sendPause()) {
 *     std::cout << "Paused" << std::endl;
 * }
 *
 * // Cleanup
 * client.sendTeardown();
 * @endcode
 */
class RTSPClient {
  public:
    /**
     * @enum State
     * @brief RTSP client state machine states
     */
    enum class State {
        INIT,    ///< Initial state - no session
        READY,   ///< Session established, ready to play
        PLAYING  ///< Currently streaming video
    };

  private:
    State state_;            ///< Current state
    int cseq_;               ///< RTSP CSeq counter
    std::string sessionId_;  ///< Session ID from server

    std::unique_ptr<Socket> rtspSocket_;  ///< TCP socket for RTSP
    std::string serverIP_;                ///< Server IP address
    int serverPort_;                      ///< Server RTSP port (default 8554)

    // RTP info
    int clientRTPPort_;  ///< Local port for receiving RTP
    int serverRTPPort_;  ///< Server RTP port (from SETUP response)

    /**
     * @brief Send RTSP request and receive response
     * @param request RTSP request string (complete message)
     * @return RTSP response string
     * @throws SocketException if send/receive fails
     *
     * @details
     * - Sends request via TCP socket
     * - Blocks until response received
     * - Logs request/response for debugging
     * - Validates response format
     */
    std::string sendRtspRequest(const std::string& request);

    /**
     * @brief Parse RTSP response status line
     * @param response RTSP response string
     * @return Status code (200 = OK, 404 = Not Found, etc.)
     *
     * @details
     * Parses first line: "RTSP/1.0 200 OK"
     * Extracts status code (200)
     */
    int parseStatusCode(const std::string& response) const;

    /**
     * @brief Extract Session ID from RTSP response
     * @param response RTSP response string
     * @return Session ID string (empty if not found)
     *
     * @details
     * Searches for "Session: " header
     * Extracts session ID value
     *
     * Example header: "Session: 123456789\r\n"
     */
    std::string extractSessionId(const std::string& response) const;

    /**
     * @brief Extract server RTP port from SETUP response
     * @param response RTSP response string
     * @return Server RTP port number
     *
     * @details
     * Parses Transport header:
     * "Transport: RTP/UDP; server_port=25000\r\n"
     */
    int extractServerRTPPort(const std::string& response) const;

    /**
     * @brief Validate state transition
     * @param expectedState Required state for operation
     * @return true if in correct state, false otherwise
     */
    bool validateState(State expectedState) const;

  public:
    /**
     * @brief Constructor - creates RTSP client
     * @param serverIP Server IP address (e.g., "127.0.0.1")
     * @param serverPort Server RTSP port (default 8554)
     * @throws SocketException if connection fails
     *
     * @details
     * - Creates TCP socket
     * - Connects to server
     * - Sets timeout (5 seconds)
     * - Initializes state to INIT
     * - Sets CSeq to 0
     */
    RTSPClient(const std::string& serverIP, int serverPort = 8554);

    /**
     * @brief Destructor - closes connection
     *
     * @details
     * - Sends TEARDOWN if in PLAYING/READY state
     * - Closes TCP socket
     */
    ~RTSPClient();

    // ==================== RTSP COMMANDS ====================

    /**
     * @brief Send RTSP SETUP request
     * @param videoFile Video filename on server (e.g., "movie.Mjpeg")
     * @param clientRTPPort Local UDP port for receiving RTP (e.g., 25000)
     * @return true if SETUP successful (200 OK), false otherwise
     *
     * @details
     * SETUP request format:
     * @code
     * SETUP movie.Mjpeg RTSP/1.0\r\n
     * CSeq: 1\r\n
     * Transport: RTP/UDP; client_port=25000\r\n
     * \r\n
     * @endcode
     *
     * Expected response:
     * @code
     * RTSP/1.0 200 OK\r\n
     * CSeq: 1\r\n
     * Session: 123456789\r\n
     * Transport: RTP/UDP; client_port=25000; server_port=25000\r\n
     * \r\n
     * @endcode
     *
     * State transition: INIT → READY
     *
     * @note Must be in INIT state
     * @note Stores session ID from response
     */
    bool sendSetup(const std::string& videoFile, int clientRTPPort);

    /**
     * @brief Send RTSP PLAY request
     * @return true if PLAY successful (200 OK), false otherwise
     *
     * @details
     * PLAY request format:
     * @code
     * PLAY RTSP/1.0\r\n
     * CSeq: 2\r\n
     * Session: 123456789\r\n
     * \r\n
     * @endcode
     *
     * State transition: READY → PLAYING
     *
     * @note Must be in READY state
     * @note Server starts sending RTP packets after this
     */
    bool sendPlay();

    /**
     * @brief Send RTSP PAUSE request
     * @return true if PAUSE successful (200 OK), false otherwise
     *
     * @details
     * PAUSE request format:
     * @code
     * PAUSE RTSP/1.0\r\n
     * CSeq: 3\r\n
     * Session: 123456789\r\n
     * \r\n
     * @endcode
     *
     * State transition: PLAYING → READY
     *
     * @note Must be in PLAYING state
     * @note Server stops sending RTP packets
     */
    bool sendPause();

    /**
     * @brief Send RTSP TEARDOWN request
     * @return true if TEARDOWN successful (200 OK), false otherwise
     *
     * @details
     * TEARDOWN request format:
     * @code
     * TEARDOWN RTSP/1.0\r\n
     * CSeq: 4\r\n
     * Session: 123456789\r\n
     * \r\n
     * @endcode
     *
     * State transition: Any state → INIT
     *
     * @note Can be called from any state
     * @note Closes session, releases resources
     */
    bool sendTeardown();

    // ==================== GETTERS ====================

    /**
     * @brief Get current state
     * @return Current RTSP state
     */
    State getState() const { return state_; }

    /**
     * @brief Get session ID
     * @return Session ID string (empty if no session)
     */
    std::string getSessionId() const { return sessionId_; }

    /**
     * @brief Get client RTP port
     * @return Local UDP port for RTP
     */
    int getClientRTPPort() const { return clientRTPPort_; }

    /**
     * @brief Get server RTP port
     * @return Server UDP port for RTP (from SETUP response)
     */
    int getServerRTPPort() const { return serverRTPPort_; }

    /**
     * @brief Get state as string (for debugging)
     * @return State name ("INIT", "READY", "PLAYING")
     */
    std::string getStateString() const;
};

#endif