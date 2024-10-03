#pragma once

#include <iosfwd>
#include <cstdint>

namespace sw::io
{
	struct SpawnWarrior
	{
		constexpr static const char* Name = "SPAWN_WARRIOR";

		uint32_t unitId {};
		uint32_t x {};
		uint32_t y {};
		uint32_t hp {};
		uint32_t strength {};

		template <typename Visitor>
		void visit(Visitor& visitor)
		{
			visitor.visit("unitId", unitId);
			visitor.visit("x", x);
			visitor.visit("y", y);
			visitor.visit("hp", hp);
			visitor.visit("strength", strength);
		}
	};
}