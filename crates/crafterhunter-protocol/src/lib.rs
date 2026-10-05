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
    PlayerState = 11,
    TerrainRequest = 12,
    TerrainResult = 13,
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
            11 => Ok(Self::PlayerState),
            12 => Ok(Self::TerrainRequest),
            13 => Ok(Self::TerrainResult),
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

/// The host hunter: world position in metres and the model rotation quaternion.
///
/// MHW world coordinates sit far from Minecraft's origin, so a consumer maps
/// them by relative displacement from an anchor rather than absolutely; the
/// payload carries raw host coordinates for the same reason `CameraState`
/// does.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct PlayerState {
    pub position: [f32; 3],
    pub rotation: [f32; 4],
}

impl PlayerState {
    pub const PAYLOAD_LEN: usize = 28;

    pub fn encode(self) -> [u8; Self::PAYLOAD_LEN] {
        let values = [
            self.position[0],
            self.position[1],
            self.position[2],
            self.rotation[0],
            self.rotation[1],
            self.rotation[2],
            self.rotation[3],
        ];
        let mut output = [0_u8; Self::PAYLOAD_LEN];
        for (index, value) in values.into_iter().enumerate() {
            output[index * 4..index * 4 + 4].copy_from_slice(&value.to_le_bytes());
        }
        output
    }

    pub fn decode(payload: &[u8]) -> Result<Self, ProtocolError> {
        if payload.len() != Self::PAYLOAD_LEN {
            return Err(ProtocolError::InvalidPlayerLength(payload.len()));
        }
        let mut values = [0_f32; 7];
        for (index, value) in values.iter_mut().enumerate() {
            let offset = index * 4;
            *value = f32::from_le_bytes(payload[offset..offset + 4].try_into().unwrap());
        }
        if values.iter().any(|value| !value.is_finite()) {
            return Err(ProtocolError::NonFinitePlayerValue);
        }
        Ok(Self {
            position: [values[0], values[1], values[2]],
            rotation: [values[3], values[4], values[5], values[6]],
        })
    }
}

/// Why a terrain request could not be answered.
///
/// `NoTerrain` is a state, not a failure of the request: the adapter is
/// disabled by the fingerprint, or the collision singleton and the hunter are
/// missing, which is what a loading screen and an area transition look like.
/// `Miss` is a real answer — the segment did not touch a surface — and must not
/// be confused with having no answer, or a guest would hold the last hit
/// through a wall it cannot see.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u8)]
pub enum TerrainStatus {
    NoTerrain = 0,
    Miss = 1,
    Hit = 2,
}

impl TryFrom<u8> for TerrainStatus {
    type Error = ProtocolError;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::NoTerrain),
            1 => Ok(Self::Miss),
            2 => Ok(Self::Hit),
            _ => Err(ProtocolError::UnknownTerrainStatus(value)),
        }
    }
}

/// A segment the guest wants cast against stage collision.
///
/// Endpoints are metres in host world coordinates. The guest cannot ask in
/// Minecraft coordinates: the host does not know where the guest anchored, so
/// the guest maps through the same proxy anchor it already uses for the player,
/// and the answer comes back in the same space it was asked in.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct TerrainRequest {
    pub id: u32,
    pub start: [f32; 3],
    pub end: [f32; 3],
}

impl TerrainRequest {
    pub const PAYLOAD_LEN: usize = 28;

    pub fn encode(self) -> [u8; Self::PAYLOAD_LEN] {
        let mut output = [0_u8; Self::PAYLOAD_LEN];
        output[0..4].copy_from_slice(&self.id.to_le_bytes());
        for (index, value) in self.start.into_iter().chain(self.end).enumerate() {
            let offset = 4 + index * 4;
            output[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        output
    }

    pub fn decode(payload: &[u8]) -> Result<Self, ProtocolError> {
        if payload.len() != Self::PAYLOAD_LEN {
            return Err(ProtocolError::InvalidTerrainRequestLength(payload.len()));
        }
        let id = read_u32(payload, 0);
        let mut values = [0_f32; 6];
        for (index, value) in values.iter_mut().enumerate() {
            let offset = 4 + index * 4;
            *value = f32::from_le_bytes(payload[offset..offset + 4].try_into().unwrap());
        }
        if values.iter().any(|value| !value.is_finite()) {
            return Err(ProtocolError::NonFiniteTerrainValue);
        }
        Ok(Self {
            id,
            start: [values[0], values[1], values[2]],
            end: [values[3], values[4], values[5]],
        })
    }
}

/// The answer to one `TerrainRequest`, echoing its `id`.
///
/// Position and normal are metres in host world coordinates and are zeroed
/// unless the status is `Hit`, so a decoded miss cannot be mistaken for a hit at
/// the origin. The attribute is the game's own surface attribute, untranslated.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct TerrainResult {
    pub id: u32,
    pub status: TerrainStatus,
    pub position: [f32; 3],
    pub normal: [f32; 3],
    pub attribute: u32,
}

impl TerrainResult {
    pub const PAYLOAD_LEN: usize = 36;

