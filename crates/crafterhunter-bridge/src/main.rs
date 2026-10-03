use crafterhunter_protocol::{Kind, Packet, Source, DEFAULT_BRIDGE_ADDR, MAX_PACKET_LEN};
use std::collections::HashMap;
use std::io;
use std::net::{SocketAddr, UdpSocket};
use std::time::{Duration, Instant};

const ENDPOINT_TIMEOUT: Duration = Duration::from_secs(5);
const READ_TIMEOUT: Duration = Duration::from_millis(250);

#[derive(Clone, Copy, Debug)]
struct Endpoint {
    address: SocketAddr,
    last_seen: Instant,
}

fn main() -> io::Result<()> {
    let bind_address = std::env::args()
        .nth(1)
        .unwrap_or_else(|| DEFAULT_BRIDGE_ADDR.to_owned());
    let socket = UdpSocket::bind(&bind_address)?;
    socket.set_read_timeout(Some(READ_TIMEOUT))?;
    println!(
        "CrafterHunter bridge v{} listening on {bind_address}",
        env!("CARGO_PKG_VERSION")
    );

    let mut endpoints: HashMap<Source, Endpoint> = HashMap::new();
    let mut response_sequence = 0_u32;
    let mut buffer = [0_u8; MAX_PACKET_LEN];

    loop {
        match socket.recv_from(&mut buffer) {
            Ok((length, sender)) => {
                let packet = match Packet::decode(&buffer[..length]) {
                    Ok(packet) => packet,
                    Err(error) => {
                        eprintln!("discarded invalid packet from {sender}: {error}");
                        continue;
                    }
                };
                if !matches!(packet.source, Source::Mhw | Source::Minecraft) {
                    eprintln!("discarded packet with non-game source {:?}", packet.source);
                    continue;
                }

                let previous = endpoints.insert(
                    packet.source,
                    Endpoint {
                        address: sender,
                        last_seen: Instant::now(),
                    },
                );
                if previous.map(|endpoint| endpoint.address) != Some(sender) {
                    println!("registered {:?} at {sender}", packet.source);
                }

                if packet.kind == Kind::Hello {
                    response_sequence = response_sequence.wrapping_add(1);
                    let ack = Packet::new(
                        Kind::HelloAck,
                        Source::Bridge,
                        response_sequence,
                        b"crafterhunter-bridge/v1".to_vec(),
                    );
                    send_packet(&socket, sender, &ack)?;
                }

                if let Some(destination) = peer_for(packet.source, &endpoints) {
                    send_bytes(&socket, destination.address, &buffer[..length])?;
                }
            }
            Err(error)
                if matches!(
                    error.kind(),
                    io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut
                ) => {}
            Err(error) => return Err(error),
        }

        let now = Instant::now();
        endpoints.retain(|source, endpoint| {
            let active = now.duration_since(endpoint.last_seen) <= ENDPOINT_TIMEOUT;
            if !active {
                println!("expired {:?} at {}", source, endpoint.address);
            }
            active
        });
    }
}

fn peer_for(source: Source, endpoints: &HashMap<Source, Endpoint>) -> Option<Endpoint> {
    match source {
        Source::Mhw => endpoints.get(&Source::Minecraft).copied(),
        Source::Minecraft => endpoints.get(&Source::Mhw).copied(),
        _ => None,
    }
}

fn send_packet(socket: &UdpSocket, destination: SocketAddr, packet: &Packet) -> io::Result<()> {
    let bytes = packet
        .encode()
        .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
    send_bytes(socket, destination, &bytes)
}

fn send_bytes(socket: &UdpSocket, destination: SocketAddr, bytes: &[u8]) -> io::Result<()> {
    let sent = socket.send_to(bytes, destination)?;
    if sent != bytes.len() {
        return Err(io::Error::new(
            io::ErrorKind::WriteZero,
            "UDP datagram was only partially sent",
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn endpoint(address: &str) -> Endpoint {
        Endpoint {
            address: address.parse().unwrap(),
            last_seen: Instant::now(),
        }
    }

    #[test]
    fn routes_each_game_to_the_other() {
        let mhw = endpoint("127.0.0.1:40001");
        let minecraft = endpoint("127.0.0.1:40002");
        let endpoints = HashMap::from([(Source::Mhw, mhw), (Source::Minecraft, minecraft)]);

        assert_eq!(
            peer_for(Source::Mhw, &endpoints).unwrap().address,
            minecraft.address
        );
        assert_eq!(
            peer_for(Source::Minecraft, &endpoints).unwrap().address,
            mhw.address
        );
        assert!(peer_for(Source::Bridge, &endpoints).is_none());
    }

    #[test]
    fn does_not_route_until_the_peer_exists() {
        let endpoints = HashMap::from([(Source::Mhw, endpoint("127.0.0.1:40001"))]);
        assert!(peer_for(Source::Mhw, &endpoints).is_none());
    }
}
