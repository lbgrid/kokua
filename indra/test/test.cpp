/** 
 * @file test.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Entry point for the test app.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

/** 
 *
 * You can add tests by creating a new cpp file in this directory, and
 * rebuilding. There are at most 50 tests per testgroup without a
 * little bit of template parameter and makefile tweaking.
 *
 */

#include "linden_common.h"
#include "llerrorcontrol.h"
#include "lltut.h"

#include "apr_pools.h"
#include "apr_getopt.h"

// the CTYPE_WORKAROUND is needed for linux dev stations that don't
// have the broken libc6 packages needed by our out-of-date static 
// libs (such as libcrypto and libcurl). -- Leviathan 20060113
#ifdef CTYPE_WORKAROUND
#	include "ctype_workaround.h"
#endif

#ifndef LL_WINDOWS
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#endif

namespace tut
{
	std::string sSourceDir;
	
    test_runner_singleton runner;
}

class LLTestCallback : public tut::callback
{
public:
	LLTestCallback(bool verbose_mode, std::ostream *stream, std::string suitename) :
	mVerboseMode(verbose_mode),
	mTotalTests(0),
	mPassedTests(0),
	mFailedTests(0),
	mSkippedTests(0),
	mStream(stream),
	suite_name(suitename)
	{
	}
	
	LLTestCallback()
	{
	}	
	
	virtual void run_started()
	{
		//std::cout << "run_started" << std::endl;
	}
	
	virtual void group_started(const std::string& name) {
		std::cout << "group_started name=" << name << std::endl;
	}
	
	virtual void group_completed(const std::string& name) {
		std::cout << "group_completed name=" << name << std::endl;
	}
	
	virtual void test_completed(const tut::test_result& tr)
	{
		++mTotalTests;
		std::ostringstream out;
		out << "[" << tr.group << ", " << tr.test << "] ";
		switch(tr.result)
		{
			case tut::test_result::ok:
				++mPassedTests;
				out << "ok";
				break;
			case tut::test_result::fail:
				++mFailedTests;
				out << "fail";
				break;
			case tut::test_result::ex:
				++mFailedTests;
				out << "exception";
				break;
			case tut::test_result::warn:
				++mFailedTests;
				out << "test destructor throw";
				break;
			case tut::test_result::term:
				++mFailedTests;
				out << "abnormal termination";
				break;
			case tut::test_result::skip:
				++mSkippedTests;			
				out << "skipped known failure";
				break;
			default:
				++mFailedTests;
				out << "unknown";
		}
		if(mVerboseMode || (tr.result != tut::test_result::ok))
		{
			if(!tr.message.empty())
			{
				out << ": '" << tr.message << "'";
			}
			if (mStream)
			{
				*mStream << out.str() << std::endl;
			}
			
			std::cout << out.str() << std::endl;
		}
	}
	
	virtual void run_completed()
	{
		if (mStream)
		{
			run_completed_(*mStream);
		}
		run_completed_(std::cout);
	}
	
	virtual int getFailedTests() const { return mFailedTests; }
	
	//private:
	virtual void run_completed_(std::ostream &stream)
	{
		stream << "\tTotal Tests:\t" << mTotalTests << std::endl;
		stream << "\tPassed Tests:\t" << mPassedTests;
		if (mPassedTests == mTotalTests)
		{
			stream << "\tYAY!! \\o/";
		}
		stream << std::endl;
		
		if (mSkippedTests > 0)
		{
			stream << "\tSkipped known failures:\t" << mSkippedTests
			<< std::endl;
		}
		
		if(mFailedTests > 0)
		{
			stream << "*********************************" << std::endl;
			stream << "Failed Tests:\t" << mFailedTests << std::endl;
			stream << "Please report or fix the problem." << std::endl;
			stream << "*********************************" << std::endl;
		}
	}
	
protected:
	std::string suite_name;
	bool mVerboseMode;
	int mTotalTests;
	int mPassedTests;
	int mFailedTests;
	int mSkippedTests;
	std::ostream *mStream;
};

// copy of LLTestCallback which should become a subclass (commented out below). Delete this LLTCTestCallback one fixed. 

// TeamCity specific class which emits service messages
// http://confluence.jetbrains.net/display/TCD3/Build+Script+Interaction+with+TeamCity;#BuildScriptInteractionwithTeamCity-testReporting

class LLTCTestCallback : public tut::callback
{
public:
	LLTCTestCallback(bool verbose_mode, std::ostream *stream, std::string suitename) :
	mVerboseMode(verbose_mode),
	mTotalTests(0),
	mPassedTests(0),
	mFailedTests(0),
	mSkippedTests(0),
	mStream(stream),
	suite_name(suitename)
	{
	}
	
