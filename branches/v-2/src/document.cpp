//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
#include <deque>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"

#include "zeep/xml/parser.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace xml {

// --------------------------------------------------------------------

struct document_imp
{
	node_ptr		m_root;

	stack<node_ptr>	cur;		// construction
	vector<pair<string,string> >
					namespaces;

	void			StartElementHandler(const string& name, const attribute_list& atts);

	void			EndElementHandler(const string& name);

	void			CharacterDataHandler(const string& data);

	void			ProcessingInstructionHandler(const string& target, const string& data);

	void			CommentHandler(const string& comment);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(const string& prefix, const string& uri);

	void			EndNamespaceDeclHandler(const string& prefix);

	void			parse(istream& data);

	void			parse_name(const string& name, string& element, string& ns, string& prefix);
};

// --------------------------------------------------------------------

void document_imp::parse_name(
	const string&		name,
	string&				element,
	string&				ns,
	string&				prefix)
{
	vector<string> n3;
	ba::split(n3, name, ba::is_any_of("="));

	if (n3.size() == 3)
	{
		element = n3[1];
		ns = n3[0];
		prefix = n3[2];
	}
	else if (n3.size() == 2)
	{
		element = n3[1];
		ns = n3[0];
		prefix.clear();
		
		if (ns.empty() == false and not cur.empty())
			prefix = cur.top()->find_prefix(ns);
	}
	else
	{
		element = n3[0];
		ns.clear();
		prefix.clear();
	}
}

void document_imp::StartElementHandler(const string& name, const attribute_list& atts)
{
	string element, ns, prefix;
	
	parse_name(name, element, ns, prefix);

	node_ptr n;
	
	n.reset(new node(element, ns, prefix));

	if (cur.empty())
	{
		cur.push(n);
		m_root = cur.top();
	}
	else
	{
		cur.top()->add_child(n);
		cur.push(n);
	}
	
	for (attribute_list::const_iterator ai = atts.begin(); ai != atts.end(); ++ai)
	{
		parse_name(ai->name(), element, ns, prefix);
		if (not prefix.empty())
			element = prefix + ':' + element;
		
		attribute_ptr attr(new xml::attribute(element, ai->value()));
		cur.top()->add_attribute(attr);
	}

	const string name_prefix("xmlns:");

	for (vector<pair<string,string> >::iterator ns = namespaces.begin(); ns != namespaces.end(); ++ns)
	{
		string name;
		if (ns->first.empty())
			name = "xmlns";
		else
			name = name_prefix + ns->first;

		attribute_ptr attr(new xml::attribute(name, ns->second));

		cur.top()->add_attribute(attr);
	}
	
	namespaces.clear();
}

void document_imp::EndElementHandler(const string& name)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.pop();
}

void document_imp::CharacterDataHandler(const string& data)
{
	if (cur.empty())
		throw exception("Empty stack");
	
	cur.top()->add_content(data);
}

void document_imp::ProcessingInstructionHandler(const string& target, const string& data)
{
//	cerr << "processing instruction, target: " << target << ", data: " << data << endl;
}

void document_imp::CommentHandler(const string& comment)
{
//	cerr << "comment " << data << endl;
}

void document_imp::StartCdataSectionHandler()
{
//	cerr << "start cdata" << endl;
}

void document_imp::EndCdataSectionHandler()
{
//	cerr << "end cdata" << endl;
}

void document_imp::StartNamespaceDeclHandler(const string& prefix, const string& uri)
{
	namespaces.push_back(make_pair(prefix, uri));
}
	
void document_imp::EndNamespaceDeclHandler(const string& prefix)
{
}

// --------------------------------------------------------------------

void document_imp::parse(
	istream&		data)
{
	parser p(data);

	p.start_element_handler = boost::bind(&document_imp::StartElementHandler, this, _1, _2);
	p.end_element_handler = boost::bind(&document_imp::EndElementHandler, this, _1);
	p.character_data_handler = boost::bind(&document_imp::CharacterDataHandler, this, _1);
	p.start_namespace_decl_handler = boost::bind(&document_imp::StartNamespaceDeclHandler, this, _1, _2);

	p.parse();
}

// --------------------------------------------------------------------

document::document(
	istream&		data)
	: impl(new document_imp)
{
	impl->parse(data);
}

document::document(
	const string&	data)
	: impl(new document_imp)
{
	stringstream s;
	s.str(data);
	impl->parse(s);
}

document::document(
	node_ptr		data)
	: impl(new document_imp)
{
	impl->m_root = data;
}
					
document::~document()
{
	delete impl;
}

node_ptr document::root() const
{
	return impl->m_root;
}

ostream& operator<<(ostream& lhs, const document& rhs)
{
	lhs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	
	if (rhs.root())
		rhs.root()->write(lhs, 0);

	return lhs;
}
	
} // xml
} // zeep