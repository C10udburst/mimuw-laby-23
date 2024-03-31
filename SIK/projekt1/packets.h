#ifndef PROJEKT1_PACKETS_H
#define PROJEKT1_PACKETS_H

#include <cstdint>
#include <cstdlib>
#include <string>

namespace PPCB {

    // region Packet
    enum class PacketType : char {
        CONN = 1,
        CONNACC = 2,
        CONRJT = 3,
        DATA = 4,
        ACC = 5,
        RJT = 6,
        RCVD = 7
    };
    static_assert(sizeof(PacketType) == 1, "PacketType must be 1 byte long");

    class Packet {
    public:
        PacketType type;
        uint64_t session_id;

        Packet(PacketType type, uint64_t session_id) : type(type), session_id(session_id) {}

        virtual ~Packet() = default;

        void *serialize();

    protected:
        virtual void serialize_inner(void *buffer);

        virtual size_t packet_size();
    };
    Packet *deserialize(void *buffer);
    // endregion

    // region ConnPacket
    enum class ConnType: char {
        TCP = 1,
        UDP = 2,
        UDPR = 3
    };
    static_assert(sizeof(ConnType) == 1, "ConnType must be 1 byte long");

    class ConnPacket : public Packet {
    public:
        ConnType conn_type;
        uint64_t length;

        ConnPacket(PacketType type, uint64_t session_id, ConnType conn_type, uint64_t length) : Packet(type, session_id), conn_type(conn_type), length(length) {}

    protected:
        void serialize_inner(void *buffer) override;

        size_t packet_size() override;
    };
    ConnPacket from_string(const std::string &str, uint64_t session_id, uint64_t length);
    // endregion

    // region DataPacket
    class DataPacket : public Packet {
    public:
        uint64_t seq_num;
        uint32_t data_length;
        char *data;

        DataPacket(PacketType type, uint64_t session_id, uint64_t seq_num, uint32_t data_length, char *data) : Packet(type, session_id), seq_num(seq_num), data_length(data_length), data(data) {}

        ~DataPacket() override {
            free(data);
        }

    protected:
        void serialize_inner(void *buffer) override;
        size_t packet_size() override;
    };
}


#endif //PROJEKT1_PACKETS_H
