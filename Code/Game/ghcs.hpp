#pragma once
#include "Game/RVSGame.hpp"

class buffer_reader;
class buffer_writer;
using byte = unsigned char;
using uint32 = unsigned int;

struct ghcs_header
{
	enum e_endianess : byte
	{
		little_endian = 1,
		big_endian = 2
	};
	byte major_version = 0;
	byte minor_version = 0;
	bool is_big_endian = false;
	unsigned int toc_offset = 0;
};

enum e_ghcs_chunk_type : byte
{
	ghcs_ConvexPolysChunk = 0x01,
	ghcs_ConvexHullsChunk = 0x02,
	ghcs_BSPTreeChunk=0x03,
	ghcs_SceneInfoChunk = 0x80,
	ghcs_Bounding_Disc_Chunk = 0x81,
	ghcs_BoundingAABBChunk = 0x82,
	ghcs_AABB2TreeChunk = 0x83,
	ghcs_OBB2TreeChunk = 0x84,
	ghcs_ConvexHullTreeChunk = 0x85,
	ghcs_AsymmetricQuadtreeChunk = 0x86,
	ghcs_SymmetricQuadtreeChunk = 0x87,
	ghcs_TiledBitRegionsChunk = 0x88,
	ghcs_ColumnRowBitRegionsChunk = 0x89,
	ghcs_Disc2TreeChunk = 0x8A,
	ghcs_Invalid = 0xFF,
};

struct ghcs_toc_chunk
{
	byte type = ghcs_Invalid;
	uint32 location = 0;
	uint32 size = 0;
};

ghcs_header parse_ghcs_header(buffer_reader& bufferReader);
std::vector<ghcs_toc_chunk> parse_ghcs_toc(buffer_reader& bufferReader);
std::vector<Zone> parse_convex_poly_chunk(buffer_reader& reader);


uint32 write_ghcs_header(buffer_writer& writer, ghcs_header* header);
uint32 write_convex_poly_chunk(buffer_writer& writer, std::vector<Zone>& zones);



