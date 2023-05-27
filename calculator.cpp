#include "calculator.h"


using namespace calculator;


inline void Token::clear() noexcept {
	raw.clear(); type = Type::none_t, data;
};


inline bool Token::empty() const noexcept {
	return raw.empty();
};


inline bool Token::is_operator() const noexcept {
	for (auto& opr : { "~", "-", "+", "*", "/", "^", "=", ",", "(", ")" })
		if (opr == raw) return true;
	return false;
}


inline bool Token::is_constant() const noexcept {
	static const auto lambda = [](int c) { return isdigit(c) || c == '.'; };
	return std::all_of(raw.begin(), raw.end(), lambda);
}


inline bool Token::is_literal() const noexcept {
	if (raw.empty()) return false;
	return (raw.front() == '$' || isalpha(raw.front())) && std::all_of(std::next(raw.begin()), raw.end(), std::isalnum);
}


Token::Type Token::what_type() const noexcept {
	if (is_operator()) return Type::operator_t;
	if (is_constant()) return Type::constant_t;
	if (is_literal()) return Type::variable_t;
	return Type::none_t;
}


int calculator::Token::priority() const noexcept {
	if (raw == "~") return 40;
	if (raw == "^") return 30;
	if (raw == "*" || raw == "/") return 20;
	if (raw == "+" || raw == "-") return 10;
	if (raw == "(" || raw == ")") return -1;
	if (raw == "=") return -2;
	return 0;
}


Expression::iterator calculator::skip_brackets(Expression::iterator begin, Expression::iterator end) {
	int counter = 1;
	if (begin->raw != "(") return begin;
	else begin++;

	while (begin != end && counter) {
		if (begin->raw == "(") counter++;
		if (begin->raw == ")") counter--;
		begin++;
	}
	return begin;
};


Expression::iterator calculator::read_func_argv(Expression::iterator t_begin, Expression::iterator t_end, std::list<Expression>& argv) {
	auto end = std::prev(skip_brackets(t_begin, t_end));
	auto begin = std::next(t_begin);


	Expression arg;
	while (begin != end && begin != t_end) {

		if (begin->raw == ",") {
			argv.push_back(Expression(arg));
			arg.clear();
		}
		else if (begin->raw == "(") {
			auto _end = skip_brackets(begin, end);
			arg.insert(arg.end(), begin, _end);
			begin = std::prev(_end);
		}
		else arg.push_back(*begin);

		if (++begin == end) argv.push_back(arg);
	}

	return begin;
}


void calculator::optimisation(Expression& expr) {
	auto begin = expr.begin();
	auto end = expr.end();

	auto step_back = [&expr, &begin]() {
		if (std::distance(expr.begin(), begin) >= 2)
			std::advance(begin, -2);
	};

	while (begin != end) {
		auto& a = *begin;
		if (std::next(begin) == end) break;

		auto& b = *std::next(begin);

		// insert implicit multiplications
		if ((a.is_constant() && (b.is_literal() || b.raw == "(")) ||
			(a.raw == ")" && (b.is_literal() || b.is_constant())) ||
			(a.raw == ")" && b.raw == "(")) {
			expr.insert(std::next(begin), { "*" });
			step_back();
		}

		// identification unary plus and minus
		if (a.is_operator() && a.raw != "(" && a.raw != ")") {
			if (b.raw == "+") {
				expr.erase(std::next(begin));
				step_back();
			}
			if (b.raw == "-") b.raw = "~";
		}

		// erase minus and plus duplications
		if ((a.raw == "-" && (b.raw == "-" || b.raw == "~")) ||
			(a.raw == "~" && (b.raw == "-" || b.raw == "~")) ||
			(a.raw == "+" && b.raw == "+")) {
			expr.erase(std::next(begin));
			a.raw = "+";
			step_back();
		}
		if ((a.raw == "+" && (b.raw == "-" || b.raw == "~")) ||
			((a.raw == "-" || a.raw == "~") && b.raw == "+")) {
			expr.erase(std::next(begin));
			a.raw = (b.raw == "~" || a.raw == "~") ? "~" : "-";
			step_back();
		};

		begin++;
	}
	return;
}


