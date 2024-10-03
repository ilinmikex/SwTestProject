#include "CommandParser.hpp"

namespace sw::io
{
	void CommandParser::parse(std::istream& stream)
	{
		std::string line;
		while (std::getline(stream, line)) {
			if (line.rfind("//", 0) == 0 || line.empty())
				continue;

			std::istringstream commandStream(line);
			std::string commandName;
			commandStream >> commandName;

			if (commandName.empty())
				continue;

			auto command = commands_.find(commandName);
			if (command == commands_.end())
				throw std::runtime_error("Unknown command: " + commandName);

			command->second(commandStream);
		}
	}
}