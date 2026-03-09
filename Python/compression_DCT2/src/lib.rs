pub mod morton;
pub mod rle;
pub mod quantize;
pub mod dct_utils;
pub mod ply_io;

pub mod bek_format {
    include!(concat!(env!("OUT_DIR"), "/bek_format.rs"));
}

pub const MAGIC_WORD: &[u8] = b"BEK3DGS";
pub const VERSION: u32 = 1;