Expression calculator::read_expr(const std::string& expr) {
	Expression tokens;

	// reading tokens
	Token tkn;
	for (auto chr_iter = expr.begin(); chr_iter != expr.end(); chr_iter++) {
		auto& chr = *chr_iter;

		if (isdigit(chr) || chr == '.') {
			if (!(tkn.empty() || isalnum(tkn.raw.back()) || tkn.raw.back() == '.')) {
				tokens.push_back(tkn);
				tkn.clear();
			}
			tkn.raw += chr;
		}

		else if (isalpha(chr)) {
			if (!(tkn.empty() || isalpha(tkn.raw.back()))) {
				tokens.push_back(tkn);
				tkn.clear();
			}
			tkn.raw += chr;
		}

		else if (ispunct(chr)) {
			if (!tkn.empty()) {
				tokens.push_back(tkn);
				tkn.clear();
			}
			tkn.raw = chr;
			tokens.push_back(tkn);
			tkn.clear();
		}
		else throw std::invalid_argument(std::string("Invalid symbol '") + chr + "(" + std::to_string(chr) + ")");
	}
	if (!tkn.empty())
		tokens.push_back(tkn);
	optimisation(tokens);

	// typing tokens
	for (auto tkn_iter = tokens.begin(); tkn_iter != tokens.end(); tkn_iter++)
		tkn_iter->type = tkn_iter->what_type();

	// decorating a function
	for (auto tkn_iter = tokens.begin(); tkn_iter != tokens.end(); tkn_iter++) {
		auto& a = *tkn_iter;

		if (std::distance(tkn_iter, tokens.end()) < 2) break;
		auto& b = *std::next(tkn_iter);

		if (a.is_literal() && b.raw == "(")
			a.type = Token::Type::function_t;

		else if (a.raw == "$" && b.is_constant()) {
			a.raw += b.raw;
			a.type = Token::Type::variable_t;
			tokens.erase(std::next(tkn_iter));
		}
	}

	return tokens;
}


Expression calculator::transform_expr(Expression::iterator begin, Expression::iterator end) {
	Expression rpn;
	std::stack<Token > stack;

	while (begin != end) {
		if (begin->is_constant() || begin->is_literal()) {
			auto opr = *begin;

			if (begin->type == Token::Type::function_t) {
				std::list<Expression> argv;
				auto _end = std::next(read_func_argv(std::next(begin), end, argv));

				Token argc = { std::to_string(argv.size()), Token::Type::argc_t, (size_t)argv.size() };

				for (auto& expr : argv) {
					auto a = transform_expr(expr.begin(), expr.end());
					rpn.insert(rpn.end(), a.begin(), a.end());
				}
				rpn.push_back(argc);
				begin = std::prev(_end);
			}
			rpn.push_back(opr);
		}

		else if (begin->raw == "(") {
			stack.push(*begin);
		}

		else if (begin->raw == ")") {
			while (stack.size() && stack.top().raw != "(") {
				rpn.push_back(stack.top());
				stack.pop();
			}
			if (!stack.empty()) stack.pop();
		}

		else if (begin->is_operator()) {
			while (stack.size() && begin->priority() <= stack.top().priority()) {
				rpn.push_back(stack.top());
				stack.pop();
			}
			stack.push(*begin);
		}
		else throw std::invalid_argument("Unknown token '" + begin->raw + "'");

		begin++;
	}

	while (stack.size()) {
		rpn.push_back(stack.top());
		stack.pop();
	}

	return rpn;
}


