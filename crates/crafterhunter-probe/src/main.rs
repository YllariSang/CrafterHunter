use crafterhunter_protocol::{
    CameraState, Kind, Packet, Source, DEFAULT_BRIDGE_ADDR, MAX_PACKET_LEN,
};
use std::io;
use std::net::UdpSocket;
use std::time::{Duration, Instant};

fn main() -> io::Result<()> {
    let role_argument = std::env::args().nth(1).unwrap_or_default();
    let source = match role_argument.as_str() {
        "mhw" => Source::Mhw,
        "minecraft" => Source::Minecraft,
        _ => {
            eprintln!("usage: crafterhunter-probe <mhw|minecraft> [bridge-address]");
            std::process::exit(2);
        }
    };
    let bridge = std::env::args()
        .nth(2)
        .unwrap_or_else(|| DEFAULT_BRIDGE_ADDR.to_owned());
    let socket = UdpSocket::bind("127.0.0.1:0")?;
    socket.connect(&bridge)?;
    socket.set_read_timeout(Some(Duration::from_millis(100)))?;

    let mut sequence = 1_u32;
    send(
        &socket,
        &Packet::new(
            Kind::Hello,
            source,
            sequence,
            format!("crafterhunter-probe/{role_argument}").into_bytes(),
        ),
    )?;
    println!(
        "{role_argument} probe connected from {} to {bridge}",
        socket.local_addr()?
    );

    let mut last_heartbeat = Instant::now();
    let mut last_camera = Instant::now();
    let mut last_camera_report = Instant::now() - Duration::from_secs(1);
    let mut buffer = [0_u8; MAX_PACKET_LEN];
    loop {
        match socket.recv(&mut buffer) {
            Ok(length) => match Packet::decode(&buffer[..length]) {
                Ok(packet) if packet.kind == Kind::CameraState => {
                    let camera = CameraState::decode(&packet.payload)
                        .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
                    if last_camera_report.elapsed() >= Duration::from_millis(250) {
                        println!(
                            "camera seq={} pos=({:.2}, {:.2}, {:.2})m rot=({:.3}, {:.3}, {:.3}, {:.3}) fov={:.1}deg aspect={:.3} clip={:.3}..{:.1}m",
                            packet.sequence,
                            camera.position[0],
                            camera.position[1],
                            camera.position[2],
                            camera.rotation[0],
                            camera.rotation[1],
                            camera.rotation[2],
                            camera.rotation[3],
                            camera.vertical_fov_radians.to_degrees(),
                            camera.aspect_ratio,
                            camera.near_plane_metres,
                            camera.far_plane_metres,
                        );
                        last_camera_report = Instant::now();
                    }
                }
                Ok(packet) => println!(
                    "received {:?} from {:?}, sequence {}, {} payload bytes",
                    packet.kind,
                    packet.source,
                    packet.sequence,
                    packet.payload.len()
                ),
                Err(error) => eprintln!("received invalid packet: {error}"),
            },
            Err(error)
                if matches!(
                    error.kind(),
                    io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut
                ) => {}
            Err(error) => return Err(error),
        }

        if last_heartbeat.elapsed() >= Duration::from_secs(1) {
            sequence = sequence.wrapping_add(1);
            send(
                &socket,
                &Packet::new(Kind::Heartbeat, source, sequence, Vec::new()),
            )?;
            last_heartbeat = Instant::now();
        }

        if source == Source::Mhw && last_camera.elapsed() >= Duration::from_millis(500) {
            sequence = sequence.wrapping_add(1);
            let camera = CameraState {
                position: [sequence as f32 / 10.0, 2.0, 3.0],
                rotation: [0.0, 0.0, 0.0, 1.0],
                vertical_fov_radians: std::f32::consts::FRAC_PI_3,
                aspect_ratio: 16.0 / 9.0,
                near_plane_metres: 0.05,
                far_plane_metres: 1000.0,
            };
            send(
                &socket,
                &Packet::new(
                    Kind::CameraState,
                    source,
                    sequence,
                    camera.encode().to_vec(),
                ),
            )?;
            last_camera = Instant::now();
        }
    }
}

fn send(socket: &UdpSocket, packet: &Packet) -> io::Result<()> {
    let bytes = packet
        .encode()
        .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
    let sent = socket.send(&bytes)?;
    if sent != bytes.len() {
        return Err(io::Error::new(
            io::ErrorKind::WriteZero,
            "UDP datagram was only partially sent",
        ));
    }
    Ok(())
}
