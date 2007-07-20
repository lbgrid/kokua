#!/usr/bin/python
# @file template_verifier.py
# @brief Message template compatibility verifier.
#
# Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
# $License$

"""template_verifier is a script which will compare the
current repository message template with the "master" message template, accessible
via http://secondlife.com/app/message_template/master_message_template.msg
If [FILE] is specified, it will be checked against the master template.
If [FILE] [FILE] is specified, two local files will be checked against
each other.
"""

from os.path import realpath, dirname, join, exists
setup_path = join(dirname(realpath(__file__)), "setup-path.py")
if exists(setup_path):
    execfile(setup_path)
import optparse
import os
import sys
import urllib

from indra.ipc import compatibility
from indra.ipc import tokenstream
from indra.ipc import llmessage

def getstatusall(command):
    """ Like commands.getstatusoutput, but returns stdout and 
    stderr separately(to get around "killed by signal 15" getting 
    included as part of the file).  Also, works on Windows."""
    (input, out, err) = os.popen3(command, 't')
    status = input.close() # send no input to the command
    output = out.read()
    error = err.read()
    status = out.close()
    status = err.close() # the status comes from the *last* pipe that is closed
    return status, output, error

def getstatusoutput(command):
    status, output, error = getstatusall(command)
    return status, output


def die(msg):
    print >>sys.stderr, msg
    sys.exit(1)

MESSAGE_TEMPLATE = 'message_template.msg'

PRODUCTION_ACCEPTABLE = (compatibility.Same, compatibility.Newer)
DEVELOPMENT_ACCEPTABLE = (
    compatibility.Same, compatibility.Newer,
    compatibility.Older, compatibility.Mixed)	

MAX_MASTER_AGE = 60 * 60 * 4   # refresh master cache every 4 hours

def compare(base, current, mode):
    """Compare the current template against the base template using the given
    'mode' strictness:

    development: Allows Same, Newer, Older, and Mixed
    production: Allows only Same or Newer

    Print out information about whether the current template is compatible
    with the base template.

    Returns a tuple of (bool, Compatibility)
    Return True if they are compatible in this mode, False if not.
    """

    # catch this exception so we can print a message explaining which template is scr0d
    try:
        base = llmessage.parseTemplateString(base)
    except tokenstream.ParseError, e:
        print "Error parsing master message template -- this might be a network problem, try again"
        raise e

    try:
        current = llmessage.parseTemplateString(current)
    except tokenstream.ParseError, e:
        print "Error parsing local message template"
        raise e
    
    compat = current.compatibleWithBase(base)
    if mode == 'production':
        acceptable = PRODUCTION_ACCEPTABLE
    else:
        acceptable = DEVELOPMENT_ACCEPTABLE

    if type(compat) in acceptable:
        return True, compat
    return False, compat

def fetch(url):
    if url.startswith('file://'):
        # just open the file directly because urllib is dumb about these things
        file_name = url[len('file://'):]
        return open(file_name).read()
    else:
        # *FIX: this doesn't throw an exception for a 404, and oddly enough the sl.com 404 page actually gets parsed successfully
        return ''.join(urllib.urlopen(url).readlines())   

def cache_master(master_url):
    """Using the url for the master, updates the local cache, and returns an url to the local cache."""
    master_cache = local_master_cache_filename()
    master_cache_url = 'file://' + master_cache
    # decide whether to refresh the master cache based on its age
    import time
    if (os.path.exists(master_cache)
        and time.time() - os.path.getmtime(master_cache) < MAX_MASTER_AGE):
        return master_cache_url  # our cache is fresh
    # new master doesn't exist or isn't fresh
    print "Refreshing master cache from %s" % master_url
    try:
        new_master_contents = fetch(master_url)
    except IOError, e:
        # the refresh failed, so we should just soldier on
        print "WARNING: unable to download new master, probably due to network error.  Your message template compatibility may be suspect."
        return master_cache_url
    mc = open(master_cache, 'wb')
    mc.write(new_master_contents)
    mc.close()
    return master_cache_url

def local_template_filename():
    """Returns the message template's default location relative to template_verifier.py:
    ./messages/message_template.msg."""
    d = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(d, 'messages', MESSAGE_TEMPLATE)

def local_master_cache_filename():
    """Returns the location of the master template cache relative to template_verifier.py
    ./messages/master_message_template_cache.msg"""
    d = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(d, 'messages', 'master_message_template_cache.msg')


def run(sysargs):
    parser = optparse.OptionParser(
        usage="usage: %prog [FILE] [FILE]",
        description=__doc__)
    parser.add_option(
        '-m', '--mode', type='string', dest='mode',
        default='development',
        help="""[development|production] The strictness mode to use
while checking the template; see the wiki page for details about
what is allowed and disallowed by each mode:
http://wiki.secondlife.com/wiki/Template_verifier.py
""")
    parser.add_option(
        '-u', '--master_url', type='string', dest='master_url',
        default='http://secondlife.com/app/message_template/master_message_template.msg',
        help="""The url of the master message template.""")
    parser.add_option(
        '-c', '--cache_master', action='store_true', dest='cache_master',
        default=False,  help="""Set to true to attempt use local cached copy of the master template.""")

    options, args = parser.parse_args(sysargs)

    if options.mode == 'production':
        options.cache_master = False

    # both current and master supplied in positional params
    if len(args) == 2:
        master_filename, current_filename = args
        print "base:", master_filename
        print "current:", current_filename
        master_url = 'file://%s' % master_filename
        current_url = 'file://%s' % current_filename
    # only current supplied in positional param
    elif len(args) == 1:
        master_url = None
        current_filename = args[0]
        print "base: <master template from repository>"
        print "current:", current_filename
        current_url = 'file://%s' % current_filename
    # nothing specified, use defaults for everything
    elif len(args) == 0:
        master_url  = None
        current_url = None
    else:
        die("Too many arguments")

    if master_url is None:
        master_url = options.master_url
        
    # fetch the template for this build
    if current_url is None:
        current_filename = local_template_filename()
        print "base: <master template from repository>"
        print "current:", current_filename
        current_url = 'file://%s' % current_filename

    if options.cache_master:
        # optionally return a url to a locally-cached master so we don't hit the network all the time
        master_url = cache_master(master_url)

    current = fetch(current_url)
    try:
        master  = fetch(master_url)
    except IOError, e:
        if options.mode == 'production':
            raise e
        else:
            print "WARNING: problems fetching the master from %s.  Syntax-checking the local template ONLY, no compatibility check is being run." % master_url
            llmessage.parseTemplateString(current)
            return 0
        

    acceptable, compat = compare(
        master, current, options.mode)
        

    def explain(header, compat):
        print header
        # indent compatibility explanation
        print '\n\t'.join(compat.explain().split('\n'))

    if acceptable:
        explain("--- PASS ---", compat)
    else:
        explain("*** FAIL ***", compat)
        return 1

if __name__ == '__main__':
    sys.exit(run(sys.argv[1:]))


