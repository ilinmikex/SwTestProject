#pragma once

#include <string>
#include <sstream>
#include <functional>
#include "details/CommandParserVisitor.hpp"
#include "actors.h"

namespace sw::io
{
	class CommandParser {
	private:
		std::unordered_map<std::string, std::function<void(std::istream&)>> commands_;

	public:
		template <class TCommandData>
		CommandParser& add(std::function<void(TCommandData)> handler)
		{
			std::string commandName = TCommandData::Name;
			auto [it, inserted] = commands_.emplace(
										commandName,
										[handler = std::move(handler)](std::istream& stream)
										{
											TCommandData data;
											CommandParserVisitor visitor(stream);
											data.visit(visitor);
											handler(std::move(data));
										});
			if (!inserted) {
				throw std::runtime_error("Command already exists: " + commandName);
			}
			return *this;
		}
		void parse(std::istream& stream);
	};
}