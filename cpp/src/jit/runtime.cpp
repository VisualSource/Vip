#include <vip/jit/runtime.hpp>
#include <stdexcept>
#include <vip/jit/components/InternalFunction.hpp>

#include <vip/jit/components/Function.hpp>
#include <vip/jit/components/String.hpp>
#include <vip/jit/components/Number.hpp>
#include <vip/ast/BinaryExpression.hpp>
#include <vip/jit/components/Null.hpp>
#include <vip/ast/ReturnStatement.hpp>
#include <vip/ast/WhileExpression.hpp>
#include <vip/ast/CallExpression.hpp>
#include <vip/ast/NumericLiteral.hpp>
#include <vip/ast/StringLiteral.hpp>
#include <vip/ast/Consts.hpp>
#include <vip/jit/Consts.hpp>

namespace jit
{
    Runtime::Runtime()
    {
        ctx = new Context("<root>", nullptr);
        ctx->set("false", std::shared_ptr<Number>(new Number(false)));
        ctx->set("true", std::shared_ptr<Number>(new Number(true)));
    }
    Runtime::~Runtime()
    {
        delete ctx;
    }

    void Runtime::declare(std::string key, std::shared_ptr<Object> value)
    {
        ctx->set(key, std::move(value));
    }
    void Runtime::drop(std::string key)
    {
        ctx->remove(key);
    }
    std::shared_ptr<Object> Runtime::execute(ast::Program &program, bool returnLast)
    {
        auto statements = program.getStatements();

        auto result = visitStatements(statements, ctx, returnLast);
        return result.first;
    }

