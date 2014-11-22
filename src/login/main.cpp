#include <shared/Version.h>
#include <shared/Banner.h>
#include <botan/init.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <iostream>


int main(int argc, char** argv) {
	ember::print_banner("Login Daemon");
	Botan::LibraryInitializer init("thread_safe");
	return 0;
}