    pub fn encode(self) -> [u8; Self::PAYLOAD_LEN] {
        let mut output = [0_u8; Self::PAYLOAD_LEN];
        output[0..4].copy_from_slice(&self.id.to_le_bytes());
        output[4] = self.status as u8;
        // Bytes 5..8 stay zero, exactly like the header's reserved bytes.
        for (index, value) in self.position.into_iter().chain(self.normal).enumerate() {
            let offset = 8 + index * 4;
            output[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        output[32..36].copy_from_slice(&self.attribute.to_le_bytes());
        output
    }

    pub fn decode(payload: &[u8]) -> Result<Self, ProtocolError> {
        if payload.len() != Self::PAYLOAD_LEN {
            return Err(ProtocolError::InvalidTerrainResultLength(payload.len()));
        }
        let id = read_u32(payload, 0);
        let status = TerrainStatus::try_from(payload[4])?;
        if payload[5..8] != [0, 0, 0] {
            return Err(ProtocolError::ReservedTerrainBitsSet);
        }
        let mut values = [0_f32; 6];
        for (index, value) in values.iter_mut().enumerate() {
            let offset = 8 + index * 4;
            *value = f32::from_le_bytes(payload[offset..offset + 4].try_into().unwrap());
        }
        if values.iter().any(|value| !value.is_finite()) {
            return Err(ProtocolError::NonFiniteTerrainValue);
        }
        Ok(Self {
            id,
            status,
            position: [values[0], values[1], values[2]],
            normal: [values[3], values[4], values[5]],
            attribute: read_u32(payload, 32),
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
    InvalidPlayerLength(usize),
    NonFinitePlayerValue,
    InvalidTerrainRequestLength(usize),
    InvalidTerrainResultLength(usize),
    UnknownTerrainStatus(u8),
    ReservedTerrainBitsSet,
    NonFiniteTerrainValue,
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

    #[test]
    fn player_round_trip() {
        let player = PlayerState {
            position: [-270.25, -455.0, -116.875],
            rotation: [0.0, 0.5, 0.0, 0.8660254],
        };
        assert_eq!(PlayerState::decode(&player.encode()).unwrap(), player);
    }

    #[test]
    fn player_state_matches_cross_language_golden_bytes() {
        // The plugin writes these bytes in C# and the guest reads them in Java;
        // this pins the layout both sides must agree on.
        let player = PlayerState {
            position: [1.0, -2.0, 3.5],
            rotation: [0.0, 0.0, 0.0, 1.0],
        };
        assert_eq!(
            player.encode().to_vec(),
            vec![
                0x00, 0x00, 0x80, 0x3F, // 1.0
                0x00, 0x00, 0x00, 0xC0, // -2.0
                0x00, 0x00, 0x60, 0x40, // 3.5
                0x00, 0x00, 0x00, 0x00, // quaternion x
                0x00, 0x00, 0x00, 0x00, // quaternion y
                0x00, 0x00, 0x00, 0x00, // quaternion z
                0x00, 0x00, 0x80, 0x3F, // quaternion w
            ]
        );
    }

    #[test]
    fn rejects_wrong_player_payload_length() {
        assert_eq!(
            PlayerState::decode(&[0_u8; 24]),
            Err(ProtocolError::InvalidPlayerLength(24))
        );
    }

    #[test]
    fn rejects_non_finite_player_value() {
        let mut payload = [0_u8; PlayerState::PAYLOAD_LEN];
        payload[4..8].copy_from_slice(&f32::INFINITY.to_le_bytes());
        assert_eq!(
            PlayerState::decode(&payload),
            Err(ProtocolError::NonFinitePlayerValue)
        );
    }

    #[test]
    fn terrain_kinds_are_twelve_and_thirteen() {
        assert_eq!(Kind::try_from(11).unwrap(), Kind::PlayerState);
        assert_eq!(Kind::try_from(12).unwrap(), Kind::TerrainRequest);
        assert_eq!(Kind::try_from(13).unwrap(), Kind::TerrainResult);
        assert_eq!(
            Kind::try_from(14),
            Err(ProtocolError::UnknownKind(14)),
            "kinds 12 and 13 are taken; 14 stays unassigned"
        );
    }

    #[test]
    fn terrain_request_round_trip() {
        let request = TerrainRequest {
            id: 7,
            start: [1.0, -2.0, 3.5],
            end: [-4.0, 5.0, 6.5],
        };
        assert_eq!(TerrainRequest::decode(&request.encode()).unwrap(), request);
    }

    #[test]
    fn terrain_request_matches_cross_language_golden_bytes() {
        // The plugin decodes these bytes in C# and the guest writes them in
        // Java; this pins the layout both sides must agree on.
        let request = TerrainRequest {
            id: 1,
            start: [1.0, 2.0, 3.5],
            end: [-4.0, 5.0, 6.5],
        };
        assert_eq!(
            request.encode().to_vec(),
            vec![
                0x01, 0x00, 0x00, 0x00, // id
                0x00, 0x00, 0x80, 0x3F, // start.x 1.0
                0x00, 0x00, 0x00, 0x40, // start.y 2.0
                0x00, 0x00, 0x60, 0x40, // start.z 3.5
                0x00, 0x00, 0x80, 0xC0, // end.x -4.0
                0x00, 0x00, 0xA0, 0x40, // end.y 5.0
                0x00, 0x00, 0xD0, 0x40, // end.z 6.5
            ]
        );
    }

    #[test]
    fn terrain_result_round_trip() {
        let result = TerrainResult {
            id: 7,
            status: TerrainStatus::Hit,
            position: [1.0, -2.0, 3.5],
            normal: [0.0, 1.0, 0.0],
            attribute: 0x0010_0000,
        };
        assert_eq!(TerrainResult::decode(&result.encode()).unwrap(), result);
    }

    #[test]
    fn terrain_result_matches_cross_language_golden_bytes() {
        let result = TerrainResult {
            id: 7,
            status: TerrainStatus::Hit,
            position: [1.0, -2.0, 3.5],
            normal: [0.0, 1.0, 0.0],
            attribute: 0x0010_0000,
        };
        assert_eq!(
            result.encode().to_vec(),
            vec![
                0x07, 0x00, 0x00, 0x00, // id
                0x02, 0x00, 0x00, 0x00, // status Hit, then three reserved zeros
                0x00, 0x00, 0x80, 0x3F, // position.x 1.0
                0x00, 0x00, 0x00, 0xC0, // position.y -2.0
                0x00, 0x00, 0x60, 0x40, // position.z 3.5
                0x00, 0x00, 0x00, 0x00, // normal.x 0.0
                0x00, 0x00, 0x80, 0x3F, // normal.y 1.0
                0x00, 0x00, 0x00, 0x00, // normal.z 0.0
                0x00, 0x00, 0x10, 0x00, // attribute 0x00100000
            ]
        );
    }

    #[test]
    fn a_miss_carries_no_position() {
        let result = TerrainResult {
            id: 3,
            status: TerrainStatus::Miss,
            position: [0.0; 3],
            normal: [0.0; 3],
            attribute: 0,
        };
        let decoded = TerrainResult::decode(&result.encode()).unwrap();
        assert_eq!(decoded.status, TerrainStatus::Miss);
        assert_eq!(decoded.position, [0.0; 3], "a miss must not look like a hit at the origin");
    }

    #[test]
    fn rejects_wrong_terrain_payload_lengths() {
        assert_eq!(
            TerrainRequest::decode(&[0_u8; 24]),
            Err(ProtocolError::InvalidTerrainRequestLength(24))
        );
        assert_eq!(
            TerrainResult::decode(&[0_u8; 28]),
            Err(ProtocolError::InvalidTerrainResultLength(28))
        );
    }

    #[test]
    fn rejects_non_finite_terrain_value() {
        let mut request = [0_u8; TerrainRequest::PAYLOAD_LEN];
        request[4..8].copy_from_slice(&f32::NAN.to_le_bytes());
        assert_eq!(
            TerrainRequest::decode(&request),
            Err(ProtocolError::NonFiniteTerrainValue)
        );
        let mut result = [0_u8; TerrainResult::PAYLOAD_LEN];
        result[8..12].copy_from_slice(&f32::INFINITY.to_le_bytes());
        assert_eq!(
            TerrainResult::decode(&result),
            Err(ProtocolError::NonFiniteTerrainValue)
        );
    }

    #[test]
    fn rejects_unknown_terrain_status() {
        let mut payload = [0_u8; TerrainResult::PAYLOAD_LEN];
        payload[4] = 9;
        assert_eq!(
            TerrainResult::decode(&payload),
            Err(ProtocolError::UnknownTerrainStatus(9))
        );
    }

    #[test]
    fn rejects_reserved_terrain_bits() {
        let mut payload = [0_u8; TerrainResult::PAYLOAD_LEN];
        payload[7] = 1;
        assert_eq!(
            TerrainResult::decode(&payload),
            Err(ProtocolError::ReservedTerrainBitsSet)
        );
    }
}
