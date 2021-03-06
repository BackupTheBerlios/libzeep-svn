//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEEP_XML_DOCTYPE_HPP
#define ZEEP_XML_DOCTYPE_HPP

#include <set>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem/operations.hpp>

namespace zeep { namespace xml { namespace doctype {

// --------------------------------------------------------------------
// doctype support with full validation.

class element;
class attlist;
class entity;
class attribute;

typedef boost::ptr_vector<entity>		entity_list;
typedef boost::ptr_vector<element>		element_list;
typedef boost::ptr_vector<attribute>	attribute_list;

// --------------------------------------------------------------------
// validation of elements is done by the validator classes

class allowed_base;
typedef boost::shared_ptr<allowed_base>		allowed_ptr;
typedef std::list<allowed_ptr>				allowed_list;

class validator_imp;
typedef boost::shared_ptr<validator_imp>	validator_imp_ptr;

struct state_base;
typedef boost::shared_ptr<state_base>	state_ptr;

class validator
{
  public:
						validator();
						
						validator(allowed_ptr allowed);

						validator(const validator& other);
	validator&			operator=(const validator& other);

	void				reset();
	bool				allow(const std::wstring& name);
	bool				allow_char_data();
	bool				done();

	bool				operator()(const std::wstring& name)		{ return allow(name); }

  private:

	friend std::ostream& operator<<(std::ostream& lhs, validator& rhs);

	state_ptr			m_state;
	allowed_ptr			m_allowed;
	int					m_nr;
	static int			s_next_nr;
	bool				m_done;
};

std::ostream& operator<<(std::ostream& lhs, validator& rhs);

struct allowed_base
{
						allowed_base() {}
	virtual				~allowed_base() {}

	virtual state_ptr	create_state() const = 0;
	virtual bool		element_content() const			{ return false; }

	virtual void		print(std::ostream& os) = 0;
};

struct allowed_any : public allowed_base
{
	virtual state_ptr	create_state() const;
	virtual void		print(std::ostream& os);
};

struct allowed_empty : public allowed_base
{
	virtual state_ptr	create_state() const;
	virtual void		print(std::ostream& os);
};

struct allowed_element : public allowed_base
{
						allowed_element(const std::wstring& name)
							: m_name(name) {}

	virtual state_ptr	create_state() const;
	virtual bool		element_content() const			{ return true; }

	virtual void		print(std::ostream& os);

	std::wstring		m_name;	
};

struct allowed_repeated : public allowed_base
{
						allowed_repeated(allowed_ptr allowed, char repetion)
							: m_allowed(allowed), m_repetition(repetion)
						{
							assert(allowed);
						}

	virtual state_ptr	create_state() const;
	virtual bool		element_content() const;

	virtual void		print(std::ostream& os);

	allowed_ptr			m_allowed;
	char				m_repetition;
};

struct allowed_seq : public allowed_base
{
						allowed_seq(const allowed_list& allowed)
							: m_allowed(allowed) {}

	virtual state_ptr	create_state() const;
	virtual bool		element_content() const;

	virtual void		print(std::ostream& os);

	allowed_list		m_allowed;
};

struct allowed_choice : public allowed_base
{
						allowed_choice(const allowed_list& allowed, bool mixed)
							: m_allowed(allowed), m_mixed(mixed) {}

	virtual state_ptr	create_state() const;
	virtual bool		element_content() const;

	virtual void		print(std::ostream& os);

	allowed_list		m_allowed;
	bool				m_mixed;
};

// --------------------------------------------------------------------

enum AttributeType
{
	attTypeString,
	attTypeTokenizedID,
	attTypeTokenizedIDREF,
	attTypeTokenizedIDREFS,
	attTypeTokenizedENTITY,
	attTypeTokenizedENTITIES,
	attTypeTokenizedNMTOKEN,
	attTypeTokenizedNMTOKENS,
	attTypeNotation,
	attTypeEnumerated
};

enum AttributeDefault
{
	attDefNone,
	attDefRequired,
	attDefImplied,
	attDefFixed,
	attDefDefault
};

class attribute : public boost::noncopyable
{
  public:
						attribute(const std::wstring& name, AttributeType type)
							: m_name(name), m_type(type), m_default(attDefNone), m_external(false) {}

						attribute(const std::wstring& name, AttributeType type,
								const std::vector<std::wstring>& enums)
							: m_name(name), m_type(type), m_default(attDefNone)
							, m_enum(enums), m_external(false) {}

