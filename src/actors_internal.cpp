#include <vector>
#include <map>
#include <algorithm>

#include <IO/Commands/SpawnWarrior.hpp>
#include <IO/Commands/SpawnArcher.hpp>
#include <IO/Commands/March.hpp>

#include <IO/Events/MapCreated.hpp>
#include <IO/Events/UnitSpawned.hpp>
#include <IO/Events/MarchStarted.hpp>
#include <IO/Events/UnitDied.hpp>
#include <IO/Events/UnitAttacked.hpp>
#include <IO/Events/UnitMoved.hpp>
#include <IO/Events/MarchEnded.hpp>

#include "actors.h"
#include "actors_internal.h"

namespace sw
{



Logger* AcquireLogger()
{
    static Logger log;
    return &log;
}

//Some common implementations
template<typename TCommandData>
class UnitImpl : public IUnitInternal
{
public:
    UnitImpl(IBattleFieldInternal* field, const TCommandData& data)
        :   field_(field)
        ,   cmddata_(data)
        ,   iter_(path_.end())
    {
        CheckFatal(!!field_);
    }
    void MarchTo(const Coord& target) override
    {
        path_ = field_->AcquirePath({cmddata_.x, cmddata_.y}, target);
        iter_ = path_.begin();
        AcquireLogger()->Log(io::MarchStarted { cmddata_.unitId, cmddata_.x, cmddata_.y, target.x, target.y });
    }
    void DoAttack(const Attack& attack) override
    {
        cmddata_.hp = (attack.damage > cmddata_.hp) ? 0 : cmddata_.hp - attack.damage;
        AcquireLogger()->Log(io::UnitAttacked{attack.attacker_id, cmddata_.unitId, attack.damage, cmddata_.hp });
        if(Dead())
        {
            AcquireLogger()->Log(io::UnitDied{ cmddata_.unitId });
        }
    }
    uint32_t Id() const override
    {
        return cmddata_.unitId;
    }
    bool Dead() const override
    {
        return !cmddata_.hp;
    }
    Coord CurrentPosition() const
    {
        return get_my_pos();
    }
    bool IfAttackHarmful(const Attack& attack) const override
    {
        return attack.type == Attack::arrow || attack.type == Attack::close_combat;
    }
protected:
    Coord get_my_pos() const
    {
        if(path_.empty())
        {
            return { cmddata_.x, cmddata_.y };
        }
        CheckFatal(iter_ != path_.end());
        return *iter_;
    }
protected:
    IBattleFieldInternal* field_;
    TCommandData cmddata_;
    std::vector<Coord> path_;
    std::vector<Coord>::iterator iter_;
};

//Specific implementation for each unit.

class Warrior : public UnitImpl<io::SpawnWarrior>
{
public:
    Warrior(IBattleFieldInternal* field, const io::SpawnWarrior& warrior)
        :   UnitImpl(field, warrior)
    {
        ;
    }
    Coord NextStep(bool& further) override
    {
        CheckFatal(iter_ != path_.end());
        if(Dead())
        {
            further = false;
            return *iter_;
        }
        const auto my_pos = get_my_pos();

        //Check if can attack closely
        const uint32_t radius = 1;
        const auto coordinates_around = field_->AcquireCoordinatesAround(my_pos, radius, radius);
        auto* unit_to_attack = field_->GetUnitToAttack(coordinates_around);
        if(unit_to_attack)
        {
            Attack attack;
            attack.type = Attack::close_combat;
            attack.attacker_id = cmddata_.unitId;
            attack.damage = cmddata_.strength;
            attack.attacker_cell = my_pos;
            if(unit_to_attack->IfAttackHarmful(attack))
            {
                unit_to_attack->DoAttack(attack);
            }
            further = true;
            return my_pos;
        }
        auto inext = std::next(iter_);
        if(inext == path_.end())
        {
            further = false;
            AcquireLogger()->Log(io::MarchEnded{cmddata_.unitId, iter_->x, iter_->y});
            return *iter_;
        }
        //If cannot attack, move to next cell.
        ++iter_;
        CheckFatal(iter_ != path_.end());
        AcquireLogger()->Log(io::UnitMoved{cmddata_.unitId, iter_->x, iter_->y});
        further = true;
        return *iter_;
    }
};

class Archer : public UnitImpl<io::SpawnArcher>
{
public:
    Archer(IBattleFieldInternal* field, const io::SpawnArcher& archer)
        : UnitImpl(field, archer)
    {
        ;
    }
    Coord NextStep(bool& further) override
    {
        CheckFatal(iter_ != path_.end());
        if(Dead())
        {
            further = false;
            return *iter_;
        }

        const auto my_pos = get_my_pos();
        //Check if can attack closely.
        {
            const uint32_t radius = 1;
            const auto coordinates_around = field_->AcquireCoordinatesAround(my_pos, radius, radius);
            auto* unit_to_attack = field_->GetUnitToAttack(coordinates_around);
            if(unit_to_attack)
            {
                Attack attack;
                attack.attacker_id = cmddata_.unitId;
                attack.attacker_cell = my_pos;
                attack.type = Attack::close_combat;
                attack.damage = cmddata_.strength;
                if(unit_to_attack->IfAttackHarmful(attack))
                {
                    unit_to_attack->DoAttack(attack);
                }
                further = true;
                return my_pos;
            }
        }
        //Check if can attack remotely.
        {
            const uint32_t radius_from = 2;
            const uint32_t radius_to = cmddata_.range;
            const auto coordinates_around = field_->AcquireCoordinatesAround(my_pos, radius_from, radius_to);
            auto* unit_to_attack = field_->GetUnitToAttack(coordinates_around);

            if(unit_to_attack)
            {
                Attack attack;
                attack.attacker_id = cmddata_.unitId;
                attack.attacker_cell = my_pos;
                attack.type = Attack::arrow;
                attack.damage = cmddata_.agility;
                if(unit_to_attack->IfAttackHarmful(attack))
                {
                    unit_to_attack->DoAttack(attack);
                }
                further = true;
                return my_pos;
            }
        }
        if(std::next(iter_) == path_.end())
        {
            AcquireLogger()->Log(io::MarchEnded{cmddata_.unitId, iter_->x, iter_->y});
            further = false;
            return *iter_;
        }
        //If cannot attack, move to next cell.
        ++iter_;
        CheckFatal(iter_ != path_.end());
        AcquireLogger()->Log(io::UnitMoved{cmddata_.unitId, iter_->x, iter_->y});
        further = true;
        return *iter_;
    }
};

template<typename TCommandData>
UnitPtr CreateUnit(IBattleFieldInternal* field, const TCommandData&);

template<>
UnitPtr CreateUnit<io::SpawnWarrior>(IBattleFieldInternal* field, const io::SpawnWarrior& warrior)
{
    UnitPtr ptr;
    ptr.reset(new Warrior(field, warrior));
    return ptr;
}
template<>
UnitPtr CreateUnit<io::SpawnArcher>(IBattleFieldInternal* field, const io::SpawnArcher& archer)
{
    UnitPtr ptr;
    ptr.reset(new Archer(field, archer));
    return ptr;
}

class UnitStorage
{
public:
    void StoreUnit(UnitPtr&& new_unit)
    {
        CheckFatal(!!new_unit);
        auto iter = std::find_if(units_.cbegin(), units_.cend(), [&new_unit](const auto& item) { return new_unit->Id() == item->Id(); });
        CheckRt(iter == units_.cend(), "Unit already created");
        units_.push_back(std::move(new_unit));
    }
    IUnitInternal* Get(uint32_t id) const
    {
        auto iter = std::find_if(units_.cbegin(), units_.cend(), [id](const auto& item) { return item->Id() == id; });
        CheckFatal(iter != units_.end());
        return iter->get();
    }
    //Wrapper to iterate over UnitStorage directly.
    class Iterator
    {
    public:
        Iterator(std::vector<UnitPtr>* units, bool start)
            : units_(units)
        {
            iter_ = start
                ? units_->begin()
                : units_->end();
        }
        Iterator& operator++()
        {
            ++iter_;
            return *this;
        }
        IUnitInternal* operator->()
        {
            CheckFatal(iter_ != units_->end());
            return iter_->get();
        }
        IUnitInternal* operator*()
        {
            CheckFatal(iter_ != units_->end());
            return iter_->get();
        }
    private:
        std::vector<UnitPtr>* units_;
        std::vector<UnitPtr>::iterator iter_;
        friend bool operator==(const UnitStorage::Iterator& lv, const UnitStorage::Iterator& rv)
        {
            return lv.iter_ == rv.iter_;
        }
        friend bool operator!=(const UnitStorage::Iterator& lv, const UnitStorage::Iterator& rv)
        {
            return lv.iter_ != rv.iter_;
        }
    };
    Iterator begin()
    {
        return Iterator(&units_, /*start*/true);
    }
    Iterator end()
    {
        return Iterator(&units_, /*start*/false);
    }

private:
    std::vector<UnitPtr> units_;
    friend class Iterator;
};

class BattleField
    : public IBattleField
    , public IBattleFieldInternal
{
public:
    BattleField(const io::CreateMap& amap)
        :   amap_(amap)
    {
        CheckRt(amap_.height && amap_.width, "Invalid arguments: height or width is zero");
        AcquireLogger()->Log(io::MapCreated{amap_.width, amap_.height});
    }
    //IBattleField
    void AddUnit(const io::SpawnWarrior& warrior)
    {
        auto ptr = CreateUnit(this, warrior);
        AddUnitI(std::move(ptr), { warrior.x, warrior.y });
        AcquireLogger()->Log(io::UnitSpawned{ warrior.unitId, warrior.Name, warrior.x, warrior.y});
    }
    void AddUnit(const io::SpawnArcher& archer)
    {
        auto ptr = CreateUnit(this, archer);
        AddUnitI(std::move(ptr), { archer.x, archer.y });
        AcquireLogger()->Log(io::UnitSpawned{ archer.unitId, archer.Name, archer.x, archer.y});
    }
    void MarchTo(const io::March& march)
    {
        auto* unit = storage_.Get(march.unitId);
        CheckRt(!!unit, "Unit not found");
        unit->MarchTo({ march.targetX, march.targetY });
    }
    //IBattleFieldInternal
    std::vector<Coord> AcquirePath(const Coord& mine, const Coord& target) override
    {
        return Bresenham(mine, target);
    }
    std::vector<Coord> AcquireCoordinatesAround(const Coord& mine, uint32_t radius_from, uint32_t radius_to) override
    {
        const Coord extreme_cell(amap_.width - 1, amap_.height - 1);
        return CoordinatesAround(mine, extreme_cell, radius_from, radius_to);
    }
    bool DoNextStep() override
    {
        int further(0);
        for(auto* unit : storage_)
        {
            const auto current_pos = unit->CurrentPosition();
            positions_.erase(current_pos);

            bool further_step(false);
            const auto new_pos = unit->NextStep(further_step);
            further += static_cast<int>(further_step);
            positions_[new_pos] = unit->Id();
        }
        return (further > 1);
    }
    IUnitInternal* GetUnitToAttack(const std::vector<Coord>& coords)
    {
        for(const auto& coord : coords)
        {
            auto iter = positions_.find(coord);
            if(iter == positions_.end())
            {
                continue;
            }
            const int32_t id = iter->second;
            auto* unit = storage_.Get(id);
            if(!unit->Dead())
            {
                return unit;
            }
        }
        return nullptr;
    }
private:
    void AddUnitI(UnitPtr&& unit, const Coord& coord)
    {
        CheckRt(coord.x < amap_.width, "X coordinate: out of range");
        CheckRt(coord.y < amap_.height, "Y coordinate: out of range");
        
        auto iter = positions_.find(coord);
        CheckRt(iter == positions_.cend(), "Could not place unit into the cell specified");
        positions_[coord] = unit->Id();

        storage_.StoreUnit(std::move(unit));
    }
private:
    io::CreateMap amap_;
    UnitStorage storage_;
    std::map<Coord, uint32_t> positions_;
};

std::unique_ptr<IBattleField> CreateBattleField(const io::CreateMap& createmap)
{
    std::unique_ptr<IBattleField> ptr;
    CheckRt(createmap.height && createmap.width, "Incorrect width or height");
    ptr.reset(new BattleField(createmap));
    return ptr;
}

}//namespace sw