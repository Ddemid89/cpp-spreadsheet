#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    void Notify() const;

    void Subscribe(const Cell& cell) const;

    void TryToSubscribe(const CellInterface* cell_interface);

    bool HasCircularDependency(const CellInterface* current_cell,
                               std::unordered_set<const CellInterface*>& checked_cells,
                               const CellInterface* start_cell);

    bool HasCircularDependency(const FormulaInterface& formula);

    mutable std::optional<Cell::Value> cache_;
    mutable std::unordered_set<const Cell*> dependent_cells_;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
};
