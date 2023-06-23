#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    if (static_cast<int>(rows_.size()) <= pos.row) {
        rows_.resize(pos.row + 1);
    }

    real_col_number = std::max(pos.col + 1, real_col_number);

    for (row_t& row : rows_) {
        row.cells.resize(real_col_number);
    }

    row_t& current_row = rows_.at(pos.row);

    current_row.cell_number = std::max(current_row.cell_number, pos.col + 1);
    no_empty_col_number = std::max(no_empty_col_number, current_row.cell_number);

    cell_t& current_cell = current_row.cells.at(pos.col);

    if (!current_cell) {
        current_cell = std::make_unique<Cell>(*this);
    }

    Cell* cell_ptr = dynamic_cast<Cell*>(current_cell.get());

    if (!cell_ptr) {
        throw std::logic_error("Wrong cell!!!");
    }

    cell_ptr->Set(text);
    no_empty_row_number = std::max(no_empty_row_number, pos.row + 1);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos);

    if (!IsCellExist(pos)) {
        return nullptr;
    }

    return rows_.at(pos.row).cells.at(pos.col).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos);

    if (!IsCellExist(pos)) {
        return nullptr;
    }

    return rows_.at(pos.row).cells.at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);

    if (!IsCellExist(pos)) {
        return;
    }

    row_t& current_row = rows_.at(pos.row);

    cell_t& current_cell = current_row.cells.at(pos.col);
    current_cell.release();

    current_row.cell_number = 0;

    for (size_t i = 0; i < current_row.cells.size(); ++i) {
        if (current_row.cells[i]) {
            current_row.cell_number = i + 1;
        }
    }

    no_empty_col_number = 0;
    no_empty_row_number = 0;
    int i = 1;

    for (row_t& row : rows_) {
        no_empty_col_number = std::max(row.cell_number, no_empty_col_number);
        if (row.cell_number != 0) {
            no_empty_row_number = i;
        }
        ++i;
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;
    result.rows = no_empty_row_number;
    result.cols = no_empty_col_number;
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int i = 0; i < size.rows; ++i) {
        bool is_first = true;

        for (int j = 0; j < size.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }

            is_first = false;

            const cell_t& cell = rows_.at(i).cells.at(j);

            if (cell) {
                Cell::Value val = cell->GetValue();
                if (std::holds_alternative<double>(val)) {
                    output << std::get<double>(val);
                } else if (std::holds_alternative<std::string>(val)) {
                    output << std::get<std::string>(val);
                } else {
                    output << std::get<FormulaError>(val);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int i = 0; i < size.rows; ++i) {
        bool is_first = true;

        for (int j = 0; j < size.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }

            is_first = false;

            const cell_t& cell = rows_.at(i).cells.at(j);

            if (cell) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

void Sheet::CheckPosition(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Sheet: invalid position!\n");
    }
}

bool Sheet::IsCellExist(Position pos) const {
    if (pos.row >= static_cast<int>(rows_.size())) {
        return false;
    }
    if (pos.col >= real_col_number) {
        return false;
    }
    return true;
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