	std::wstring		name() const							{ return m_name; }

	bool				validate_value(std::wstring& value, const entity_list& entities) const;
	
	void				set_default(AttributeDefault def, const std::wstring& value)
						{
							m_default = def;
							m_default_value = value;
						}

	boost::tuple<AttributeDefault,std::wstring>
						get_default() const						{ return boost::make_tuple(m_default, m_default_value); }
	
	AttributeType		get_type() const						{ return m_type; }
	AttributeDefault	get_default_type() const				{ return m_default; }
	const std::vector<std::wstring>&
						get_enums() const						{ return m_enum; }

	void				external(bool external)						{ m_external = external; }
	bool				external() const							{ return m_external; }
	
  private:

	// routines used to check _and_ reformat attribute value strings
	bool				is_name(std::wstring& s) const;
	bool				is_names(std::wstring& s) const;
	bool				is_nmtoken(std::wstring& s) const;
	bool				is_nmtokens(std::wstring& s) const;

	bool				is_unparsed_entity(const std::wstring& s, const entity_list& l) const;

	std::wstring		m_name;
	AttributeType		m_type;
	AttributeDefault	m_default;
	std::wstring		m_default_value;
	std::vector<std::wstring>
						m_enum;
	bool				m_external;
};

// --------------------------------------------------------------------

class element : boost::noncopyable
{
  public:
						element(const std::wstring& name, bool declared, bool external)
							: m_name(name), m_declared(declared), m_external(external) {}

						~element();

	void				add_attribute(std::auto_ptr<attribute> attr);
	
	const attribute*	get_attribute(const std::wstring& name) const;

	std::wstring		name() const								{ return m_name; }
	
	const attribute_list&
						attributes() const							{ return m_attlist; }

	void				set_allowed(allowed_ptr allowed)			{ m_allowed = allowed; }

	void				declared(bool declared)						{ m_declared = declared; }
	bool				declared() const							{ return m_declared; }

	void				external(bool external)						{ m_external = external; }
	bool				external() const							{ return m_external; }

	bool				empty() const;
	bool				element_content() const;

	validator			get_validator() const;

  private:

	std::wstring		m_name;
	attribute_list		m_attlist;
	allowed_ptr			m_allowed;
	bool				m_declared, m_external;
};

// --------------------------------------------------------------------

class entity : boost::noncopyable
{
  public:
	const std::wstring&	name() const							{ return m_name; }		
	const std::wstring&	replacement() const						{ return m_replacement; }		
	const boost::filesystem::path&
						path() const							{ return m_path; }		
	bool				parameter() const						{ return m_parameter; }		

	bool				parsed() const							{ return m_parsed; }
	void				parsed(bool parsed)						{ m_parsed = parsed; }

	const std::wstring&	ndata() const							{ return m_ndata; }
	void				ndata(const std::wstring& ndata)		{ m_ndata = ndata; }

	bool				external() const						{ return m_external; }

	bool				externally_defined() const				{ return m_externally_defined; }
	void				externally_defined(bool externally_defined)
																{ m_externally_defined = externally_defined; }

  protected:
						entity(const std::wstring& name, const std::wstring& replacement,
								bool external, bool parsed)
							: m_name(name), m_replacement(replacement), m_parameter(false), m_parsed(parsed), m_external(external)
							, m_externally_defined(false) {}

						entity(const std::wstring& name, const std::wstring& replacement,
								const boost::filesystem::path& path)
							: m_name(name), m_replacement(replacement), m_path(path), m_parameter(true), m_parsed(true), m_external(true)
							, m_externally_defined(false) {}

	std::wstring		m_name;
	std::wstring		m_replacement;
	std::wstring		m_ndata;
	boost::filesystem::path
						m_path;
	bool				m_parameter;
	bool				m_parsed;
	bool				m_external;
	bool				m_externally_defined;
};

class general_entity : public entity
{
  public:
						general_entity(const std::wstring& name, const std::wstring& replacement,
								bool external = false, bool parsed = true)
							: entity(name, replacement, external, parsed) {}
};

class parameter_entity : public entity
{
  public:
						parameter_entity(const std::wstring& name, const std::wstring& replacement,
								const boost::filesystem::path& path)
							: entity(name, replacement, path) {}
};
	
}
}
}

#endif