    std::pair<std::shared_ptr<Object>, bool> Runtime::visitStatement(ast::Node *statement, Context *context)
    {
        switch (statement->getKind())
        {
        case ast::consts::VARIABLE_STATEMENT:
        { // Variale Statement
            if (auto *v = dynamic_cast<ast::VariableStatement *>(statement); v != nullptr)
            {
                visitVariableStatement(v, context);
                break;
            }
            throw std::runtime_error("Expected a variable statement.");
        }
        case ast::consts::IF_STATEMENT:
        { // if statement
            if (auto *v = dynamic_cast<ast::IfStatement *>(statement); v != nullptr)
            {
                return visitIfStatement(v, context);
            }
            throw std::runtime_error("Expected a if statement.");
        }
        case ast::consts::FUNCTION_EXPRESSION:
        { // Function declartion
            if (auto *v = dynamic_cast<ast::FunctionDeclartion *>(statement); v != nullptr)
            {
                visitFunctionDeclartion(v, context);
                break;
            }
            throw std::runtime_error("Expected a function declartion statement.");
        }
        case ast::consts::EXPRESSION_STATEMENT:
        { // expression
            if (auto *v = dynamic_cast<ast::ExpressionStatement *>(statement); v != nullptr)
            {
                return std::make_pair(visitExpression(v->getExpression(), context), false);
            }
            throw std::runtime_error("Expected a expression statement statement.");
        }
        case ast::consts::RETURN_STATEMENT:
        { // return statement
            if (auto *v = dynamic_cast<ast::ReturnStatement *>(statement); v != nullptr)
            {
                return std::make_pair(visitExpression(v->getExpression(), context), true);
            }
            throw std::runtime_error("Uncaught SyntaxError: Illegal statement");
        }
        case ast::consts::WHILE_EXRESSION:
        {
            if (auto *w = dynamic_cast<ast::WhileExpression *>(statement); w != nullptr)
            {
                while (true)
                {
                    auto while_ctx = new Context("<while>", context, context->canReturn());
                    auto expr = visitExpression(w->getExpression(), context);
                    if (auto r = std::dynamic_pointer_cast<Number>(expr); r == nullptr || (r != nullptr && !r->asBool()))
                    {
                        break;
                    }
                    auto result = visitStatements(w->getBody()->getStatements(), while_ctx);
                    if (result.second)
                        return result;
                }

                return std::make_pair(std::shared_ptr<Null>(new Null()), false);
            }
            throw std::runtime_error("Uncaught SyntaxError: Illegal statement");
        }
        default:
            throw std::runtime_error("Uncaught SyntaxError: Illegal statement");
        }

        return std::make_pair(std::shared_ptr<Null>(new Null()), false);
    }
    std::pair<std::shared_ptr<Object>, bool> Runtime::visitStatements(std::vector<ast::Node *> &statements, Context *context, bool returnLast)
    {
        int last = statements.size() - 1;
        int idx = 0;
        for (auto &&i : statements)
        {

            auto result = visitStatement(i, context);
            if (result.second || (returnLast && (idx == last)))
            {
                if (context->canReturn() || returnLast)
                    return result;

                throw std::runtime_error("Uncaught SyntaxError: Illegal return statement");
            }

            idx++;
        }

        return std::make_pair(std::shared_ptr<Null>(new Null()), false);
    }
    std::shared_ptr<Object> Runtime::visitExpression(ast::Node *value, Context *context)
    {
        if (auto bin = dynamic_cast<ast::BinaryExpression *>(value); bin != nullptr)
        {
            if (bin->getOp() == ast::consts::EQUAL)
            {
                auto ident = dynamic_cast<ast::Identifier *>(bin->getLhs());
                if (ident == nullptr)
                    throw std::runtime_error("Can not assign to value.");

                std::shared_ptr<Object> rhs = visitExpression(bin->getRhs(), context);
                if (rhs == nullptr)
                    throw std::runtime_error("No value on rhs.");

                return context->update(ident->getValue(), rhs);
            }

            std::shared_ptr<Object> lhs = visitExpression(bin->getLhs(), context);
            std::shared_ptr<Object> rhs = visitExpression(bin->getRhs(), context);

            if (lhs == nullptr || rhs == nullptr)
            {
                throw std::runtime_error("Unable to operate on given types.");
            }

            if (lhs->getKind() != rhs->getKind())
            {
                throw std::runtime_error("Can not operate on two different types.");
            }

            switch (bin->getOp())
            {
            case ast::consts::AND:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(ln->asBool() && rn->asBool()));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::OR:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(ln->asBool() || rn->asBool()));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::GREATER_THEN:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln > *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::GREATER_THEN_OR_EQUAL:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln >= *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::LESS_THEN_OR_EQUAL:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln <= *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::LESS_THEN:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln < *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::MINUS:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln - *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::DIV:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln / *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::MULT:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln * *rn));
                }
                throw std::runtime_error("Invalid operation.");
            }
            case ast::consts::PLUS:
            {
                if (auto ln = std::dynamic_pointer_cast<Number>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<Number>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a number");

                    return std::shared_ptr<Number>(new Number(*ln + *rn));
                }
                else if (auto ln = std::dynamic_pointer_cast<String>(lhs); ln != nullptr)
                {
                    auto rn = std::dynamic_pointer_cast<String>(rhs);
                    if (rn == nullptr)
                        throw std::runtime_error("rhs does is not a string");

                    return std::shared_ptr<String>(new String(*ln + *rn));
                }

                throw std::runtime_error("Invalid operation.");
            }
            default:
                throw std::runtime_error("Unknown operation");
            }
        }
        else if (auto call = dynamic_cast<ast::CallExpression *>(value); call != nullptr)
        {
            auto name = dynamic_cast<ast::Identifier *>(call->getExpression());

            auto fn = context->get(name->getValue());
            if (fn == nullptr)
                throw std::runtime_error("No function with give name exists.");

            if (auto fnc = std::dynamic_pointer_cast<Function>(fn); fnc != nullptr)
            {
                auto fn_ctx = new Context("<function " + name->getValue() + ">", context, true);

                auto args = call->getArguments();
                auto params = fnc->getParams();

                if (args.size() != params.size())
                {
                    throw std::runtime_error("Given params does not function sig.");
                }
                // set arguments.
                for (std::size_t i = 0; i < args.size(); i++)
                {
                    auto param = params.at(i);
                    auto arg = args.at(i);

                    auto var = visitExpression(arg, context);

                    auto typedata = dynamic_cast<ast::Identifier *>(param->getType());
                    if (typedata == nullptr)
                        throw std::runtime_error("Unable to detrmine type");

                    if (typedata->getValue() == "string" && var->getKind() == consts::ID_STRING)
                    {
                        fn_ctx->set(param->getName()->getValue(), var);
                    }
                    else if (typedata->getValue() == "number" && var->getKind() == consts::ID_NUMBER)
                    {
                        fn_ctx->set(param->getName()->getValue(), var);
                    }
                    else
                    {
                        throw std::runtime_error("Invalid type");
                    }
                }

                // set params
                auto result = visitStatements(fnc->getBody()->getStatements(), fn_ctx);

                return result.first;
            }
            else if (auto ifn = std::dynamic_pointer_cast<InternalFunction>(fn); ifn != nullptr)
            {
                std::vector<std::shared_ptr<Object>> args;

                for (auto &&i : call->getArguments())
                {
                    args.push_back(visitExpression(i, context));
                }

                return ifn->execute(args);
            }

            throw std::runtime_error("Failed to execute function");
        }
        else if (auto num = dynamic_cast<ast::NumericLiteral *>(value); num != nullptr)
        {
            return std::shared_ptr<Number>(new Number(num->getValue()));
        }
        else if (auto str = dynamic_cast<ast::StringLiteral *>(value); str != nullptr)
        {
            return std::shared_ptr<String>(new String(str->getValue()));
        }
        else if (auto idnt = dynamic_cast<ast::Identifier *>(value); idnt != nullptr)
        {
            auto r = context->get(idnt->getValue());

            if (r == nullptr)
                throw std::runtime_error("No variable exsists");

            return r;
        }
        else
        {
            throw std::runtime_error("Unknown expression.");
        }
    }

    void Runtime::visitVariableDeclaration(ast::VariableDeclaration *value, Context *context)
    {
        std::string name = value->getName()->getValue();
        std::string type = value->getType()->getValue();

        auto init = value->getInitalizer();

        if (init == nullptr)
        {
            if (type == "string")
            {
                context->set(name, std::shared_ptr<String>());
            }
            else if (type == "number")
            {
                context->set(name, std::shared_ptr<Number>());
            }
            else
            {
                throw std::runtime_error("Unsupported type");
            }
            return;
        }

        context->set(name, visitExpression(init, context));
    }

    void Runtime::visitVariableStatement(ast::VariableStatement *value, Context *context)
    {
        for (auto &&i : value->getDeclarations())
        {
            visitVariableDeclaration(i, context);
        }
    }

    std::pair<std::shared_ptr<Object>, bool> Runtime::visitIfStatement(ast::IfStatement *value, Context *context)
    {
        auto result = visitExpression(value->getExpression(), context);

        if (auto exp = std::dynamic_pointer_cast<Number>(result); exp != nullptr)
        {
            if (exp->asBool())
            {
                auto if_ctx = new Context("<if>", context, context->canReturn());
                return visitStatements(value->getThen()->getStatements(), if_ctx);
            }
        }

        auto elseBlock = value->getElse();

        if (elseBlock == nullptr)
        {
            return std::make_pair(std::shared_ptr<Null>(new Null()), false);
        }

        if (auto block = dynamic_cast<ast::Block *>(elseBlock); block != nullptr)
        {
            auto if_ctx = new Context("<if>", context, context->canReturn());
            return visitStatements(block->getStatements(), if_ctx);
        }

        if (auto elseif = dynamic_cast<ast::IfStatement *>(elseBlock); elseif != nullptr)
        {
            return visitIfStatement(elseif, context);
        }

        return std::make_pair(std::shared_ptr<Null>(new Null()), false);
    }

    void Runtime::visitFunctionDeclartion(ast::FunctionDeclartion *value, Context *context)
    {
        auto fn = std::shared_ptr<Function>(new Function(value->getName(), value->getBodyBlock(), value->getParameters()));

        value->setBody(nullptr);
        value->clearParams();

        if (context->has(value->getName()))
        {
            throw std::runtime_error("A variable already exists with this name.");
        }

        context->set(value->getName(), fn);
    }

} // namespace jit
