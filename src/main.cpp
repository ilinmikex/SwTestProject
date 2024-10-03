#include <iostream>
#include <fstream>
#include <IO/System/CommandParser.hpp>
#include <IO/System/PrintDebug.hpp>
#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/SpawnWarrior.hpp>
#include <IO/Commands/SpawnArcher.hpp>
#include <IO/Commands/March.hpp>
#include <IO/System/EventLog.hpp>
#include <IO/Events/MapCreated.hpp>
#include <IO/Events/UnitSpawned.hpp>
#include <IO/Events/MarchStarted.hpp>
#include <IO/Events/MarchEnded.hpp>
#include <IO/Events/UnitMoved.hpp>
#include <IO/Events/UnitDied.hpp>
#include <IO/Events/UnitAttacked.hpp>
#include <memory>
#include "actors.h"
#include "helper.h"

namespace sw
{

class SimulatingMachine
{
public:
	SimulatingMachine(const char* filename)
		:	file_(filename)
	{
		Expected(!!file_, "File not found");
		parser_
			.add<io::CreateMap>(
				[this](auto command)
				{
					Expected(!field_, "Already created");
					field_ = CreateBattleField(command);
				})
			.add<io::SpawnWarrior>(
				[this](auto command)
				{
					Expected(!!field_, "Battle field has not been created");
					field_->AddUnit(command);
				})
			.add<io::SpawnArcher>(
				[this](auto command)
				{
					Expected(!!field_, "Battle field has not been created");
					field_->AddUnit(command);
				})
			.add<io::March>(
				[this](auto command)
				{
					Expected(!!field_, "Battle field has not been created");
					field_->MarchTo(command);
				});
	}
	void Run()
	{
		parser_.parse(file_);
		while(true)
		{
			//const auto ch = getchar();
			std::cout << std::endl;
			AcquireLogger()->NextTick();
			const bool steps_more = field_->DoNextStep();
			if(!steps_more)
			{
				break;
			}
		}
	}
private:
	std::ifstream file_;
	io::CommandParser parser_;
	std::unique_ptr<IBattleField> field_;
};

}//namespace sw

int main(int argc, char** argv)
{
	using namespace sw;

	if (argc != 2)
	{
		throw std::runtime_error("Error: No file specified in command line argument");
	}
	sw::SimulatingMachine sm(argv[1]);
	sm.Run();

	return 0;
}
