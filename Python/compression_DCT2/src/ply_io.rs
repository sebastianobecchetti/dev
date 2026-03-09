use std::collections::HashMap;
use std::fs::File;
use std::io::{BufRead, BufReader, BufWriter, Write};
use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};

/// Full PLY data: property names (in order) and SoA vertex data.
#[derive(Debug, Clone)]
pub struct PlyVertexData {
    pub property_names: Vec<String>,
    pub properties: HashMap<String, Vec<f64>>,
    pub count: usize,
}

/// Read a binary little-endian PLY file and extract all vertex properties as f64 in SoA format.
pub fn read_ply(path: &str) -> Result<PlyVertexData, String> {
    let file = File::open(path).map_err(|e| format!("Failed to open '{}': {}", path, e))?;
    let mut reader = BufReader::new(file);

    let mut line = String::new();
    reader.read_line(&mut line).map_err(|e| e.to_string())?;
    if line.trim() != "ply" {
        return Err("Not a PLY file".to_string());
    }

    let mut count = 0;
    let mut property_names = Vec::new();
    let mut property_types = Vec::new();
    let mut is_binary_le = false;

    loop {
        line.clear();
        reader.read_line(&mut line).map_err(|e| e.to_string())?;
        let tokens: Vec<&str> = line.trim().split_whitespace().collect();
        if tokens.is_empty() { continue; }

        match tokens[0] {
            "format" => {
                if tokens.get(1) == Some(&"binary_little_endian") {
                    is_binary_le = true;
                } else if tokens.get(1) == Some(&"ascii") {
                    return Err("ASCII PLY not supported, only binary_little_endian".to_string());
                } else {
                    return Err(format!("Unsupported format: {:?}", tokens));
                }
            }
            "element" => {
                if tokens.get(1) == Some(&"vertex") {
                    count = tokens[2].parse().unwrap_or(0);
                }
            }
            "property" => {
                if tokens.len() >= 3 && count > 0 {
                    property_types.push(tokens[1].to_string());
                    property_names.push(tokens[2].to_string());
                }
            }
            "end_header" => break,
            _ => {}
        }
    }

    if !is_binary_le {
        return Err("Not a valid binary_little_endian PLY file".to_string());
    }

    let mut properties: HashMap<String, Vec<f64>> = HashMap::new();
    for name in &property_names {
        properties.insert(name.clone(), Vec::with_capacity(count));
    }

    // Read binary data vertex by vertex
    for _ in 0..count {
        for (name, ptype) in property_names.iter().zip(property_types.iter()) {
            let val: f64 = match ptype.as_str() {
                "float" | "float32" => reader.read_f32::<LittleEndian>().map_err(|e| e.to_string())? as f64,
                "double" | "float64" => reader.read_f64::<LittleEndian>().map_err(|e| e.to_string())?,
                "uchar" | "uint8" => reader.read_u8().map_err(|e| e.to_string())? as f64,
                "char" | "int8" => reader.read_i8().map_err(|e| e.to_string())? as f64,
                "ushort" | "uint16" => reader.read_u16::<LittleEndian>().map_err(|e| e.to_string())? as f64,
                "short" | "int16" => reader.read_i16::<LittleEndian>().map_err(|e| e.to_string())? as f64,
                "uint" | "uint32" => reader.read_u32::<LittleEndian>().map_err(|e| e.to_string())? as f64,
                "int" | "int32" => reader.read_i32::<LittleEndian>().map_err(|e| e.to_string())? as f64,
                _ => return Err(format!("Unsupported property type {}", ptype)),
            };
            properties.get_mut(name).unwrap().push(val);
        }
    }

    Ok(PlyVertexData {
        property_names,
        properties,
        count,
    })
}

/// Write SoA vertex data to binary little endian PLY (always as float32)
pub fn write_ply(path: &str, property_names: &[String], ply_data: &PlyVertexData) -> Result<(), String> {
    let file = File::create(path).map_err(|e| format!("Failed to create '{}': {}", path, e))?;
    let mut writer = BufWriter::new(file);

    writeln!(writer, "ply").unwrap();
    writeln!(writer, "format binary_little_endian 1.0").unwrap();
    writeln!(writer, "element vertex {}", ply_data.count).unwrap();
    for name in property_names {
        writeln!(writer, "property float {}", name).unwrap();
    }
    writeln!(writer, "end_header").unwrap();

    for i in 0..ply_data.count {
        for name in property_names {
            let val = ply_data.properties.get(name).map(|v| v[i]).unwrap_or(0.0);
            writer.write_f32::<LittleEndian>(val as f32).unwrap();
        }
    }
    Ok(())
}

/// Get a single channel (property) as a Vec<f64>
pub fn extract_channel(ply_data: &PlyVertexData, name: &str) -> Vec<f64> {
    ply_data.properties.get(name).cloned().unwrap_or_else(|| vec![0.0; ply_data.count])
}

/// Reorder ALL properties by given index permutation.
pub fn reorder_vertices(ply_data: &PlyVertexData, indices: &[usize]) -> PlyVertexData {
    let count = indices.len();
    let mut new_props = HashMap::new();
    for name in &ply_data.property_names {
        if let Some(old_vec) = ply_data.properties.get(name) {
            let mut new_vec = Vec::with_capacity(count);
            for &idx in indices {
                new_vec.push(old_vec[idx]);
            }
            new_props.insert(name.clone(), new_vec);
        }
    }
    PlyVertexData {
        property_names: ply_data.property_names.clone(),
        properties: new_props,
        count,
    }
}

/// Set or update a property vector
pub fn set_channel(ply_data: &mut PlyVertexData, name: &str, values: Vec<f64>) {
    if !ply_data.property_names.contains(&name.to_string()) {
        ply_data.property_names.push(name.to_string());
    }
    ply_data.properties.insert(name.to_string(), values);
}
