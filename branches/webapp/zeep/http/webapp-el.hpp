// --------------------------------------------------------------------
//
// expression language support
//

#pragma once

typedef int8_t		int8;
typedef uint8_t		uint8;
typedef int16_t		int16;
typedef uint16_t	uint16;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;

namespace zeep {
namespace http {
namespace el
{
	
typedef uint32	unicode;

struct object
{
	enum object_type
	{
		ot_undef,
		ot_number,
		ot_string,
		ot_struct,
		ot_array,
		ot_boolean
	}				m_type;
	std::string		m_string;
	double			m_number;
	std::map<std::string,object>
					m_fields;
	std::vector<object>	m_array;
	
					object()					: m_type(ot_undef) {}
					object(const object& o)
						: m_type(o.m_type)
					{
						switch (m_type)
						{
							case ot_string:		m_string = o.m_string; break;
							case ot_boolean:
							case ot_number:		m_number = o.m_number; break;
							case ot_struct:		m_fields = o.m_fields; break;
							case ot_array:		m_array = o.m_array;
							default:			break;
						}
					}

	explicit		object(const char* s)		: m_type(ot_string), m_string(s ? s : "") {}
	explicit		object(const std::string& s)		: m_type(ot_string), m_string(s) {}
	explicit		object(double n)			: m_type(ot_number), m_number(n) {}
	explicit		object(int8 n)				: m_type(ot_number), m_number(n) {}
	explicit		object(uint8 n)				: m_type(ot_number), m_number(n) {}
	explicit		object(int16 n)				: m_type(ot_number), m_number(n) {}
	explicit		object(uint16 n)			: m_type(ot_number), m_number(n) {}
	explicit		object(int32 n)				: m_type(ot_number), m_number(n) {}
	explicit		object(uint32 n)			: m_type(ot_number), m_number(n) {}
	explicit		object(int64 n)				: m_type(ot_number), m_number(n) {}
	explicit		object(uint64 n)			: m_type(ot_number), m_number(n) {}
	explicit		object(bool b)				: m_type(ot_boolean), m_number(b) {}
	explicit		object(const std::vector<object>& a)
												: m_type(ot_array), m_array(a) {}

					object(object_type type)	: m_type(type) {}

	bool			empty() const
					{
						bool result = false;
						switch (m_type)
						{
							case ot_string:		result = m_string.empty(); break;
							case ot_array:		result = m_array.empty(); break;
							case ot_undef:		result = true; break;
							default:			break;
						}
						return result;
					}

	object&			operator=(const object& rhs)
					{
						m_type = rhs.m_type;
						switch (m_type)
						{
							case ot_string:		m_string = rhs.m_string; break;
							case ot_boolean:
							case ot_number:		m_number = rhs.m_number; break;
							case ot_struct:		m_fields = rhs.m_fields; break;
							case ot_array:		m_array = rhs.m_array;
							default:			break;
						}
						return *this;
					}
					
	object&			operator=(const std::string& rhs)
					{
						try
						{
							m_number = boost::lexical_cast<double>(rhs);
							m_type = ot_number;
						}
						catch (...)
						{
							m_string = rhs;
							m_type = ot_string;
						}

						return *this;
					}
					
	object&			operator=(double rhs)
					{
						m_type = ot_number;
						m_number = rhs;
						return *this;
					}

	object&			operator=(bool rhs)
					{
						m_type = ot_boolean;
						m_number = rhs;
						return *this;
					}

					operator std::string() const
					{
						std::string result;
						switch (m_type)
						{
							case ot_number:		result = boost::lexical_cast<std::string>(m_number); break;
							case ot_string:		result = m_string; break;
							case ot_boolean:	result = m_number ? "true" : "false"; break;
							default:			break;
						}
						return result;
					}
					
					operator double() const
					{
						double result;
						switch (m_type)
						{
							case ot_boolean:
							case ot_number:		result = m_number; break;
							case ot_string:		result = boost::lexical_cast<double>(m_string); break;
							default:			throw zeep::exception("object is not a number");
						}
						return result;
					}

					operator bool() const
					{
						bool result;
						switch (m_type)
						{
							case ot_boolean:
							case ot_number:		result = m_number != 0; break;
							case ot_string:		result = not m_string.empty(); break;
							case ot_array:		result = not m_array.empty(); break;
							case ot_struct:		result = not m_fields.empty(); break;
							default:			result = false; break;
						}
						return result;
					}
	
	object			operator[](
						const std::string& name) const
					{
						std::map<std::string,object>::const_iterator i = m_fields.find(name);
						if (i == m_fields.end())
							throw zeep::exception((boost::format("field %1% not found") % name).str());
						return i->second;
					}

	object			operator[](
						const char* name) const
					{
						return operator[](std::string(name));
					}

	object&			operator[](
						const std::string& name)
					{
						m_type = ot_struct;
						return m_fields[name];
					}

	object&			operator[](
						const char* name)
					{
						return operator[](std::string(name));
					}

	struct compare_object
	{
					compare_object(const std::string& field, bool descending) : m_field(field), m_descending(descending) {}
		
