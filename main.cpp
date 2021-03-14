#include <iostream>
#include <thread>
#include <zip.h>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <termios.h>
#include <sys/ioctl.h>
#include <cmath>

#define RETURN_TO_BEGIN_OF_PREV_LINE "\033[F"
#define RETURN_TO_PREV_LINE "\033[A"

struct termios* stdin_defaults = nullptr;
struct termios* stdout_defaults = nullptr;

void setting1()
{
	if (stdin_defaults == nullptr)
	{
		stdin_defaults = new termios;
	}
	::tcgetattr(0, stdin_defaults);
	struct termios settings = *stdin_defaults;
	settings.c_lflag &= (~ICANON);
	settings.c_lflag &= (~ECHO);
	settings.c_cc[VTIME] = 0;
	settings.c_cc[VMIN] = 1;
	::tcsetattr(0, TCSANOW, &settings);
}

void setting2()
{
	if (stdout_defaults == nullptr)
	{
		stdout_defaults = new termios;
	}
	::tcgetattr(1, stdout_defaults);
	struct termios settings = *stdout_defaults;
	settings.c_lflag &= (~ICANON);
	settings.c_lflag &= (~ECHO);
	settings.c_cc[VTIME] = 0;
	settings.c_cc[VMIN] = 1;
	::tcsetattr(1, TCSANOW, &settings);
}

void default_()
{
	if (stdin_defaults != nullptr)
	{
		::tcsetattr(0, TCSANOW, stdin_defaults);
		stdin_defaults = nullptr;
	}
	
	if (stdout_defaults != nullptr)
	{
		::tcsetattr(1, TCSANOW, stdout_defaults);
		stdout_defaults = nullptr;
	}
}

static void error(const std::string& error)
{
	std::cerr << error << "\n";
	throw std::runtime_error(error);
}

static bool is_dir(const std::string& path)
{
	struct stat st;
	::stat(path.c_str(), &st);
	return S_ISDIR(st.st_mode);
}

static void walk_though_directory(const std::string& start_path, const std::string& input_path, zip_t* zipper)
{
	DIR* dir = ::opendir(input_path.c_str());
	if (dir == nullptr)
	{
		throw std::runtime_error("Failed to open input directory: " + input_path + ". " + ::strerror(errno));
	}
	
	struct dirent* dirent_p;
	while ((dirent_p = readdir(dir)) != NULL)
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

static void help(FILE* output_stream, const char* appname)
{
	fprintf(
			output_stream,
			"Usage:\t%s (encrypt / e) <input_path> <encrypted_filename> <password>\n or\n"
			"      \t%s (decrypt / d) <input_filename> <output_filename> <password>",
			appname, appname
	);
	exit(0);
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
	std::cout << std::fixed;
	std::cout.precision(2);
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
		double perc = (double)progress * 100.0 / (double)file_size;
		std::cout << "Encrypting: " << perc << "%\n"; // 11 + (1 -> 3) + 2 = 14 -> 16
		struct winsize sz;
		ioctl(stdout->_fileno, TIOCGWINSZ, &sz);
		size_t size = sz.ws_col;
		-- --size;
		std::cout.flush();
		std::cout << "[";
		size_t part = std::round(perc * (double)size / 100.0);
		for (int i = 0; i < size; ++i)
		{
			if (i < part)
			{
				std::cout << '=';
			}
			else
			{
				std::cout << ' ';
			}
		}
		std::cout << "]" RETURN_TO_BEGIN_OF_PREV_LINE;
		std::cout.flush();
	}
	std::cout << "\n\n";
	fclose(input_f);
	fclose(output_f);
}

static void remove_file(const char* name)
{
	errno = 0;
	if (::remove(name) && errno != ENOENT)
	{
		error("Failed to delete file " + std::string(name) + ": " + ::strerror(errno));
	}
}

