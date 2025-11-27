#ifndef SERVER_VIDEO_VIDEOSTREAM_HPP
#define SERVER_VIDEO_VIDEOSTREAM_HPP

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <iostream>

/**
 * @class VideoStream
 * @brief Đọc video frames từ MJPEG file
 *
 * @details
 * VideoStream class quản lý việc đọc MJPEG file format:
 * - MJPEG format: Chuỗi các JPEG images
 * - Mỗi frame có format: [5-byte length header][JPEG data]
 * - Header chứa frame length ở big-endian format
 *
 * MJPEG File Structure:
 * @code
 * [Frame 0 length (5 bytes)][Frame 0 JPEG data (variable)]
 * [Frame 1 length (5 bytes)][Frame 1 JPEG data (variable)]
 * [Frame 2 length (5 bytes)][Frame 2 JPEG data (variable)]
 * ...
 * @endcode
 *
 * Frame Length Header (5 bytes):
 * @code
 * Byte 0: Most significant byte
 * Byte 1: ...
 * Byte 2: ...
 * Byte 3: ...
 * Byte 4: Least significant byte
 * Total: 32-bit length in big-endian format (extended to 40 bits)
 * @endcode
 *
 * @example Server Usage:
 * @code
 * // Open video file
 * VideoStream video("movie.Mjpeg");
 *
 * // Read frames in loop
 * while (video.hasMoreFrames()) {
 *     auto frameData = video.nextFrame();
 *
 *     // Create RTP packets from frame
 *     RTPPacket packet = RTPPacketBuilder()
 *         .setSequenceNumber(seqNum++)
 *         .setTimestamp(getCurrentTime())
 *         .setPayload(frameData.data(), frameData.size())
 *         .build();
 *
 *     // Send packet
 *     udpSocket->sendTo(packet.getPacket(), clientIP, clientRTPPort);
 * }
 *
 * // Rewind for loop playback
 * video.rewind();
 * @endcode
 *
 * @note This class is used by ServerWorker to read video frames
 * @note Each frame is a complete JPEG image (can be saved as .jpg)
 */
class VideoStream
{
private:
    std::ifstream videoFile_; ///< Input file stream
    int frameNumber_;         ///< Current frame number (0-indexed)
    std::string filename_;    ///< Video file path

public:
    /**
     * @brief Constructor - opens MJPEG file
     * @param filename Path to MJPEG file (e.g., "movie.Mjpeg")
     * @throws std::runtime_error if file cannot be opened
     *
     * @details
     * - Opens file in binary mode
     * - Validates file exists
     * - Initializes frame counter to 0
     *
     * @example
     * @code
     * VideoStream video("movie.Mjpeg");
     * @endcode
     */
    VideoStream(const std::string &filename);

    /**
     * @brief Destructor - closes file
     */
    ~VideoStream();

    /**
     * @brief Read next frame from MJPEG file
     * @return Vector containing frame data (JPEG bytes)
     * @throws std::runtime_error if read fails or no more frames
     *
     * @details
     * Reading process:
     * 1. Read 5-byte header (frame length in big-endian)
     * 2. Extract uint32_t length from header
     * 3. Read frameLength bytes (JPEG data)
     * 4. Increment frame counter
     * 5. Return frame data as vector
     *
     * @example
     * @code
     * std::vector<uint8_t> frameData = video.nextFrame();
     * std::cout << "Frame " << video.getCurrentFrameNumber()
     *           << " size: " << frameData.size() << " bytes" << std::endl;
     * @endcode
     *
     * @note Frame data is complete JPEG image (can be saved as .jpg)
     */
    std::vector<uint8_t> nextFrame();

    /**
     * @brief Rewind to beginning of file
     *
     * @details
     * - Seeks to file start (position 0)
     * - Resets frame counter to 0
     * - Clears error flags
     * - Useful for loop playback
     *
     * @example
     * @code
     * // Play video in loop
     * while (true) {
     *     while (video.hasMoreFrames()) {
     *         auto frame = video.nextFrame();
     *         // Send frame...
     *     }
     *     video.rewind(); // Start over
     * }
     * @endcode
     */
    void rewind();

    /**
     * @brief Check if more frames available
     * @return true if can read more frames, false if EOF or error
     *
     * @details
     * Checks:
     * - File stream is good (no errors)
     * - Not at end of file
     *
     * @example
     * @code
     * if (!video.hasMoreFrames()) {
     *     std::cout << "Reached end of video" << std::endl;
     *     video.rewind(); // Start over
     * }
     * @endcode
     */
    bool hasMoreFrames() const;

    /**
     * @brief Get current frame number
     * @return Frame number (0-indexed)
     *
     * @details
     * - Incremented after each nextFrame() call
     * - Reset to 0 by rewind()
     * - Useful for logging and debugging
     *
     * @example
     * @code
     * std::cout << "Currently at frame " << video.getCurrentFrameNumber() << std::endl;
     * @endcode
     */
    int getCurrentFrameNumber() const { return frameNumber_; }
};

#endif