	LLTCTestCallback()
	{
	}	
	
	virtual void run_started()
	{
		//std::cout << "unit test run_started" << std::flush;
	}
	
	virtual void group_started(const std::string& name) {
		std::cout << "group_started name=" << name << std::endl;
		std::cout << "##teamcity[testSuiteStarted name='" << name << "']\n"  << std::flush;
	}
	
	virtual void group_completed(const std::string& name) {
		std::cout << "group_completed name=" << name << std::endl;
		std::cout << "##teamcity[testSuiteFinished name='" << name << "']\n"  << std::flush;
	}
	
	virtual void test_completed(const tut::test_result& tr)
	{
		++mTotalTests;
		std::ostringstream out;
		out << "[" << tr.group << ", " << tr.test << "] \n";
		switch(tr.result)
		{
			case tut::test_result::ok:
				++mPassedTests;
				out << "ok";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			case tut::test_result::fail:
				++mFailedTests;
				out << "fail";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			case tut::test_result::ex:
				++mFailedTests;
				out << "exception";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			case tut::test_result::warn:
				++mFailedTests;
				out << "test destructor throw";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			case tut::test_result::term:
				++mFailedTests;
				out << "abnormal termination";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			case tut::test_result::skip:
				++mSkippedTests;			
				out << "skipped known failure";
				std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testIgnored name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
				break;
			default:
				++mFailedTests;
				out << "unknown";
		}
		if(mVerboseMode || (tr.result != tut::test_result::ok))
		{
			if(!tr.message.empty())
			{
				out << ": '" << tr.message << "'";
			}
			if (mStream)
			{
				*mStream << out.str() << std::endl;
			}
			
			std::cout << out.str() << std::endl;
		}
	}
	
	virtual void run_completed()
	{
		if (mStream)
		{
			run_completed_(*mStream);
		}
		run_completed_(std::cout);
	}
	
	virtual int getFailedTests() const { return mFailedTests; }
	
	//private:
	virtual void run_completed_(std::ostream &stream)
	{
		stream << "\tTotal Tests:\t" << mTotalTests << std::endl;
		stream << "\tPassed Tests:\t" << mPassedTests;
		if (mPassedTests == mTotalTests)
		{
			stream << "\tYAY!! \\o/";
		}
		stream << std::endl;
		
		if (mSkippedTests > 0)
		{
			stream << "\tSkipped known failures:\t" << mSkippedTests
			<< std::endl;
		}
		
		if(mFailedTests > 0)
		{
			stream << "*********************************" << std::endl;
			stream << "Failed Tests:\t" << mFailedTests << std::endl;
			stream << "Please report or fix the problem." << std::endl;
			stream << "*********************************" << std::endl;
		}
	}
	
protected:
	std::string suite_name;
	bool mVerboseMode;
	int mTotalTests;
	int mPassedTests;
	int mFailedTests;
	int mSkippedTests;
	std::ostream *mStream;
};


/*
 // commented out subclass which should be fixed to eliminate the duplicated LLTestCallback and LLTCTestCallaback classes
 // when this is fixed, the duplicated code in the if(getenv("TEAMCITY_PROJECT_NAME") statements below
 // 
 // currectly producing errors like thr following:
 //		{path}viewer-tut-teamcity2/indra/build-darwin-i386/sharedlibs/RelWithDebInfo/RelWithDebInfo/PROJECT_llmessage_TEST_llmime 
 //				--touch={path}viewer-tut-teamcity2/indra/build-darwin-i386/llmessage/PROJECT_llmessage_TEST_llmime_ok.txt 
 //				--{path}viewer-tut-teamcity2/indra/llmessage
 // 
 //		run_started
 //		group_started name=mime_index
 //		##teamcity[testSuiteStarted name='mime_index']
 //		Segmentation fault
 
 
 // TeamCity specific class which emits service messages
 // http://confluence.jetbrains.net/display/TCD3/Build+Script+Interaction+with+TeamCity;#BuildScriptInteractionwithTeamCity-testReporting
 
 class LLTCTestCallback : public LLTestCallback
 {
 public:
 LLTCTestCallback(bool verbose_mode, std::ostream *stream, std::string suitename) :
 mVerboseMode(verbose_mode),
 mTotalTests(0),
 mPassedTests(0),
 mFailedTests(0),
 mSkippedTests(0),
 mStream(stream),
 suite_name(suitename)
 {
 }
 
 LLTCTestCallback()
 {
 }	
 
 virtual void group_started(const std::string& name) {
 LLTestCallback::group_started(name);
 std::cout << "##teamcity[testSuiteStarted name='" << name << "']\n"  << std::flush;
 }
 
 virtual void group_completed(const std::string& name) {
 LLTestCallback::group_completed(name);
 std::cout << "##teamcity[testSuiteFinished name='" << name << "']\n"  << std::flush;
 }
 
 virtual void test_completed(const tut::test_result& tr)
 {
 LLTestCallback::test_completed(tr);
 
 switch(tr.result)
 {
 case tut::test_result::ok:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 case tut::test_result::fail:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 case tut::test_result::ex:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 case tut::test_result::warn:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 case tut::test_result::term:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFailed name='" << tr.group << "." << tr.test << "' message='" << tr.message << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 case tut::test_result::skip:
 std::cout << "##teamcity[testStarted name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testIgnored name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 std::cout << "##teamcity[testFinished name='" << tr.group << "." << tr.test << "']\n" << std::flush;
 break;
 default:
 break;
 }
 
 }
 
 protected:
 std::string suite_name;
 bool mVerboseMode;
 int mTotalTests;
 int mPassedTests;
 int mFailedTests;
 int mSkippedTests;
 std::ostream *mStream;
 };
 
 }
 */

