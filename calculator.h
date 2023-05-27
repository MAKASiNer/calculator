#pragma once
#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <stack>
#include <string>

#ifndef MAX_CALC_RECURSION_DEPTH
#define MAX_CALC_RECURSION_DEPTH 0x400
#endif

#ifndef MAX_DEFINITIONS_SIZE
#define MAX_DEFINITIONS_SIZE 0x100
#endif 

namespace calculator {
	struct Token {
		enum Type {
			none_t = 0,
			constant_t,
			operator_t,
			variable_t,
			function_t,
			argc_t,
		};

		std::string raw;
		Type type{ (Type)0 };
		union Data { double _val = 0; } data;

		inline void clear() noexcept;
		inline bool empty() const noexcept;
		inline bool is_operator() const noexcept;
		inline bool is_constant() const noexcept;
		inline bool is_literal() const noexcept;
		Type what_type() const noexcept;
		int priority() const noexcept;
	};

	typedef std::list<Token> Expression;
	typedef std::map<std::string, Expression> Definition;

	Expression::iterator skip_brackets(Expression::iterator, Expression::iterator);

	Expression::iterator read_func_argv(Expression::iterator, Expression::iterator, std::list<Expression>&);

	void optimisation(Expression&);

	Expression read_expr(const std::string&);

	Expression transform_expr(Expression::iterator, Expression::iterator);

	double calc(const Expression&, const Definition&, Definition&, bool);

	std::string evaluate(std::string, calculator::Definition&);
};