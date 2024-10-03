#ifndef __ACTORS_H__
#define __ACTORS_H__
#include <set>
#include <memory>
#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/SpawnWarrior.hpp>
#include <IO/Commands/SpawnArcher.hpp>
#include <IO/Commands/March.hpp>

#include <IO/System/PrintDebug.hpp>
#include <IO/System/EventLog.hpp>

namespace sw
{

//! \brief Throw std::runtime_error if condition is false.
void Expected(bool condition, const char* message);

class Logger
{
public:
    Logger()
        :   tick_(0)
    {}
    template<typename TEvent>
    void Log(TEvent&& evt)
    {
        log_.log(tick_, std::move(evt));
    }
    void NextTick()
    {
        ++tick_;
    }
private:
    uint64_t tick_;
    sw::EventLog log_;
};

Logger* AcquireLogger();

//! \brief Public interface to operate on.
class IBattleField
{
public:
    virtual ~IBattleField() = default;
    
    //! \brief Add unit of type io::SpawnWarrior.
    virtual void AddUnit(const io::SpawnWarrior& warrior) = 0;

    //! \brief Add unit of type io::SpawnArcher.
    virtual void AddUnit(const io::SpawnArcher& archer) = 0;

    //! \brief Start march of the unit specified in `march` argument.
    virtual void MarchTo(const io::March& march) = 0;

    /*! \brief Enforce all actors to do next step.
        \return true if further steps may be done. False otherwise.
    */
    virtual bool DoNextStep() = 0;
};

/*! \brief Create a new battle field.
    \return IBattleField pointer. Never returns nullptr.
    \exception std::runtime error if CreateMap::width or Create::height equals to 0.
*/
std::unique_ptr<IBattleField> CreateBattleField(const io::CreateMap&);

}//namespace sw

#endif /*__ACTORS_H__*/