#pragma once
#include "./Node.hpp"

namespace ast
{

    class NumericLiteral : public Node
    {
    private:
        double value;

    public:
        NumericLiteral(double value) : value(value) {}
        std::string toString(int padding = 0) override;
    };
}