#ifndef COMMON_NETWORK_RTSPMESSAGE_HPP
#define COMMON_NETWORK_RTSPMESSAGE_HPP

#include <map>
#include <string>

/**
 * @class RTSPMessage
 * @brief Parser and builder for RTSP protocol messages
 * 
 * RTSP Message Format:
 * Request:  METHOD URL RTSP/1.0\r\nCSeq: N\r\n[Headers]\r\n\r\n
 * Response: RTSP/1.0 CODE REASON\r\nCSeq: N\r\n[Headers]\r\n\r\n
 */
class RTSPMessage {
public:
    /**
     * @struct Request
     * @brief Parsed RTSP request
     */
    struct Request{
        std::string method; ///< SETUP, PLAY, PAUSE, TEARDOWN
        std::string url; ///< Video filename or URL
        int cseq; ///< Sequence number 
        std::map<std::string, std::string> headers; ///< Additional headers
    };

    /**
    * @struct Response
    * @brief Parsed RTSP response
    */
    struct Response{
        int statusCode; ///< 200=OK, 404=NOT_FOUND, 500=Error
        std::string reason; ///< "OK", "NOT_FOUND", "ERROR"
        int cseq; ///< Sequence number 
        std::map<std::string, std::string> headers; ///< Session ID, Transport, etc.
    };

    /**
     * @brief Parse RTSP request string
     * @param message Raw RTSP request (e.g., "SETUP movie.Mjpeg RTSP/1.0\r\n...")
     * @return Parsed Request structure
     * @throws std::runtime_error if parsing fails
     */
    static Request parseRequest(const std::string& message); 

    /**
     * @brief Parse RTSP request string
     * @param message Raw RTSP request (e.g., "SETUP movie.Mjpeg RTSP/1.0\r\n...")
     * @return Parsed Request structure
     * @throws std::runtime_error if parsing fails
     */
    static Response parseResponse(const std::string& message);

    /**
     * @brief Build RTSP request string
     * @param method RTSP method (SETUP, PLAY, PAUSE, TEARDOWN)
     * @param url Video URL or filename
     * @param cseq Sequence number
     * @param headers Additional headers (Transport, Session, etc.)
     * @return Complete RTSP request string with \r\n line endings
     * 
     * @example
     * auto request = RTSPMessage::buildRequest(
     *     "SETUP", "movie.Mjpeg", 1, 
     *     {{"Transport", "RTP/UDP; client_port=25000"}}
     * );
     * // Output: "SETUP movie.Mjpeg RTSP/1.0\r\nCSeq: 1\r\n..."
     */
    static std::string buildRequest(const std::string &method,
                                    std::string url,
                                    int cseq,
                                    const std::map<std::string, std::string> &headers = {});

    /**
     * @brief Build RTSP response string
     * @param statusCode HTTP-style status code (200, 404, 500)
     * @param reason Status reason ("OK", "Not Found", "Internal Server Error")
     * @param cseq Sequence number (must match request)
     * @param headers Additional headers (Session, Transport, etc.)
     * @return Complete RTSP response string with \r\n line endings
     * 
     * @example
     * auto response = RTSPMessage::buildResponse(
     *     200, "OK", 1,
     *     {{"Session", "123456789"}, {"Transport", "RTP/UDP; server_port=25000"}}
     * );
     * // Output: "RTSP/1.0 200 OK\r\nCSeq: 1\r\nSession: 123456789\r\n..."
     */
    static std::string buildResponse(int statusCode,
                                    std::string reason, 
                                    int cseq,
                                    const std::map<std::string, std::string> &headers = {});

    /**
     * @brief Extract header value from headers map
     * @param headers Headers map
     * @param key Header key (case-insensitive)
     * @param defaultValue Default if not found
     * @return Header value or default
     */
    static std::string getHeader(const std::map<std::string, std::string> &headers,
                                const std::string &key,
                                const std::string &defaultValue = "");

    /**
     * @brief Convert status code to reason phrase
     * @param statusCode HTTP-style status code
     * @return Reason phrase (e.g., 200 → "OK")
     */
    static std::string statusCodeToReason(int statusCode);
};

#endif