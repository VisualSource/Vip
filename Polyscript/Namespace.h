#pragma once
#include <string>
#include "Variable.h"

class Context;

class Namespace : public Variable {
	public:
		Namespace(std::string name, Context* ctx);
		~Namespace();
		virtual Namespace& SetContext(Context* context) override;
		virtual Namespace& SetPostion(Position start, Position end) override;
		friend std::ostream& operator<<(std::ostream& lhs, const Namespace& other);
		Context* GetNamespaceCtx() const;
		std::string GetName() const;
	private:
		Context* nsctx = nullptr;
		std::string name;

};
