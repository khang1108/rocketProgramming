#ifndef CLIENT_RTP_RTPRECEIVER_HPP
#define CLIENT_RTP_RTPRECEIVER_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include "network/Socket.hpp"
#include "rtp/RTPPacket.hpp"

/**
 * @class RTPReceiver
 * @brief Receives RTP packets from server via UDP
 * 
 * Runs in background thread, receives packets, and notifies callback
 */
class RTPReceiver 
{
public:
    /**
     * @typedef PacketCallback
     * @brief Callback function for received packets
     * @param packet Received RTP packet
     */
    using PacketCallback = std::function<void(const RTPPacket& packet)>;
private:
    std::unique_ptr<Socket> socket_;
    int rtpPort_;

    std::thread receiverThread_;
    std::atomic<bool> running_;

    PacketCallback callback_;

    std::atomic<uint64_t> packetReceived_;
    std::atomic<uint64_t> packetLost_;
    std::atomic<uint64_t> bytesReceived_;
    uint16_t lastSequenceNumber_;

    /**
     * @brief Receiver loop (runs in thread)
     * 
     * Algorithm:
     * while (running_) {
     *     1. Receive packet from UDP socket (timeout 5ms)
     *     2. Parse packet using RTPPacket
     *     3. Update statistics
     *     4. Call user callback
     * }
     */
    void receiveLoop();

    /**
     * @brief Update packet loss statistics
     * @param currentSeq Current sequence number
     */
    void updateStatistics(uint16_t currentSeq);

public:
    /** 
     * @brief Constructor
     * @param rtpPort Local UDP port for receiving (e.g., 25000)
     */
    explicit RTPReceiver(int rtpPort);

    /** 
     * @brief Constructor
     * @param rtpPort Local UDP port for receiving (e.g., 25000)
     * @param frameReassembler Pointer to frame reassembler (không own)
     */
    RTPReceiver(int rtpPort, FrameReassembler* frameReassembler);

    ~RTPReceiver();

    /**
     * @brief Start receiving packets
     * @param callback Function to call for each received packet
     * 
     * @example
     * receiver.start([](const RTPPacket& packet) {
     *     std::cout << "Received packet seq=" << packet.getSequenceNumber() << std::endl;
     *     // Pass to FrameReassembler
     * });
     */
    void start();

    /**
    * @brief Stop receiving packets
    */
    void stop();

    /**
    * @brief Check if receiver is running
    */
    bool isRunning() const {return running_;}

    uint64_t getPacketReceived() const {return packetReceived_;}
    uint64_t getPacketLost() const {return packetLost_;}
    uint64_t getBytesReceived() const {return bytesReceived_;}

    /**
    * @brief Get packet loss percentage
    * @return Packet loss percentage
    */
    double getPacketLossPercentage() const;
};
#endif