#pragma once

#include <cstdio>
#include <string>

namespace ember { namespace log {

class File {
	std::FILE* file;

public:
	File(std::string const& path, std::string const &mode = std::string("r")) {
		if(fopen_s(&file, path.c_str(), mode.c_str()) != 0) {
			file = NULL;
		}
	}

	~File() { if(file) { fclose(file); } };
	
	operator FILE*() const { return file; }
	
	int close() {
		if(!file) return 0;
		int ret = fclose(file); file = NULL; return ret;
	};
	File(const File& that) = delete;
};

}} //log, ember