static const apr_getopt_option_t TEST_CL_OPTIONS[] =
{
	{"help", 'h', 0, "Print the help message."},
	{"list", 'l', 0, "List available test groups."},
	{"verbose", 'v', 0, "Verbose output."},
	{"group", 'g', 1, "Run test group specified by option argument."},
	{"output", 'o', 1, "Write output to the named file."},
	{"sourcedir", 's', 1, "Project source file directory from CMake."},
	{"touch", 't', 1, "Touch the given file if all tests succeed"},
	{"wait", 'w', 0, "Wait for input before exit."},
	{"debug", 'd', 0, "Emit full debug logs."},
	{"suitename", 'x', 1, "Run tests using this suitename"},
	{0, 0, 0, 0}
};

void stream_usage(std::ostream& s, const char* app)
{
	s << "Usage: " << app << " [OPTIONS]" << std::endl
	<< std::endl;
	
	s << "This application runs the unit tests." << std::endl << std::endl;
	
	s << "Options: " << std::endl;
	const apr_getopt_option_t* option = &TEST_CL_OPTIONS[0];
	while(option->name)
	{
		s << "  ";
		s << "  -" << (char)option->optch << ", --" << option->name
		<< std::endl;
		s << "\t" << option->description << std::endl << std::endl;
		++option;
	}
	
	s << "Examples:" << std::endl;
	s << "  " << app << " --verbose" << std::endl;
	s << "\tRun all the tests and report all results." << std::endl;
	s << "  " << app << " --list" << std::endl;
	s << "\tList all available test groups." << std::endl;
	s << "  " << app << " --group=uuid" << std::endl;
	s << "\tRun the test group 'uuid'." << std::endl;
}

void stream_groups(std::ostream& s, const char* app)
{
	s << "Registered test groups:" << std::endl;
	tut::groupnames gl = tut::runner.get().list_groups();
	tut::groupnames::const_iterator it = gl.begin();
	tut::groupnames::const_iterator end = gl.end();
	for(; it != end; ++it)
	{
		s << "  " << *(it) << std::endl;
	}
}

void wouldHaveCrashed(const std::string& message)
{
	tut::fail("llerrs message: " + message);
}

