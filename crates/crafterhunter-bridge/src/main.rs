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
            Err(error) if is_transient(&error) => {}
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

/// A read can come back interrupted by a signal, or empty because the 250 ms
/// timeout fired; neither means the bridge is broken, so the loop tries again.
/// Every other error is returned so the process stops loudly instead of
/// silently going quiet on a live game.
fn is_transient(error: &io::Error) -> bool {
    matches!(
        error.kind(),
        io::ErrorKind::Interrupted | io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut
    )
}

fn send_packet(socket: &UdpSocket, destination: SocketAddr, packet: &Packet) -> io::Result<()> {
    let bytes = packet
        .encode()
        .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
    send_bytes(socket, destination, &bytes)
}

fn send_bytes(socket: &UdpSocket, destination: SocketAddr, bytes: &[u8]) -> io::Result<()> {
    send_with_retries(|| socket.send_to(bytes, destination), bytes.len())
}

/// A signal landing on a datagram send must not take the bridge down either:
/// retry that one interruption, and keep every other failure fatal.
fn send_with_retries(
    mut send: impl FnMut() -> io::Result<usize>,
    length: usize,
) -> io::Result<()> {
    loop {
        match send() {
            Ok(sent) if sent == length => return Ok(()),
            Ok(_) => {
                return Err(io::Error::new(
                    io::ErrorKind::WriteZero,
                    "UDP datagram was only partially sent",
                ))
            }
            Err(error) if error.kind() == io::ErrorKind::Interrupted => continue,
            Err(error) => return Err(error),
        }
    }
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

    #[test]
    fn an_interrupted_read_is_retried_not_fatal() {
        // 2026-10-05: a signal interrupted recv_from and the bridge exited with
        // `Os { code: 4, kind: Interrupted }` instead of trying again.
        for kind in [
            io::ErrorKind::Interrupted,
            io::ErrorKind::WouldBlock,
            io::ErrorKind::TimedOut,
        ] {
            assert!(
                is_transient(&io::Error::from(kind)),
                "{kind:?} must be retried"
            );
        }
    }

    #[test]
    fn a_real_read_failure_stops_the_bridge() {
        for kind in [io::ErrorKind::BrokenPipe, io::ErrorKind::PermissionDenied] {
            assert!(
                !is_transient(&io::Error::from(kind)),
                "{kind:?} must stay fatal so the bridge fails loudly"
            );
        }
    }

    #[test]
    fn an_interrupted_send_is_retried_until_it_lands() {
        let mut attempts = 0;
        let result = send_with_retries(
            || {
                attempts += 1;
                if attempts < 3 {
                    Err(io::Error::from(io::ErrorKind::Interrupted))
                } else {
                    Ok(4)
                }
            },
            4,
        );

        assert!(result.is_ok());
        assert_eq!(attempts, 3);
    }

    #[test]
    fn a_real_send_failure_is_returned_immediately() {
        let mut attempts = 0;
        let result: io::Result<()> = send_with_retries(
            || {
                attempts += 1;
                Err(io::Error::from(io::ErrorKind::ConnectionRefused))
            },
            4,
        );

        assert_eq!(
            result.unwrap_err().kind(),
            io::ErrorKind::ConnectionRefused
        );
        assert_eq!(attempts, 1);
    }

    #[test]
    fn a_truncated_datagram_is_an_error() {
        let result: io::Result<()> = send_with_retries(|| Ok(3), 4);
        assert_eq!(result.unwrap_err().kind(), io::ErrorKind::WriteZero);
    }
}
