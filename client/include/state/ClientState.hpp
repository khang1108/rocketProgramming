#ifndef CLIENT_STATE_HPP
#define CLIENT_STATE_HPP

#include <memory>
#include <string>

class RTSPClient
{
    /**
    * @class ClientState
    * @brief Abstract base class for RTSP client state pattern
    * 
    * State Pattern:
    * - Each state handles commands differently
    * - State transitions managed by concrete states
    * - Prevents invalid operations (e.g., PLAY in INIT state)
    */
    class ClientState 
    {
        protected:
            RTSPClient* context_; ///< Reference to the RTSP client

        public:
            explicit ClientState(RTSPClient* context) : context_(context) {}
            virtual ~ClientState() = default;

            /**
            * @brief Handle SETUP command
            * @param videoFile Video filename
            * @param clientRTPPort RTP port
            * @return true if successful
            */
            virtual bool handleSetup(const std::string& videoFile, int clientRTPort){ return false;} ///< Default: not allowed

            /**
            * @brief Handle PLAY command
            * @return true if successful
            */
            virtual bool handlePlay(){ return false;} ///< Default: not allowed

            /**
            * @brief Handle PAUSE command
            * @return true if successful
            */
            virtual bool handlePause(){ return false;} ///< Default: not allowed

            /**
            * @brief Handle TEARDOWN command
            * @return true if successful
            */
            virtual bool handleTeardown(){ return false;} ///< Default: not allowed

            virtual std::string getStateName() const = 0;
    };

    /**
    * @class InitState
    * @brief Initial state - only SETUP allowed
    */
    class InitState : public ClientState
    {
        public:
            explicit InitState(RTSPClient* context) : ClientState(context) {}

            bool handleSetup(const std::string& videoFile, int clientRTPort) override;
            bool handleTearDown() override {return true;}

            std::string getStateName() const override {return "INIT";}
    };
    
    /**
    * @class ReadyState
    * @brief Ready state - PLAY, TEARDOWN allowed
    */
    class ReadyState : public ClientState
    {
        public:
            explicit ReadyState(RTSPClient *context) : ClientState(context) {}

            bool handlePlay() override;
            bool handleTeardown() override;

            std::string getStateName() const override {return "READY";}
    };

    /**
    * @class PlayingState
    * @brief Playing state - PAUSE, TEARDOWN allowed
    */
    class PlayingState : public ClientState
    {
        public:
            explicit PlayingState(RTSPClient *context) : ClientState(context) {}

            bool handlePause() override;
            bool handleTeardown() override;

            std::string getStateName() const override {return "PLAYING";}
    }
};
#endif