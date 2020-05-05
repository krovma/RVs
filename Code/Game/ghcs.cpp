#pragma once
#include "Game/RVSGame.hpp"
#include "Engine/Core/StringUtils.hpp"

using byte = unsigned char;

struct ghcs_header
{
	byte major_version = 0;
	byte minor_version = 0;
	bool big_endian = false;
	unsigned int toc_offset = 0;
};

ghcs_header parse_ghcs_header(buffer_reader& bufferReader)
{
	ghcs_header r;
	return r;
}
