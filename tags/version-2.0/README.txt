This is the second release of libzeep.

The second release of libzeep introduces a full XML library. You no longer
need expat to build libzeep. The XML library comes with a full validating
SAX parser. There's also a complete XPath 1.0 implementation.

The rest of this file describes the SOAP functionality of libzeep.

It is supposed to become a very easy way to create SOAP server software in
C++. You use it to export a C++ object's methods as SOAP actions. The library
can generate a WSDL for the exported actions and it also has a REST style
interface.

libzeep uses several Boost libraries and currently requires at least version
1.36 of Boost since it uses the new asio library for network I/O. When needed,
it is possible to make a few changes to the code and use the standalone
libasio along with Boost 1.35 instead.

To test out libzeep, you have to edit the makefile and make sure the names
of the boost libraries are correct for your installation. After this you
simply type 'make zeep-test' and a 'zeep-test' executable is build.

Have a look at the zeep-test.cpp file to see how to create a server. This
example server is not entirely trivial since it has three exported methods
that each take another set of parameters.

When you run this sample server, it listens to port 10333 on your localhost.
You can access the wsdl by pointing your browser at:

http://localhost:10333/wsdl

and to access e.g. the Count method of this server from the REST interface
you can browse to:

http://localhost:10333/rest/Count/db/sprot/booleanquery/os:human

As you can see, parameters and values are passed in the URL, order is not
important and multiple id/value pairs can be specified for input parameters
that allow more than one value.

The steps to create a server are:

Create a new server object that derives from soap::server. The constructor
of this object should call the inherited constructor passing it the
namespace and the service name for this new SOAP server as well as the
internet address and port to listen to.

Inside the constructor of your new server object you have to register the
methods of the server you want to export. These methods can take any number
of input arguments and only one output parameter which is the last parameter
of the method. The result of these methods should be void.

Please note that if the method's last (output) parameter is a struct, then
the fields of this struct will be moved up in the Response message of the
action in the WSDL. To the outside world this method will look like it has
multiple output parameters. This was done to be compatible with another
popular SOAP tool but the result may be a bit confusing at times.

To register the methods you have to call the inherited 'register_action'
method which takes four parameters:

- the name of the action as it is published
- the pointer for your server object, usually it is 'this'.
- a pointer to the method of your server object you want to export
- an array of pointers to the exported names for each of the parameters
	of the exported method/action. The size of this array should be exactly
	as long as the arity of your method. You will get a compilation error
	if it isn't.

If you export enum parameters, you add the names for all possible values of 
your enumerated type by using the SOAP_XML_ADD_ENUM macro. The first parameter
should be the name of the enum type, the second the value identifier.

If you have structured types you want to export, you have to do two things.
First of all the structure needs to be able to serialize itself. You do this
by adding the method serialize to each struct. For a struct consisting of two 
fields, db and id, you specify:

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(db)
		   & BOOST_SERIALIZATION_NVP(id);
	}

The next thing you need for each struct is to set its exported name using the
SOAP_XML_SET_STRUCT_NAME macro.

And that's it. The moment the constructor is done, your server is ready to
run. You can start it by calling the 'run' method, normally you do this from
a new thread. The servers will start listening to the address and port you
specified. Beware though that the server is multithreaded and so your exported
methods should be reentrant. The number of threads the server will use can be
specified in the constructor of the soap::server base class.

If your server is behind a reverse proxy, you set the actual location in the
WSDL from which it is accessible by calling the server's set_location method.

Inside your server method you have access to the ostream object used to write
out log entries by using the inherited log() member function.

That's it.

This is a first release, please send all the problems and/or bugs you encounter
to: m.hekkelman@cmbi.ru.nl

-maarten hekkelman
