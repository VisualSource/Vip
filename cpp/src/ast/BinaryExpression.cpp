#include <vip/ast/BinaryExpression.hpp>

namespace ast
{
    BinaryExpression::~BinaryExpression()
    {
        if (lhs != nullptr)
            delete lhs;
        if (rhs != nullptr)
            delete rhs;
    }

    // <BinaryExpression
    //        lhs: expression
    //        op: char
    //        rhs: expression
    // >
    std::string BinaryExpression::toString(int padding)
    {
        std::string header = std::string("<BinaryExpression>\n").insert(0, padding, ' ');
        std::string left = lhs->toString(padding + 7);
        std::string opt = std::string("<Op value=\"" + std::to_string(op) + "\"/>\n").insert(0, padding + 7, ' ');
        std::string right = rhs->toString(padding + 7);
        std::string footer = std::string("</BinaryExpression>\n").insert(0, padding, ' ');
        return header + left + opt + right + footer;
    }
} // namespace ast