// refresh must be true at first call
double calculator::calc(const Expression& expr, const Definition& locals, Definition& globals, bool refresh = false) {
	static unsigned long long recursion_depth = 0;
	if (refresh) recursion_depth = 0;
	else recursion_depth += 1;

	if (recursion_depth > MAX_CALC_RECURSION_DEPTH)
		throw std::overflow_error("Recursion limit reached");

	if (std::max(globals.size(), locals.size()) > MAX_DEFINITIONS_SIZE)
		throw std::overflow_error("Definition limit reached");

	if (expr.empty())
		throw std::invalid_argument("Empty expression");

	auto tod = [](Token& token, double d) {
		token.data._val = d;
		token.raw = std::to_string(token.data._val);
		token.type = Token::Type::constant_t;
	};

	auto tokens = expr;
	auto begin = tokens.begin();
	auto end = tokens.end();

	while (begin != end) {

		if (begin->type == Token::Type::constant_t) {
			if (!begin->data._val)
				begin->data._val = std::stod(begin->raw);
		}

		else if (begin->type == Token::Type::variable_t) {

			//------------- add here custom constants -------------
			if (begin->raw == "e") tod(*begin, 2.718281);
			else if (begin->raw == "pi") tod(*begin, 3.141592);
			else if (begin->raw == "tau") tod(*begin, 6.283185);
			else if (begin->raw == "phi") tod(*begin, 1.618033);
			// ----------------------------------------------------

			else if (locals.find(begin->raw) != locals.end())
				tod(*begin, calc(locals.at(begin->raw), locals, globals));

			else if (globals.find(begin->raw) != globals.end()) {
				if (globals.at(begin->raw).size())
					tod(*begin, calc(globals.at(begin->raw), locals, globals));
			}

			else throw std::invalid_argument("Undefined variable '" + begin->raw + "'");
		}

		else if (begin->type == Token::Type::function_t) {
			auto _end = std::prev(begin);
			auto _begin = _end;

			if (std::distance(tokens.begin(), _begin) < _end->data._val)
				throw std::invalid_argument("Empty argument for '" + begin->raw + "'");
			else std::advance(_begin, -_end->data._val);

			Definition _locals;
			for (auto iter = _begin; iter != _end; iter++)
				_locals["$" + std::to_string(std::distance(_begin, iter))] = { *iter };

			//----------------------------- add here custom functions -----------------------------
			// Exapple for "sum(a, b) = a + b":
			// if (begin->raw == "sum") {
			//     auto arg1 = calc(_locals["$0"], locals, globals);
			//     auto arg2 = calc(_locals["$1"], locals, globals);
			//     tod(*begin, arg1 + arg2); 
			// };

			if (begin->raw == "sin")       tod(*begin, std::sin(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "cos")  tod(*begin, std::cos(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "tg")   tod(*begin, std::tan(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "ctg")  tod(*begin, 1.0 / std::tan(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "sh")   tod(*begin, std::sinh(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "ch")   tod(*begin, std::cosh(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "th")   tod(*begin, std::tanh(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "cth")  tod(*begin, 1.0 / std::tanh(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "exp")  tod(*begin, std::exp(calc(_locals["$0"], locals, globals)));
			else if (begin->raw == "sqrt") tod(*begin, std::sqrt(calc(_locals["$0"], locals, globals)));
			//--------------------------------------------------------------------------------------

			else if (globals.find(begin->raw) != globals.end()) {
				tod(*begin, calc(globals.at(begin->raw), _locals, globals));
			}

			else throw  std::invalid_argument("Undefined function '" + begin->raw + "'");


			tokens.erase(_begin, std::next(_end));
		}

		else if (begin->type == Token::Type::operator_t) {
			int argc = (begin->raw == "~") ? 1 : 2;

			if (std::distance(tokens.begin(), begin) < argc)
				throw std::invalid_argument("Invalid operation arguments for '" + begin->raw + "'");

			if (argc == 1) {
				auto _begin = std::prev(begin);
				auto a = calc({ _begin, begin }, locals, globals);
				tokens.erase(_begin);

				if (begin->raw == "~") tod(*begin, -a);
				else throw std::invalid_argument("Unknown unary operation '" + begin->raw + "'");
			}
			else {
				auto _begin = begin;
				std::advance(_begin, -argc);
				auto a = calc(Expression(_begin, std::next(_begin)), locals, globals);
				auto b = calc(Expression(std::next(_begin), begin), locals, globals);

				if (begin->raw == "=") {
					if (!_begin->is_literal()) throw std::invalid_argument("Impossible assignment for '" + _begin->raw + "'");
					auto res = *begin;
					tod(res, b);
					globals[_begin->raw] = { res };
					*begin = { _begin->raw, Token::Type::variable_t, res.data._val };
				}
				else if (begin->raw == "+") tod(*begin, a + b);
				else if (begin->raw == "-") tod(*begin, a - b);
				else if (begin->raw == "*") tod(*begin, a * b);
				else if (begin->raw == "/") tod(*begin, a / b);
				else if (begin->raw == "^") tod(*begin, std::pow(a, b));
				else throw std::invalid_argument("Unknown binary operation '" + begin->raw + "'");

				tokens.erase(_begin, begin);
			}
		}

		begin++;
	}

	return tokens.front().data._val;
}


// the most important function
std::string calculator::evaluate(std::string expr, Definition& globals) {
	expr.erase(std::remove_if(expr.begin(), expr.end(), isspace), expr.end());

	if (expr.empty()) return "NAN";

	try {
		auto tokens = read_expr(expr);

		// function definition
		if (tokens.front().type == Token::Type::function_t) {
			std::list<Expression> argv;
			auto _begin = read_func_argv(std::next(tokens.begin()), tokens.end(), argv);
			if (_begin != tokens.end()) _begin++;
			else throw std::invalid_argument("Invalid expression");

			if (_begin != tokens.end() && _begin->raw == "=") {
				auto body = transform_expr(std::next(_begin), tokens.end());
				auto argc = 0;

				for (auto& arg : argv) {
					auto lambda = [&arg](Token token) {return arg.front().raw == token.raw; };
					auto pos = std::find_if(body.begin(), body.end(), lambda);
					bool one_or_more = 0;

					while (pos != body.end()) {
						one_or_more = true;
						*pos = { "$" + std::to_string(argc), Token::Type::variable_t };
						pos = std::find_if(body.begin(), body.end(), lambda);
					}
					if (one_or_more) argc++;
				}
				globals[tokens.front().raw] = body;
				return tokens.front().raw;
			}
		}

		// variable definition
		else if (tokens.front().type == Token::Type::variable_t) {
			if (std::next(tokens.begin()) != tokens.end() && std::next(tokens.begin())->raw == "=") {
				globals[tokens.front().raw] = {};
			}
		}

		auto expr = transform_expr(tokens.begin(), tokens.end());
		return std::to_string(calc(expr, {}, globals, true));

	}
	catch (const std::exception& err) {
		std::cout << "Error: " << err.what() << std::endl;
		return err.what();
	}

	return "NAN";
}