int main(int argc, char **argv)
{
	// The following line must be executed to initialize Google Mock
	// (and Google Test) before running the tests.
#ifndef LL_WINDOWS
	::testing::InitGoogleMock(&argc, argv);
#endif
	LLError::initForApplication(".");
	LLError::setFatalFunction(wouldHaveCrashed);
	LLError::setDefaultLevel(LLError::LEVEL_ERROR);
	//< *TODO: should come from error config file. Note that we
	// have a command line option that sets this to debug.
	
#ifdef CTYPE_WORKAROUND
	ctype_workaround();
#endif
	
	apr_initialize();
	apr_pool_t* pool = NULL;
	if(APR_SUCCESS != apr_pool_create(&pool, NULL))
	{
		std::cerr << "Unable to initialize pool" << std::endl;
		return 1;
	}
	apr_getopt_t* os = NULL;
	if(APR_SUCCESS != apr_getopt_init(&os, pool, argc, argv))
	{
		std::cerr << "Unable to  pool" << std::endl;
		return 1;
	}
	
	// values used for controlling application
	bool verbose_mode = false;
	bool wait_at_exit = false;
	std::string test_group;
	std::string suite_name;
	
	// values use for options parsing
	apr_status_t apr_err;
	const char* opt_arg = NULL;
	int opt_id = 0;
	std::ofstream *output = NULL;
	const char *touch = NULL;
	
	while(true)
	{
		apr_err = apr_getopt_long(os, TEST_CL_OPTIONS, &opt_id, &opt_arg);
		if(APR_STATUS_IS_EOF(apr_err)) break;
		if(apr_err)
		{
			char buf[255];		/* Flawfinder: ignore */
			std::cerr << "Error parsing options: "
			<< apr_strerror(apr_err, buf, 255) << std::endl;
			return 1;
		}
		switch (opt_id)
		{
			case 'g':
				test_group.assign(opt_arg);
				break;
			case 'h':
				stream_usage(std::cout, argv[0]);
				return 0;
				break;
			case 'l':
				stream_groups(std::cout, argv[0]);
				return 0;
			case 'v':
				verbose_mode = true;
				break;
			case 'o':
				output = new std::ofstream;
				output->open(opt_arg);
				break;
			case 's':	// --sourcedir
				tut::sSourceDir = opt_arg;
				// For convenience, so you can use tut::sSourceDir + "myfile"
				tut::sSourceDir += '/';
				break;
			case 't':
				touch = opt_arg;
				break;
			case 'w':
				wait_at_exit = true;
				break;
			case 'd':
				// *TODO: should come from error config file. We set it to
				// ERROR by default, so this allows full debug levels.
				LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
				break;
			case 'x':
				suite_name.assign(opt_arg);
				break;
			default:
				stream_usage(std::cerr, argv[0]);
				return 1;
				break;
		}
	}
	
	/*
	 // commented out test tunner logic which should be fixed when eliminate the duplicated LLTestCallback and LLTCTestCallaback classes
	 // become proper class:subclass
	 // if the Segmentation fault issue is resolved, all code in the block comments can be uncommented, and all code below can be removed.
	 
	 LLTestCallback* mycallback;
	 if (getenv("TEAMCITY_PROJECT_NAME"))
	 {
	 mycallback = new LLTCTestCallback(verbose_mode, output, suite_name);
	 
	 }
	 else
	 {
	 mycallback = new LLTestCallback(verbose_mode, output, suite_name);
	 }
	 
	 tut::runner.get().set_callback(mycallback);
	 
	 if(test_group.empty())
	 {
	 tut::runner.get().run_tests();
	 }
	 else
	 {
	 tut::runner.get().run_tests(test_group);
	 }
	 
	 bool success = (mycallback->getFailedTests() == 0);
	 
	 if (wait_at_exit)
	 {
	 std::cerr << "Press return to exit..." << std::endl;
	 std::cin.get();
	 }
	 
	 if (output)
	 {
	 output->close();
	 delete output;
	 }
	 
	 if (touch && success)
	 {
	 std::ofstream s;
	 s.open(touch);
	 s << "ok" << std::endl;
	 s.close();
	 }
	 
	 apr_terminate();
	 
	 int retval = (success ? 0 : 1);
	 return retval;
	 */
	
	// run the tests
	
	if (getenv("TEAMCITY_PROJECT_NAME"))
	{
		LLTCTestCallback* mycallback;
		mycallback = new LLTCTestCallback(verbose_mode, output, suite_name);
		
		tut::runner.get().set_callback(mycallback);
		
		if(test_group.empty())
		{
			tut::runner.get().run_tests();
		}
		else
		{
			tut::runner.get().run_tests(test_group);
		}
		
		bool success = (mycallback->getFailedTests() == 0);
		
		if (wait_at_exit)
		{
			std::cerr << "Press return to exit..." << std::endl;
			std::cin.get();
		}
		
		if (output)
		{
			output->close();
			delete output;
		}
		
		if (touch && success)
		{
			std::ofstream s;
			s.open(touch);
			s << "ok" << std::endl;
			s.close();
		}
		
		apr_terminate();
		
		int retval = (success ? 0 : 1);
		return retval;
		
		
	}
	// NOT if (getenv("TEAMCITY_PROJECT_NAME"))
	else
	{
		LLTestCallback* mycallback;
		mycallback = new LLTestCallback(verbose_mode, output, suite_name);
		
		tut::runner.get().set_callback(mycallback);
		
		if(test_group.empty())
		{
			tut::runner.get().run_tests();
		}
		else
		{
			tut::runner.get().run_tests(test_group);
		}
		
		bool success = (mycallback->getFailedTests() == 0);
		
		if (wait_at_exit)
		{
			std::cerr << "Press return to exit..." << std::endl;
			std::cin.get();
		}
		
		if (output)
		{
			output->close();
			delete output;
		}
		
		if (touch && success)
		{
			std::ofstream s;
			s.open(touch);
			s << "ok" << std::endl;
			s.close();
		}
		
		apr_terminate();
		
		int retval = (success ? 0 : 1);
		return retval;
		
	}
}