		bool		operator()(const object& a, const object& b) const;
		
		std::string		m_field;
		bool		m_descending;
	};

	void			sort(const std::string& sort_field, bool descending)
					{
						if (m_type == ot_array)
							std::sort(m_array.begin(), m_array.end(), compare_object(sort_field, descending));
					}

};

template<class S>
S& operator<<(S& os, const object& o)
{
	switch (o.m_type)
	{
		case object::ot_undef:		os << "undef"; break;
		case object::ot_number:		os << "number(" << o.m_number << ")"; break;
		case object::ot_string:		os << "string(" << o.m_string << ")"; break;
		case object::ot_struct:
			os << "struct(";
			for (std::map<std::string,object>::const_iterator fi = o.m_fields.begin(); fi != o.m_fields.end(); ++fi)
				os << fi->first << ':' << fi->second << ", ";
			os << ")";
			break;
		case object::ot_array:
			os << "array[";
			BOOST_FOREACH (const object& f, o.m_array)
				os << f << ", ";
			os << "]";
			break;
		case object::ot_boolean:	os << "bool(" << (o.m_number ? "true)" : "false)"); break;
	}
	
	return os;
}

object operator<(const object& a, const object& b);
object operator<=(const object& a, const object& b);
object operator>=(const object& a, const object& b);
object operator>(const object& a, const object& b);
object operator!=(const object& a, const object& b);
object operator==(const object& a, const object& b);
object operator+(const object& a, const object& b);
object operator-(const object& a, const object& b);
object operator*(const object& a, const object& b);
object operator%(const object& a, const object& b);
object operator/(const object& a, const object& b);
object operator&&(const object& a, const object& b);
object operator||(const object& a, const object& b);
object operator-(const object& a);

struct scope
{
					scope(
						const scope&	next)
						: m_next(const_cast<scope*>(&next))
						, m_req(nil) {}

					scope(
						scope*			next = nil)
						: m_next(next)
						, m_req(nil) {}


	template<typename T>
	void			put(
						const std::string&	name,
						const T&		value);

	template<typename T>
	void			put(
						const std::string&	name,
						T*				begin,
						T*				end);

	object&			lookup(
						const std::string&	name)
					{
						std::map<std::string,object>::iterator i = m_data.find(name);
						if (i != m_data.end())
							return i->second;
						else if (m_next != nil)
							return m_next->lookup(name);
						
						static object s_undef;
						return s_undef;
					}

	object&			operator[](
						const std::string& name)
					{
						return lookup(name);
					}

	const request&	get_request() const
					{
						if (m_next)
							return m_next->get_request();
						if (m_req == nil)
							throw zeep::exception("Invalid scope, no request");
						return *m_req;
					}

	std::map<std::string,object>
					m_data;
	scope*			m_next;
	const request*	m_req;
};

std::ostream& operator<<(std::ostream& lhs, const scope& rhs);

template<typename T>
inline
void scope::put(
	const std::string&	name,
	const T&		value)
{
	m_data[name] = object(value);
}

template<>
inline
void scope::put(
	const std::string&	name,
	const object&	value)
{
	m_data[name] = value;
}

template<typename T>
inline
void scope::put(
	const std::string&	name,
	T*				begin,
	T*				end)
{
	std::vector<object> elements;
	while (begin != end)
		elements.push_back(object(*begin++));
	m_data[name] = object(elements);
}

struct interpreter
{
					interpreter(
						scope&			scope)
						: m_scope(scope) {}

	template<class OutputIterator, class Match>
	OutputIterator	operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type);

	object			evaluate(
						const std::string&	s);

	void			process(
						std::string&			s);
	
	void			match(
						uint32					t);

	unsigned char	next_byte();
	unicode			get_next_char();
	void			retract();
	void			get_next_token();

	object			parse_expr();					// or_expr ( '?' expr ':' expr )?
	object			parse_or_expr();				// and_expr ( 'or' and_expr)*
	object			parse_and_expr();				// equality_expr ( 'and' equality_expr)*
	object			parse_equality_expr();			// relational_expr ( ('=='|'!=') relational_expr )?
	object			parse_relational_expr();		// additive_expr ( ('<'|'<='|'>='|'>') additive_expr )*
	object			parse_additive_expr();			// multiplicative_expr ( ('+'|'-') multiplicative_expr)*
	object			parse_multiplicative_expr();	// unary_expr (('%'|'/') unary_expr)*
	object			parse_unary_expr();				// ('-')? primary_expr
	object			parse_primary_expr();			// '(' expr ')' | number | string
	
	scope&			m_scope;
	uint32			m_lookahead;
	std::string		m_token_string;
	double			m_token_number;
	std::string::const_iterator
					m_ptr, m_end;
};

template<class OutputIterator, class Match>
inline
OutputIterator interpreter::operator()(Match& m, OutputIterator out, boost::regex_constants::match_flag_type)
{
	std::string s(m[1]);

	process(s);
	
	std::copy(s.begin(), s.end(), out);

	return out;
}

}
}
}
