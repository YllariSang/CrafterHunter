use std::fmt;

pub const MAGIC: [u8; 4] = *b"CHNT";
pub const VERSION: u16 = 1;
pub const HEADER_LEN: usize = 24;
pub const MAX_PAYLOAD_LEN: usize = 1200;
pub const MAX_PACKET_LEN: usize = HEADER_LEN + MAX_PAYLOAD_LEN;
pub const DEFAULT_BRIDGE_ADDR: &str = "127.0.0.1:38470";

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[repr(u8)]
pub enum Source {
    Unknown = 0,
    Bridge = 1,
    Mhw = 2,
    Minecraft = 3,
    Probe = 255,
}

impl TryFrom<u8> for Source {
    type Error = ProtocolError;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::Bridge),
            2 => Ok(Self::Mhw),
            3 => Ok(Self::Minecraft),
            255 => Ok(Self::Probe),
            _ => Err(ProtocolError::UnknownSource(value)),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u16)]
pub enum Kind {
    Hello = 1,
    HelloAck = 2,
    Heartbeat = 3,
    CameraState = 10,
    BlockPixels = 20,
    BlockPng = 21,
}

impl TryFrom<u16> for Kind {
    type Error = ProtocolError;

    fn try_from(value: u16) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Hello),
            2 => Ok(Self::HelloAck),
            3 => Ok(Self::Heartbeat),
            10 => Ok(Self::CameraState),
            20 => Ok(Self::BlockPixels),
            21 => Ok(Self::BlockPng),
            _ => Err(ProtocolError::UnknownKind(value)),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct Packet {
    pub kind: Kind,
    pub source: Source,
    pub sequence: u32,
    pub session: u32,
    pub payload: Vec<u8>,
}

impl Packet {
    pub fn new(kind: Kind, source: Source, sequence: u32, payload: Vec<u8>) -> Self {
        Self {
            kind,
            source,
            sequence,
            session: 0,
            payload,
        }
    }

    pub fn encode(&self) -> Result<Vec<u8>, ProtocolError> {
        if self.payload.len() > MAX_PAYLOAD_LEN {
            return Err(ProtocolError::PayloadTooLarge(self.payload.len()));
        }

        let mut bytes = Vec::with_capacity(HEADER_LEN + self.payload.len());
        bytes.extend_from_slice(&MAGIC);
        bytes.extend_from_slice(&VERSION.to_le_bytes());
        bytes.extend_from_slice(&(self.kind as u16).to_le_bytes());
        bytes.push(self.source as u8);
        bytes.extend_from_slice(&[0, 0, 0]);
        bytes.extend_from_slice(&self.sequence.to_le_bytes());
        bytes.extend_from_slice(&(self.payload.len() as u32).to_le_bytes());
        bytes.extend_from_slice(&self.session.to_le_bytes());
        bytes.extend_from_slice(&self.payload);
        Ok(bytes)
    }

