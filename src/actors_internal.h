#ifndef __ACTORS_INTERNAL_H__
#define __ACTORS_INTERNAL_H__
#include <memory>
#include <vector>
#include "helper.h"

namespace sw
{

//! \brief Attack descriptor.
struct Attack
{
    /*! \brief Attack type. Add here new type if a unit can attack this way.
    */
    enum harmful_attack_t
    {
        unknown,
        close_combat,
        arrow
    };
    uint32_t attacker_id = 0;
    uint32_t damage = 0;
    Coord attacker_cell;
    harmful_attack_t type = unknown;
};

//! \brief Private interfaces seen by actors to operate internally.
class IUnitInternal
{
public:
    virtual ~IUnitInternal() = default;

    /*! \brief Accept an attack.
        \param attack Attack parameters.
    */
    virtual void DoAttack(const Attack& attack) = 0;

    //! \brief Check if unit is dead.
    virtual bool Dead() const = 0;

    /*! \brief Force unit to make the next step.
        \param [out] further True if there are further steps.
        \return Coordinates of new position.
    */
    virtual Coord NextStep(bool& further) = 0;

    //! \brief Get Id of the unit.
    virtual uint32_t Id() const = 0;

    //! \brief Start march to the cell specified.
    virtual void MarchTo(const Coord& coord) = 0;

    //! \brief Get position of the unit.
    virtual Coord CurrentPosition() const = 0;

    //! \brief Check if this attack is harmful for the target.
    virtual bool IfAttackHarmful(const Attack& attack) const = 0;
};

using UnitPtr = std::unique_ptr<IUnitInternal>;

//! \brief Internal interface used by actors within the battle.
class IBattleFieldInternal
{
public:
    virtual ~IBattleFieldInternal() = default;

    /*! \brief Get a unit to attack.
        \param coords std::vector of cells to select unit from.
        \return IUnitInternal* pointer. NULL if no units found in the cells specified.
    */
    virtual IUnitInternal* GetUnitToAttack(const std::vector<Coord>& coords) = 0;

    //! \brief Get path between two cells, Besenham's algorithm.
    virtual std::vector<Coord> AcquirePath(const Coord& from, const Coord& target) = 0;

    /*! \brief Get cells around.
        \param center center cell.
        \param radius_from
        \param radius_to
        \return std::vector of cells around `center`.
    */
    virtual std::vector<Coord> AcquireCoordinatesAround(const Coord& center, uint32_t radius_from, uint32_t radius_to) = 0;
};

}//namespace sw

#endif /*__ACTORS_INTERNAL_H__*/