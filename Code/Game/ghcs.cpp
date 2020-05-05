#pragma once
#include "Game/ghcs.hpp"
#include "Engine/Core/StringUtils.hpp"

ghcs_header parse_ghcs_header(buffer_reader& bufferReader)
{
	ghcs_header r;
	char fcc[4];
	bufferReader.next_n_byte((byte*)fcc, 4);
	if (fcc[0] != 'G' || fcc[1] != 'H' || fcc[2] != 'C' || fcc[3] != 'S') {
		ERROR_RECOVERABLE("Not a valid GHCS file, error found in header");
		return r;
	}
	bufferReader.next_basic<byte>();
	r.major_version = bufferReader.next_basic<byte>();
	r.minor_version = bufferReader.next_basic<byte>();
	byte endian = bufferReader.next_basic<byte>();
	if (endian == 2u) {
		r.is_big_endian = true;
	}
	r.toc_offset = bufferReader.next_basic<unsigned int>();
	return r;
}

std::vector<ghcs_toc_chunk> parse_ghcs_toc(buffer_reader& bufferReader)
{
	std::vector<ghcs_toc_chunk> r;
	char fcc[4];
	bufferReader.next_n_byte((byte*)fcc, 4);
	if (fcc[0] != '0' || fcc[1] != 'T' || fcc[2] != 'O' || fcc[3] != 'C') {
		ERROR_RECOVERABLE("Not a valid GHCS file, error found in ToC");
		return r;
	}
	byte nChunks = bufferReader.next_basic<byte>();
	r.reserve(nChunks);
	for (byte i = 0; i < nChunks; ++i) {
		ghcs_toc_chunk c;
		c.type = bufferReader.next_basic<byte>();
		c.location = bufferReader.next_basic<uint32>();
		c.size = bufferReader.next_basic<uint32>();
		r.push_back(c);
	}
	return r;
}

std::vector<Zone> parse_convex_poly_chunk(buffer_reader& reader)
{
	std::vector<Zone> r;
	const uint32 nZone = reader.next_basic<uint32>();
	r.reserve(nZone);
	for (uint32 i = 0; i < nZone; ++i) {
		Zone newZone;
		const unsigned short nPoint = reader.next_basic<unsigned short>();
		for (auto k = 0; k < nPoint; ++k) {
			Vec2 position = reader.next_user_data<Vec2>();
			newZone.m_poly.m_points.push_back(position);
		}
		newZone.m_hull = ConvexHull2(newZone.m_poly);
		newZone.m_position = Vec2::ZERO;
		r.push_back(newZone);
	}
	return r;
}

uint32 write_ghcs_header(buffer_writer& writer, ghcs_header* header)
{
	writer.append_c_str("GHCS");
	writer.append_byte(0);
	writer.append_byte(header->major_version);
	writer.append_byte(header->minor_version);
	writer.append_byte(header->is_big_endian ? ghcs_header::big_endian : ghcs_header::little_endian);
	writer.append_multi_byte<uint32>(header->toc_offset);
	return 12;
}

uint32 write_convex_poly_chunk(buffer_writer& writer, std::vector<Zone>& zones)
{
	//head
	//4cc
	const size_t begin_offset = writer.get_current_offset();
	writer.append_byte(0);
	writer.append_c_str("CHK");
	writer.append_byte(ghcs_ConvexPolysChunk);
	writer.append_byte(1); //little endianess
	const size_t offset_of_chunk_data_size = writer.get_current_offset(); // fill later
	writer.append_multi_byte<uint32>(0);
	const size_t offset_of_data_begin = writer.get_current_offset();
	const uint32 nConvex = zones.size();
	writer.append_multi_byte(nConvex);
	for (uint32 i = 0; i < nConvex; ++i) {
		auto& points = zones[i].m_poly.m_points;
		const unsigned short nPoints = points.size();
		writer.append_multi_byte(nPoints);
		for (auto j = 0; j < nPoints; ++j) {
			writer.append_user_data(points[j]);
		}
	}
	const size_t offset_of_data_end = writer.get_current_offset();
	const int diff = offset_of_data_end - offset_of_data_begin;
	writer.overwrite_bytes(offset_of_chunk_data_size, diff);
	return offset_of_data_end - begin_offset;
}