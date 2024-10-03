#ifndef __HELPER_H__
#define __HELPER_H__
#include <cstdint>
#include <stdexcept>

namespace sw
{
/*
    Helper functions.
*/
inline void CheckRt(bool condition, const char* message)
{
    if(!condition)
    {
        throw std::runtime_error(message);
    }
}

inline void Expected(bool condition, const char* message)
{
    CheckRt(condition, message);
}

inline void CheckFatal(bool condition)
{
    if(!condition)
    {
        throw std::logic_error("Internal error.");
    }
}

struct Cell
{
    int64_t x = 0;
    int64_t y = 0;
};
inline bool operator<(const Cell& lv, const Cell& rv)
{
    return lv.x == rv.x ? lv.y < rv.y : lv.x < rv.x;
}

struct Coord
{
    uint32_t x;
    uint32_t y;
    Coord(uint32_t xx, uint32_t yy)
        :   x(xx)
        ,   y(yy)
    {
        ;
    }
    Coord()
    {
        Clear();
    }
    void Clear()
    {
        x = ~uint32_t();
        y = ~uint32_t();
    }
};

inline bool operator<(const Coord& lv, const Coord& rv)
{
    return std::pair<uint32_t, uint32_t>(lv.x, lv.y) < std::pair<uint32_t, uint32_t>(rv.x, rv.y);
}
inline bool operator==(const Coord& lv, const Coord& rv)
{
    return lv.x == rv.x && lv.y == rv.y;
}
inline bool operator!=(const Coord& lv, const Coord& rv)
{
    return !(lv == rv);
}

inline std::set<Cell> GetCellsOfLevels(int64_t level_from, int64_t level_to)
{
    if(level_from > level_to)
    {
        return {};
    }
    std::set<Cell> cells;
    for(auto level = level_from; level <= level_to; ++level)
    {
        for(int64_t x = 0; x <= level; ++x)
        {
            cells.emplace(x, level);
        }
        for(int64_t y = 0; y <= level; ++y)
        {
            cells.emplace(level, y);
        }
    }
    const std::vector<Cell> quarters
    {
        {-1, 1},
        {1, -1},
        {-1, -1}
    };
    for(const auto& quarter : quarters)
    {
        std::set<Cell> aux;
        std::transform(cells.cbegin(), cells.cend(), std::inserter(aux, aux.end()), [&quarter](const auto& cell)
        {
            return Cell(cell.x * quarter.x, cell.y * quarter.y);
        });
        std::copy(aux.cbegin(), aux.cend(), std::inserter(cells, cells.end()));
    }
    return cells;
}

inline std::set<Cell> MoveCells(const std::set<Cell>& cells, const Cell& new_center)
{
    std::set<Cell> result;
    std::transform(cells.cbegin(), cells.cend(), std::inserter(result, result.end()), [&new_center](const auto& cell)
    {
        Cell moved;
        moved.x = cell.x + new_center.x;
        moved.y = cell.y + new_center.y;
        return moved;
    });
    return result;
}

inline std::vector<Coord> RemoveCellsOutOfBounds(const std::set<Cell>& cells, const Cell& extreme_point)
{
    std::set<Cell> tmp;
    std::copy_if(cells.cbegin(), cells.cend(), std::inserter(tmp, tmp.end()), [&extreme_point](const auto& cell)
    {
        return
            cell.x >= 0 && cell.x <= extreme_point.x &&
            cell.y >= 0 && cell.y <= extreme_point.y;
    });
    std::vector<Coord> result;
    std::transform(tmp.cbegin(), tmp.cend(), std::back_inserter(result), [](const auto& cell)
    {
        return Coord{ static_cast<uint32_t>(cell.x), static_cast<uint32_t>(cell.y) };
    });
    return result;
}

inline std::vector<Coord> CoordinatesAround(const Coord& center, const Coord& extreme_point, uint32_t radius_from, uint32_t radius_to)
{
    const auto cells = GetCellsOfLevels(static_cast<int64_t>(radius_from), static_cast<int64_t>(radius_to));
    const Cell new_center(static_cast<int64_t>(center.x), static_cast<int64_t>(center.y));
    const auto cells2 = MoveCells(cells, new_center);

    Cell extreme;
    extreme.x = extreme_point.x;
    extreme.y = extreme_point.y;
    return RemoveCellsOutOfBounds(cells2, extreme);
}

inline std::vector<Coord> Bresenham(const Coord& start, const Coord& end)
{
    std::vector<Cell> tmp;

    int64_t x1 = start.x;
    int64_t x2 = end.x;
    int64_t y1 = start.y;
    int64_t y2 = end.y;

    int64_t dx = abs(x2 - x1);
    int64_t dy = abs(y2 - y1);
    int64_t sx = (x1 < x2) ? 1 : -1;
    int64_t sy = (y1 < y2) ? 1 : -1;
    int64_t err = dx - dy;

    while (true) {
        tmp.emplace_back(x1, y1);
        if (x1 == x2 && y1 == y2)
        {
            break;
        }
        int64_t err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    std::vector<Coord> result;
    std::transform(tmp.cbegin(), tmp.cend(), std::back_inserter(result), [](const Cell& cell)
    {
        CheckFatal(cell.x >= 0 && cell.y >= 0);
        return Coord(cell.x, cell.y);
    });
    return result;
}

};

#endif /*__HELPER_H__*/