int main(int argc, char** argv)
{
	setting1();
	setting2();
	
	if (argc < 2)
	{
		help(stdout, argv[0]);
	}
	
	if ((!strcmp(argv[1], "encrypt") || !strcmp(argv[1], "e")) && argc == 5)
	{
		char* resolved = ::realpath(argv[2], nullptr);
		if (resolved == nullptr)
		{
			error("Can not access file " + std::string(argv[2]) + ". Maybe it does not exists.");
		}
		
		if (is_dir(resolved))
		{
			std::string tmp_zip_file(argv[3]);
			tmp_zip_file += ".0";
			struct stat st;
			while (!::stat(tmp_zip_file.c_str(), &st))
			{
				tmp_zip_file += ".0";
			}
			archive_directory(resolved, tmp_zip_file);
			if (!::stat(argv[3], &st))
			{
				std::cout << "File \"" << argv[3] << "\" will be overwritten. Do you agree?(y/N)";
				char y_n;
				std::cin >> y_n;
				if (y_n == 'Y' || y_n == 'y')
				{
					std::cout << "\nremoving...\n";
				}
				else
				{
					std::cout << "\nexiting...\n";
					remove(tmp_zip_file.c_str());
					exit(0);
				}
			}
			remove_file(argv[3]);
			xor_crypt(tmp_zip_file, argv[3], argv[4]);
			remove(tmp_zip_file.c_str());
		}
		else
		{
			xor_crypt(resolved, argv[3], argv[4]);
		}
	}
	else if ((!strcmp(argv[1], "decrypt") || !strcmp(argv[1], "d")) && argc == 5)
	{
		remove_file(argv[3]);
		xor_crypt(argv[2], argv[3], argv[4]);
	}
	else if ((!strcmp(argv[1], "--info") || !strcmp(argv[1], "-i")) && argc == 2)
	{
		std::cout << "[                                  ]\n";
		std::cout << "Program xor_crypto is xor encryptor.\n"
					 "Operation XOR works in such way:\n"
					 "+-------+-------+------+\n"
					 "|input 1|input 2|output|\n"
					 "+-------+-------+------+\n"
					 "|   0   |   0   |  0   |\n"
					 "+-------+-------+------+\n"
					 "|   0   |   1   |  1   |\n"
					 "+-------+-------+------+\n"
					 "|   1   |   0   |  1   |\n"
					 "+-------+-------+------+\n"
					 "|   1   |   1   |  1   |\n"
					 "+-------+-------+------+\n"
					 "\n"
					 "It means if we got two similar bits as input we produce 0, otherwise - 1.\n"
					 "For example:\n"
					 "1001 XOR 1001 = 0000\n"
					 "1011 XOR 0111 = 1100\n"
					 "\n"
					 "It's interesting that if we know any two numbers we can obtain third.\n"
					 "For example:\n"
					 "1001 XOR 1010 = 0011\n"
					 "   a        b      c\n"
					 "\n"
					 "1001 XOR 0011 = 1010\n"
					 "   a        c      b\n"
					 "\n"
					 "0011 XOR 1010 = 1001\n"
					 "   c        b      a\n"
					 "\n"
					 "It means if we want to encrypt some data we can XOR our data with some secret number called password.\n"
					 "To decrypt our data we must have two of three numbers: our data (encrypted or original) and password.\n"
					 "We always store only one of them - data (encrypted or original), password we memorize.";
		std::cout.flush();
		
		for (int i = 0; i < 34; ++i)
		{
			std::cout << RETURN_TO_BEGIN_OF_PREV_LINE;
		}
		
		std::cout.flush();
		
		int progress = 0;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
		while (true)
		{
			std::cout << '[';
			for (int i = 0; i < 34; ++i)
			{
				if (i < progress)
				{
					std::cout << '=';
				}
				else
				{
					std::cout << ' ';
				}
			}
			std::cout << "]\r";// 34
			std::cout.flush();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++progress;
			if (progress > 34)
			{
				progress = 0;
			}
		}
#pragma clang diagnostic pop
	}
	else
	{
		help(stdout, argv[0]);
	}
	
	default_();
	
	return 0;
}
