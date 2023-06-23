#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CheckPosition(Position pos) const;

    bool IsCellExist(Position pos) const;

    using cell_t = std::unique_ptr<CellInterface>;

    struct row_t {
        int cell_number = 0;
        std::vector<cell_t> cells;
    };

    int real_col_number = 0;

    int no_empty_col_number = 0;
    int no_empty_row_number = 0;

    std::vector<row_t> rows_;
};