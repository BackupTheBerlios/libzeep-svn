#!/usr/bin/perl
# the first line of the script must tell us which language interpreter to use,
# in this case its perl

use strict;
use warnings;
use Data::Dumper;
use LWP::Simple;

# import the modules we need for this test; XML::Compile is included on the server
# by default.
use XML::Compile::WSDL11;
use XML::Compile::SOAP11;
use XML::Compile::Transport::SOAPHTTP;

eval
{	
	# Retrieving and processing the WSDL
	my $wsdl = get('http://localhost:10333/wsdl');
	
	my $fh;
	open($fh, "<", \$wsdl);
	my $services  = XML::LibXML->new->parse_fh($fh);
	close($fh);

	my $proxy = XML::Compile::WSDL11->new($services);
	
	# Generating a request message based on the WSDL
	my $count = $proxy->compileClient('Count');

	# Calling the service and getting the response
	my ($answer, $trace) = $count->( db => 'sprot', booleanquery => 'os:human' );

	# If the response arrived, look for a specific value that is supposed to be larger than 0
	# If the value is larger than zero, return 0 because the test passed.
	# If the value is something else, return 2 to indicate a warning.
	# If no answer has arrived, return 1 to indicate the test failed.
	if ( defined $answer ) {
		if ($answer->{parameters}->{response}->is_int() and 
			$answer->{parameters}->{response}->is_pos() and 
			not $answer->{parameters}->{response}->is_zero()) {
			print "Passed\n";
			exit 0;
		} else {
			print "Unexpected data\n";
			exit 2;
		}
	} else {    
		print "Failed\n";
		exit 1;
	}
};

if ($@)
{
	print "Caught an exception: $@\n";
	exit 1;
}
