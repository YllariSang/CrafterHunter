package dev.crafterhunter.client;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.concurrent.atomic.AtomicBoolean;

final class BridgeClient implements AutoCloseable {
    private static final InetSocketAddress BRIDGE = new InetSocketAddress(
        InetAddress.getLoopbackAddress(),
        38470
    );
    private static final Duration HEARTBEAT_INTERVAL = Duration.ofSeconds(1);
    private static final Duration RETRY_INTERVAL = Duration.ofSeconds(1);

    private final AtomicBoolean running = new AtomicBoolean();
    private Thread thread;
    private volatile DatagramSocket socket;
    private int sequence;

    synchronized void start() {
        if (!running.compareAndSet(false, true)) {
            return;
        }
        thread = new Thread(this::run, "crafterhunter-bridge-client");
        thread.setDaemon(true);
        thread.start();
    }

    private void run() {
        try {
            while (running.get()) {
                try {
                    runSession();
                } catch (IOException error) {
                    CameraFeed.clear();
                    if (running.get()) {
                        System.err.println("[CrafterHunter] Bridge I/O failed; retrying: " + error);
                        waitBeforeRetry();
                    }
                }
            }
        } finally {
            socket = null;
            running.set(false);
            CameraFeed.clear();
        }
    }

    private void runSession() throws IOException {
        try (DatagramSocket activeSocket = new DatagramSocket()) {
            socket = activeSocket;
            activeSocket.connect(BRIDGE);
            activeSocket.setSoTimeout(100);
            sendHello(activeSocket);

            long nextKeepAlive = System.nanoTime() + HEARTBEAT_INTERVAL.toNanos();
            byte[] receiveBuffer = new byte[Protocol.HEADER_LENGTH + Protocol.MAX_PAYLOAD_LENGTH];
            while (running.get()) {
                DatagramPacket datagram = new DatagramPacket(receiveBuffer, receiveBuffer.length);
                try {
                    activeSocket.receive(datagram);
                    onPacket(Protocol.decode(datagram.getData(), datagram.getLength()));
                } catch (SocketTimeoutException ignored) {
                    // The timeout keeps shutdown and keepalive handling responsive.
                } catch (IllegalArgumentException error) {
                    System.err.println("[CrafterHunter] Discarded invalid packet: " + error.getMessage());
                }

                long now = System.nanoTime();
                if (now >= nextKeepAlive) {
                    send(activeSocket, Protocol.KIND_HEARTBEAT, new byte[0]);
                    nextKeepAlive = now + HEARTBEAT_INTERVAL.toNanos();
                }
            }
        } finally {
            socket = null;
        }
    }

    private void onPacket(Protocol.Packet packet) {
        if (packet.source() == Protocol.SOURCE_BRIDGE && packet.kind() == Protocol.KIND_HELLO_ACK) {
            System.out.println("[CrafterHunter] Connected to " + Protocol.payloadText(packet));
            return;
        }
        if (packet.source() == Protocol.SOURCE_MHW && packet.kind() == Protocol.KIND_CAMERA_STATE) {
            CameraFeed.publish(
                CameraState.decode(packet.payload(), packet.sequence(), System.nanoTime())
            );
        }
    }

    private void sendHello(DatagramSocket activeSocket) throws IOException {
        send(
            activeSocket,
            Protocol.KIND_HELLO,
            "crafterhunter-fabric/0.2.0".getBytes(StandardCharsets.UTF_8)
        );
    }

    private void send(DatagramSocket activeSocket, short kind, byte[] payload) throws IOException {
        byte[] bytes = Protocol.packet(
            kind,
            Protocol.SOURCE_MINECRAFT,
            ++sequence,
            payload
        );
        activeSocket.send(new DatagramPacket(bytes, bytes.length));
    }

    private void waitBeforeRetry() {
        try {
            Thread.sleep(RETRY_INTERVAL.toMillis());
        } catch (InterruptedException ignored) {
            Thread.currentThread().interrupt();
        }
    }

    @Override
    public synchronized void close() {
        running.set(false);
        CameraFeed.clear();
        if (socket != null) {
            socket.close();
        }
        if (thread != null) {
            thread.interrupt();
        }
    }
}
