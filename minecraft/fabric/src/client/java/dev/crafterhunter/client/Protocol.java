package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

final class Protocol {
    static final byte[] MAGIC = {'C', 'H', 'N', 'T'};
    static final short VERSION = 1;
    static final int HEADER_LENGTH = 24;
    static final int MAX_PAYLOAD_LENGTH = 1200;
    static final byte SOURCE_BRIDGE = 1;
    static final byte SOURCE_MHW = 2;
    static final byte SOURCE_MINECRAFT = 3;
    static final short KIND_HELLO = 1;
    static final short KIND_HELLO_ACK = 2;
    static final short KIND_HEARTBEAT = 3;
    static final short KIND_CAMERA_STATE = 10;
    static final short KIND_PLAYER_STATE = 11;
    static final short KIND_BLOCK_PIXELS = 20;
    static final short KIND_BLOCK_PNG = 21;

    private Protocol() {
    }

    static byte[] packet(short kind, byte source, int sequence, byte[] payload) {
        if (payload.length > MAX_PAYLOAD_LENGTH) {
            throw new IllegalArgumentException("payload exceeds protocol maximum");
        }
        ByteBuffer buffer = ByteBuffer
            .allocate(HEADER_LENGTH + payload.length)
            .order(ByteOrder.LITTLE_ENDIAN);
        buffer.put(MAGIC);
        buffer.putShort(VERSION);
        buffer.putShort(kind);
        buffer.put(source);
        buffer.put(new byte[3]);
        buffer.putInt(sequence);
        buffer.putInt(payload.length);
        buffer.putInt(0);
        buffer.put(payload);
        return buffer.array();
    }

    static Packet decode(byte[] datagram, int length) {
        if (length < HEADER_LENGTH || length > HEADER_LENGTH + MAX_PAYLOAD_LENGTH) {
            throw new IllegalArgumentException("invalid packet length: " + length);
        }
        ByteBuffer buffer = ByteBuffer.wrap(datagram, 0, length).order(ByteOrder.LITTLE_ENDIAN);
        byte[] magic = new byte[4];
        buffer.get(magic);
        if (!Arrays.equals(magic, MAGIC)) {
            throw new IllegalArgumentException("invalid packet magic");
        }
        short version = buffer.getShort();
        if (version != VERSION) {
            throw new IllegalArgumentException("unsupported protocol version: " + version);
        }
        short kind = buffer.getShort();
        byte source = buffer.get();
        if (buffer.get() != 0 || buffer.get() != 0 || buffer.get() != 0) {
            throw new IllegalArgumentException("reserved header bits are set");
        }
        int sequence = buffer.getInt();
        int payloadLength = buffer.getInt();
        int session = buffer.getInt();
        if (payloadLength < 0 || payloadLength > MAX_PAYLOAD_LENGTH
            || length != HEADER_LENGTH + payloadLength) {
            throw new IllegalArgumentException("declared payload length does not match packet");
        }
        byte[] payload = new byte[payloadLength];
        buffer.get(payload);
        return new Packet(kind, source, sequence, session, payload);
    }

    static String payloadText(Packet packet) {
        return new String(packet.payload(), StandardCharsets.UTF_8);
    }

    record Packet(short kind, byte source, int sequence, int session, byte[] payload) {
    }
}
