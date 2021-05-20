#include "BinOpNode.h"
#include "NodeUtils.h"

using namespace std;

BinOpNode::BinOpNode(any left_node, Token op_token, any right_node, Position start, Position end): Node(start,end), left_node(left_node), op_token(op_token), right_node(right_node) {

}

Token BinOpNode::GetOpToken() const {
    return op_token;
}

any BinOpNode::GetRightNode() const {
    return right_node;
}

any BinOpNode::GetLeftNode() const {
    return left_node;
}

std::ostream& operator<<(std::ostream& lhs, const BinOpNode& token) {
      
    lhs << "(";
    NodeUtils::printNode(lhs, token.left_node);
    lhs << "," << token.op_token << ", ";
    NodeUtils::printNode(lhs, token.right_node);
    lhs << ")";

    return lhs;
}
