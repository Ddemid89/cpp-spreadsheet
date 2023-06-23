#include "cell.h"

#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl{
public:
    virtual Cell::Value GetValue(Sheet&) = 0;
    virtual std::string GetText() = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    Cell::Value GetValue(Sheet&) override {
        return "";
    }
    std::string GetText() override {
        return "";
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string text) : text_(text) {}

    Cell::Value GetValue(Sheet&) override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() override {
        return text_;
    }
private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(std::unique_ptr<FormulaInterface>& formula)
                : formula_(std::move(formula)) {}

    Cell::Value GetValue(Sheet& sheet) override {
        auto result = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        return std::get<FormulaError>(result);
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    std::string GetText() override {
        return std::string(1, FORMULA_SIGN) + formula_->GetExpression();
    }
private:
    std::unique_ptr<FormulaInterface> formula_;
};


Cell::Cell(Sheet& sheet) : sheet_(sheet) {
    impl_ = std::unique_ptr<EmptyImpl>(new EmptyImpl{});
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::unique_ptr<EmptyImpl>(new EmptyImpl{});
        Notify();
        return;
    }

    if (text.size() < 2 || text[0] != FORMULA_SIGN) {
        impl_ = std::unique_ptr<TextImpl>(new TextImpl{std::move(text)});
        Notify();
        return;
    }

    std::unique_ptr<FormulaInterface> temp;

    temp = ParseFormula(text.substr(1));

    if (HasCircularDependency(*temp)) {
        throw CircularDependencyException("Circular dependency!");
    }

    Notify();
    auto formula_impl_ptr = std::make_unique<FormulaImpl>(temp);
    FormulaImpl& formula_impl_ref = *formula_impl_ptr.get();
    impl_ = std::move(formula_impl_ptr);

    for (Position pos : formula_impl_ref.GetReferencedCells()) {
        assert(pos.IsValid());

        const CellInterface* cell = sheet_.GetCell(pos);

        if (!cell) {
            sheet_.SetCell(pos, "");
            cell = sheet_.GetCell(pos);
        }

        TryToSubscribe(cell);
    }
}

void Cell::Clear() {
    impl_ = std::unique_ptr<EmptyImpl>(new EmptyImpl{});
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::Notify() const {
    if (cache_) {
        cache_.reset();
        for (const Cell* cell : dependent_cells_) {
            cell->Notify();
        }
    }
}

void Cell::Subscribe(const Cell& cell) const {
    dependent_cells_.insert(&cell);
}

void Cell::TryToSubscribe(const CellInterface* cell_interface) {
    const Cell* cell = dynamic_cast<const Cell*>(cell_interface);
    if (cell) {
        cell->Subscribe(*this);
    }
}

bool Cell::HasCircularDependency(const FormulaInterface& formula) {
    std::unordered_set<const CellInterface*> checked_cells;
    for (Position pos : formula.GetReferencedCells()) {
        assert(pos.IsValid());
        const CellInterface* cell = sheet_.GetCell(pos);
        if (cell && HasCircularDependency(cell, checked_cells, this)) {
            return true;
        }
    }
    return false;
}

bool Cell::HasCircularDependency(const CellInterface* current_cell,
                           std::unordered_set<const CellInterface*>& checked_cells,
                           const CellInterface* start_cell) {
    if (checked_cells.count(current_cell) != 0) {
        return false;
    }
    if (current_cell == start_cell) {
        return true;
    }
    for (Position pos : current_cell->GetReferencedCells()) {
        assert(pos.IsValid());
        const CellInterface* cell = sheet_.GetCell(pos);
        if (cell && HasCircularDependency(cell, checked_cells, start_cell)) {
            return true;
        }
    }
    checked_cells.insert(current_cell);
    return false;
}
