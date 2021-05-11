//
// Created by imper on 5/10/21.
//

#ifndef XOR_CRYPTO_FILES_HPP
#define XOR_CRYPTO_FILES_HPP

#include "terminal_output.hpp"

#include <zip.h>
#include <dirent.h>
#include <cstring>

inline static void error(const std::string& error)
{
	std::cerr << error << "\n";
	throw std::runtime_error(error);
}

inline static bool is_dir(const std::string& path)
{
	struct stat st;
	::stat(path.c_str(), &st);
	return S_ISDIR(st.st_mode);
}

inline static bool is_file_or_block(const std::string& path)
{
	struct stat st;
	::stat(path.c_str(), &st);
	return S_ISREG(st.st_mode) || S_ISBLK(st.st_mode);
}

static void walk_though_directory(const std::string& start_path, const std::string& input_path, zip_t* zipper)
{
	DIR* dir = ::opendir(input_path.c_str());
	if (dir == nullptr)
	{
		error("Failed to open input directory: " + input_path + ". " + ::strerror(errno));
	}
	
	struct dirent* dirent_p;
	while ((dirent_p = readdir(dir)) != nullptr)
	{
		if (strcmp(dirent_p->d_name, ".") && strcmp(dirent_p->d_name, ".."))
		{
			std::string fullname = input_path + "/" + dirent_p->d_name;
			if (is_dir(fullname))
			{
				if (zip_dir_add(zipper, fullname.substr(start_path.length() + 1).c_str(), ZIP_FL_ENC_UTF_8) < 0)
				{
					error("Failed to add directory " + fullname + " to zip archive. " + zip_strerror(zipper));
				}
				walk_though_directory(start_path, fullname, zipper);
			}
			else
			{
				zip_source_t* source = zip_source_file(zipper, fullname.c_str(), 0, 0);
				if (source == nullptr)
				{
					error("Failed to add file " + fullname + " to zip archive. " + zip_strerror(zipper));
				}
				std::cout << "Compressing file: \"" << fullname << "\" to temporary zip.\n";
				if (zip_file_add(zipper, fullname.substr(start_path.length() + 1).c_str(), source, ZIP_FL_ENC_UTF_8) < 0)
				{
					zip_source_free(source);
					error("Failed to add file " + fullname + " to zip archive. " + zip_strerror(zipper));
				}
			}
		}
	}
	::closedir(dir);
}

static void archive_directory(const std::string& path, const std::string& out_file_name)
{
	int errorp;
	zip_t* zipper = zip_open(out_file_name.c_str(), ZIP_CREATE | ZIP_EXCL, &errorp);
	if (zipper == nullptr)
	{
		zip_error_t zerror;
		zip_error_init_with_code(&zerror, errorp);
		throw std::runtime_error("Failed to open output file " + out_file_name + ". " + zip_error_strerror(&zerror));
	}
	
	try
	{
		walk_though_directory(path, path, zipper);
	}
	catch (...)
	{
		zip_close(zipper);
		throw;
	}
	
	zip_close(zipper);
}

#define BUFFER_SIZE 1024

static void xor_crypt(const std::string& input_file, const std::string& output_file, const std::string& password)
{
	struct stat st;
	if (stat(input_file.c_str(), &st))
	{
		error("Could not open file " + input_file + ". " + ::strerror(errno));
	}
	FILE* input_f = ::fopen(input_file.c_str(), "rb");
	FILE* output_f = ::fopen(output_file.c_str(), "wb");
	unsigned char buffer[BUFFER_SIZE];
	int password_iter = 0;
	size_t read, progress = 0, file_size = st.st_size;
	while (!::feof(input_f))
	{
		::bzero(buffer, BUFFER_SIZE);
		read = ::fread(buffer, sizeof(char), BUFFER_SIZE, input_f);
		if (!read || read > BUFFER_SIZE)
		{
			fclose(input_f);
			fclose(output_f);
			error("Failed to read bytes from file " + input_file + ".");
		}
		
		progress += read;
		
		for (int i = 0; i < read; ++i)
		{
			if (password_iter == password.length())
			{
				password_iter = 0;
			}
			
			buffer[i] ^= password[password_iter];
			
			++password_iter;
		}
		
		::fwrite(buffer, sizeof(char), read, output_f);
		double perc = (double)((long double)progress * 100.0 / (long double)file_size);
		draw_progress_bar("Encrypting:", perc);
	}
	std::cout << "\n";
	fclose(input_f);
	fclose(output_f);
}

static void xor_crypt_with_password_file(const std::string& input_file, const std::string& output_file, const std::string& password_file)
{
	struct stat st;
	if (stat(input_file.c_str(), &st))
	{
		error("Could not open file " + input_file + ". " + ::strerror(errno));
	}
	FILE* input_f = ::fopen(input_file.c_str(), "rb");
	FILE* output_f = ::fopen(output_file.c_str(), "wb");
	FILE* passwd_f = ::fopen(password_file.c_str(), "rb");
	unsigned char buffer[BUFFER_SIZE];
	int password_iter = 0;
	size_t read, read_passwd = 0, progress = 0, file_size = st.st_size;
	::stat(password_file.c_str(), &st);
	char password[BUFFER_SIZE];
	while (!::feof(input_f))
	{
		::bzero(buffer, BUFFER_SIZE);
		read = ::fread(buffer, sizeof(char), BUFFER_SIZE, input_f);
		if (!read || read > BUFFER_SIZE)
		{
			::fclose(input_f);
			::fclose(output_f);
			::fclose(passwd_f);
			error("Failed to read bytes from file \"" + input_file + "\".");
		}
		
		progress += read;
		
		for (int i = 0; i < read; ++i)
		{
			if (!(password_iter % BUFFER_SIZE) && !::feof(passwd_f))
			{
				::bzero(password, BUFFER_SIZE);
				read_passwd = ::fread(password, sizeof(char), BUFFER_SIZE, passwd_f);
				if (!read_passwd || read_passwd > BUFFER_SIZE)
				{
					::fclose(input_f);
					::fclose(output_f);
					::fclose(passwd_f);
					error("Failed to read bytes from file \"" + password_file + "\".");
				}
			}
			else if ((password_iter % BUFFER_SIZE == read_passwd || !(password_iter % BUFFER_SIZE)) && ::feof(passwd_f))
			{
				::fseek(passwd_f, 0, SEEK_SET);
				::bzero(password, BUFFER_SIZE);
				read_passwd = ::fread(password, sizeof(char), BUFFER_SIZE, passwd_f);
				password_iter = 0;
			}
			
			buffer[i] ^= password[password_iter % BUFFER_SIZE];
			
			++password_iter;
		}
		
		::fwrite(buffer, sizeof(char), read, output_f);
		double perc = (double)((long double)progress * 100.0 / (long double)file_size);
		draw_progress_bar("Encrypting:", perc);
	}
	std::cout << "\n";
	fclose(input_f);
	fclose(output_f);
}

inline static void remove_file(const char* name)
{
	errno = 0;
	if (::remove(name) && errno != ENOENT)
	{
		error("Failed to delete file " + std::string(name) + ": " + ::strerror(errno));
	}
}

#endif //XOR_CRYPTO_FILES_HPP