    pub fn decode(bytes: &[u8]) -> Result<Self, ProtocolError> {
        if bytes.len() < HEADER_LEN {
            return Err(ProtocolError::PacketTooShort(bytes.len()));
        }
        if bytes[0..4] != MAGIC {
            return Err(ProtocolError::BadMagic);
        }

        let version = read_u16(bytes, 4);
        if version != VERSION {
            return Err(ProtocolError::UnsupportedVersion(version));
        }
        if bytes[9..12] != [0, 0, 0] {
            return Err(ProtocolError::ReservedBitsSet);
        }

        let kind = Kind::try_from(read_u16(bytes, 6))?;
        let source = Source::try_from(bytes[8])?;
        let sequence = read_u32(bytes, 12);
        let payload_len = read_u32(bytes, 16) as usize;
        let session = read_u32(bytes, 20);

        if payload_len > MAX_PAYLOAD_LEN {
            return Err(ProtocolError::PayloadTooLarge(payload_len));
        }
        if bytes.len() != HEADER_LEN + payload_len {
            return Err(ProtocolError::LengthMismatch {
                declared: payload_len,
                actual: bytes.len() - HEADER_LEN,
            });
        }

        Ok(Self {
            kind,
            source,
            sequence,
            session,
            payload: bytes[HEADER_LEN..].to_vec(),
        })
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CameraState {
    pub position: [f32; 3],
    pub rotation: [f32; 4],
    pub vertical_fov_radians: f32,
    pub aspect_ratio: f32,
    pub near_plane_metres: f32,
    pub far_plane_metres: f32,
}

impl CameraState {
    pub const PAYLOAD_LEN: usize = 44;

    pub fn encode(self) -> [u8; Self::PAYLOAD_LEN] {
        let values = [
            self.position[0],
            self.position[1],
            self.position[2],
            self.rotation[0],
            self.rotation[1],
            self.rotation[2],
            self.rotation[3],
            self.vertical_fov_radians,
            self.aspect_ratio,
            self.near_plane_metres,
            self.far_plane_metres,
        ];
        let mut output = [0_u8; Self::PAYLOAD_LEN];
        for (index, value) in values.into_iter().enumerate() {
            output[index * 4..index * 4 + 4].copy_from_slice(&value.to_le_bytes());
        }
        output
    }

    pub fn decode(payload: &[u8]) -> Result<Self, ProtocolError> {
        if payload.len() != Self::PAYLOAD_LEN {
            return Err(ProtocolError::InvalidCameraLength(payload.len()));
        }
        let mut values = [0_f32; 11];
        for (index, value) in values.iter_mut().enumerate() {
            let offset = index * 4;
            *value = f32::from_le_bytes(payload[offset..offset + 4].try_into().unwrap());
        }
        if values.iter().any(|value| !value.is_finite()) {
            return Err(ProtocolError::NonFiniteCameraValue);
        }
        Ok(Self {
            position: [values[0], values[1], values[2]],
            rotation: [values[3], values[4], values[5], values[6]],
            vertical_fov_radians: values[7],
            aspect_ratio: values[8],
            near_plane_metres: values[9],
            far_plane_metres: values[10],
        })
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum ProtocolError {
    PacketTooShort(usize),
    BadMagic,
    UnsupportedVersion(u16),
    ReservedBitsSet,
    UnknownKind(u16),
    UnknownSource(u8),
    PayloadTooLarge(usize),
    LengthMismatch { declared: usize, actual: usize },
    InvalidCameraLength(usize),
    NonFiniteCameraValue,
}

impl fmt::Display for ProtocolError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(formatter, "{self:?}")
    }
}

impl std::error::Error for ProtocolError {}

fn read_u16(bytes: &[u8], offset: usize) -> u16 {
    u16::from_le_bytes(bytes[offset..offset + 2].try_into().unwrap())
}

fn read_u32(bytes: &[u8], offset: usize) -> u32 {
    u32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn packet_round_trip() {
        let packet = Packet::new(Kind::Hello, Source::Mhw, 42, b"mhw-test".to_vec());
        let encoded = packet.encode().unwrap();
        assert_eq!(Packet::decode(&encoded).unwrap(), packet);
    }

    #[test]
    fn heartbeat_matches_cross_language_golden_bytes() {
        let packet = Packet::new(Kind::Heartbeat, Source::Mhw, 0x0102_0304, Vec::new());
        assert_eq!(
            packet.encode().unwrap(),
            vec![
                b'C', b'H', b'N', b'T', 1, 0, 3, 0, 2, 0, 0, 0, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
            ]
        );
    }

    #[test]
    fn rejects_declared_length_mismatch() {
        let packet = Packet::new(Kind::Heartbeat, Source::Minecraft, 1, Vec::new());
        let mut encoded = packet.encode().unwrap();
        encoded[16..20].copy_from_slice(&1_u32.to_le_bytes());
        assert!(matches!(
            Packet::decode(&encoded),
            Err(ProtocolError::LengthMismatch { .. })
        ));
    }

    #[test]
    fn camera_round_trip() {
        let camera = CameraState {
            position: [1.0, 2.0, 3.0],
            rotation: [0.0, 0.0, 0.0, 1.0],
            vertical_fov_radians: 1.2,
            aspect_ratio: 16.0 / 9.0,
            near_plane_metres: 0.05,
            far_plane_metres: 1000.0,
        };
        assert_eq!(CameraState::decode(&camera.encode()).unwrap(), camera);
    }

    #[test]
    fn rejects_non_finite_camera_value() {
        let mut payload = [0_u8; CameraState::PAYLOAD_LEN];
        payload[0..4].copy_from_slice(&f32::NAN.to_le_bytes());
        assert_eq!(
            CameraState::decode(&payload),
            Err(ProtocolError::NonFiniteCameraValue)
        );
    }
}
