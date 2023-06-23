#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    output << fe.ToString();
    return output;
}

class Mediator {
public:
    double operator()(double val) {
        return val;
    }

    double operator()(const std::string& txt) {
        if (txt.empty()) {
            return 0.;
        }
        std::istringstream in(txt);
        double d_res;

        in >> d_res;

        if (!in.fail() && in.eof()) {
            return d_res;
        }

        throw FormulaError(FormulaError::Category::Value);
    }

    double operator()(FormulaError error) {
        throw error;
    }
};

class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            double res = ast_.Execute([&sheet](Position pos){
                                        if (!pos.IsValid()) {
                                            throw FormulaError(FormulaError::Category::Ref);
                                        }

                                        auto cell = sheet.GetCell(pos);
                                        if (!cell) {
                                            return 0.;
                                        }

                                        auto res = cell->GetValue();

                                        Mediator mediator;

                                        return std::visit(mediator, res);
                                      });
            return res;
        } catch (FormulaError& e){
            return e;
        }
    }

    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto list = ast_.GetCells();
        std::vector<Position> result(list.begin(), list.end());
        return result;
    }
private:
    FormulaAST ast_;
};

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Incorrect formula!");
    }
}
