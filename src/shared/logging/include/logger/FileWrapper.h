#pragma once

#include <cstdio>
#include <string>

#pragma warning(push)
#pragma warning(disable: 4996)

namespace ember { namespace log {

class File {
	std::FILE* file_ = nullptr;

public:
	File(std::string const& path, std::string const &mode = std::string("r")) {
		file_ = std::fopen(path.c_str(), mode.c_str());
	}

	~File() { if(file_) { fclose(file_); } };
	
	operator FILE*() const { return file_; }
	
	int close() {
		if(!file_) return 0;
		int ret = fclose(file_); 
		file_ = nullptr;
		return ret;
	};

	std::FILE* handle() {
		return file_;
	}

	File(const File&) = delete;
	File& operator=(const File&) = delete;
};

}} //log, ember

#pragma warning(pop)