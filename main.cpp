#include <iostream>
#include <algorithm>
#include <thread>
#include <zip.h>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <termios.h>
#include <sys/ioctl.h>
#include <cmath>
#include <map>
#include <unistd.h>
#include <sstream>
#include <pwd.h>

#define RETURN_TO_BEGIN_OF_PREV_LINE "\033[F"
#define RETURN_TO_PREV_LINE "\033[A"

struct termios* stdin_defaults = nullptr;
struct termios* stdout_defaults = nullptr;

inline void setting1()
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

inline void setting2()
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

inline void default_()
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

inline static void help(FILE* output_stream, const char* appname)
{
	fprintf(
			output_stream,
			"Usage: %s --action=(encrypt / e) --input=<input_path> --output=<encrypted_filename> (--passwd=<password> / --passwd-file=<password-file>)\n"
			"                                                                                                        or\n"
			"       %s --action=(decrypt / d) --input=<input_filename> --output=<output_filename> (--passwd=<password> / --passwd-file=<password-file>)\n"
			"                                                                                                        or\n"
			"       %s --action=info",
			appname, appname, appname
	);
	exit(0);
}

void draw_progress_bar(const std::string& label, double progress)
{
	std::cout << std::fixed;
	std::cout.precision(2);
	std::cout << label << " ";
	if (std::round(progress * 100) / 100.0 < 10.0)
	{
		std::cout << " ";
	}
	if (std::round(progress * 100) / 100.0 < 100.0)
	{
		std::cout << " ";
	}
	std::cout << progress << "% "; // 3 + (4 -> 6) = (7 -> 9)
	struct winsize sz;
	ioctl(stdout->_fileno, TIOCGWINSZ, &sz);
	size_t size = sz.ws_col;
	size -= label.size() + 11; // 2 + 9 = 11
	std::cout.flush();
	std::cout << "[";
	size_t part = std::lround(progress * (double)size / 100.0);
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
	std::cout << "]\r";
	std::cout.flush();
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

inline std::map<std::string, std::string>& parse_args(int argc, char** const& argv)
{
	auto result = new std::map<std::string, std::string>();
	for (int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		std::string::size_type pos = arg.find('=');
		if (pos != std::string::npos)
		{
			result->insert({arg.substr(0, pos), arg.substr(++pos, arg.size() - pos)});
		}
		else
		{
			result->insert({arg, ""});
		}
	}
	return *result;
}

typedef void (* cleanup_function)(void* args);

inline void promt(std::string file, cleanup_function cleanup_func, void* args = nullptr)
{
	struct stat st;
	if (!::stat(file.c_str(), &st))
	{
		std::cout << "File \"" << file << "\" will be overwritten. Do you agree?(y/N): ";
		char y_n = std::cin.get();
		if (y_n == 'Y' || y_n == 'y')
		{
			std::cout << "y\nremoving...\n";
		}
		else
		{
			std::cout << "N\nexiting...\n";
			cleanup_func(args);
		}
	}
}

std::string& exec(const std::string& cmd)
{
	char buffer[128];
	auto* result = new std::string;
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try
	{
		while (fgets(buffer, sizeof buffer, pipe) != nullptr)
		{
			*result += buffer;
			bzero(buffer, sizeof buffer);
		}
	} catch (...)
	{
		pclose(pipe);
		throw;
	}
	pclose(pipe);
	return *result;
}

FILE* completions = nullptr;

void set_completion(const char* appname, const char* arg, const char** parameters, size_t size, const char* description, const char* display_after = ""/*, const char* dont_mix_with = ""*/)
{
	std::string cmd("complete -c ");
	cmd += appname;
	if (size)
	{
		cmd += " -n \"not __fish_seen_subcommand_from ";
		for (int i = 0; i < size; ++i)
		{
			std::stringstream ss(exec(std::string("/bin/fish -c \"echo ") + parameters[i] + "\""));
			std::string s;
			while (ss >> s)
			{
				cmd += "--";
				cmd += arg;
				cmd += "=";
				cmd += s;
				cmd += " ";
			}
		}
		if (!std::string(display_after).empty())
		{
			cmd += "&& __fish_seen_subcommand_from ";
			cmd += display_after;
		}
//		if (!std::string(dont_mix_with).empty())
//		{
//			cmd += " && not __fish_seen_subcommand_from ";
//			cmd += dont_mix_with;
//		}
		cmd += "\"";
	}
	cmd += " -f -l ";
	cmd += arg;
	if (size)
	{
		cmd += " -a \"";
		for (int i = 0; i < size; ++i)
		{
			cmd += parameters[i];
			if (i != size - 1)
			{
				cmd += " ";
			}
		}
		cmd += "\"";
	}
	cmd += " -d \"";
	cmd += description;
	cmd += "\"\n";
	
	if (completions == nullptr)
	{
		std::cerr << ::strerror(errno) << "\n";
		exit(1);
	}
	fwrite(cmd.c_str(), 1, cmd.size(), completions);
}

void completion_init(const char* appname)
{
	std::string cmd("complete -c ");
	cmd += appname;
	cmd += " -e\n";
	
//	struct stat st{ };
//	if (!::stat((std::string("/etc/fish/completions/") + appname + ".fish").c_str(), &st))
//	{
//		::remove((std::string("/etc/fish/completions/") + appname + ".fish").c_str());
//	}
	
	struct passwd *pw = ::getpwuid(getuid());
	std::string s(pw->pw_dir);
	s += "/.config/fish/completions/";
	::mkdir(s.c_str(), 0777);
	s += "xor-crypto.fish";
	completions = ::fopen(s.c_str(), "wb");
	if (completions == nullptr)
	{
		std::cerr << ::strerror(errno) << "\n";
		exit(1);
	}
	fwrite(cmd.c_str(), 1, cmd.size(), completions);
}

int main(int argc, char** argv)
{
	setting1();
	setting2();
	
	if (argc < 2)
	{
		help(stdout, argv[0]);
	}
	
	auto parsed_args = parse_args(argc, argv);
	
	auto pos = parsed_args.find("--action");
	if (pos == parsed_args.end() || pos->second.empty())
	{
		help(stdout, argv[0]);
	}
	std::string action = pos->second;
	
	if ((action == "encrypt" || action == "e") && argc == 5)
	{
		pos = parsed_args.find("--input");
		auto pos2 = parsed_args.find("--output");
		auto pos3 = parsed_args.find("--passwd");
		auto pos4 = parsed_args.find("--passwd-file");
		
		if (pos == parsed_args.end() || pos->second.empty() || pos2 == parsed_args.end() || pos2->second.empty() ||
			(pos3 == parsed_args.end() && pos4 == parsed_args.end()) ||
			(pos3 != parsed_args.end() && pos4 != parsed_args.end()) ||
			(pos3->second.empty() && pos3->second.empty()) || (!pos3->second.empty() && !pos3->second.empty()))
		{
			help(stdout, argv[0]);
		}
		
		std::string input(pos->second), output(pos2->second), passwd;
		
		if (pos3 == parsed_args.end())
		{
			passwd = pos4->second;
		}
		else
		{
			passwd = pos3->second;
		}
		
		const char* resolved_input = ::realpath(input.c_str(), nullptr);
		if (resolved_input == nullptr)
		{
			error("Can not access file " + input + ". Maybe it does not exists.");
		}
		
		const char* resolved_passwd;
		if (pos4 != parsed_args.end())
		{
			resolved_passwd = ::realpath(passwd.c_str(), nullptr);
			if (resolved_passwd == nullptr)
			{
				error("Can not access file " + passwd + ". Maybe it does not exists.");
			}
		}
		
		if (is_dir(resolved_input))
		{
			std::string tmp_zip_file(output);
			tmp_zip_file += ".0";
			struct stat st;
			while (!::stat(tmp_zip_file.c_str(), &st))
			{
				tmp_zip_file += ".0";
			}
			archive_directory(resolved_input, tmp_zip_file);
			if (!::stat(output.c_str(), &st))
			{
				promt(
						output, [](void* tmp_zip_file)
						{
							remove(((std::string*)tmp_zip_file)->c_str());
							exit(0);
						},
						&tmp_zip_file
				);
			}
			remove_file(output.c_str());
			if (pos4 == parsed_args.end())
			{
				xor_crypt(tmp_zip_file, output, resolved_passwd);
			}
			else
			{
				xor_crypt_with_password_file(tmp_zip_file, output, resolved_passwd);
			}
			remove(tmp_zip_file.c_str());
		}
		else if (is_file_or_block(resolved_input))
		{
			promt(
					output, [](void*)
					{ exit(0); }
			);
			remove_file(output.c_str());
			if (pos4 == parsed_args.end())
			{
				xor_crypt(resolved_input, output, passwd);
			}
			else
			{
				xor_crypt_with_password_file(resolved_input, output, resolved_passwd);
			}
		}
		else
		{
			std::cout << "specified path: " << resolved_input << " is not file, block device or directory\n";
		}
	}
	else if ((action == "decrypt" || action == "d") && argc == 5)
	{
		pos = parsed_args.find("--input");
		auto pos2 = parsed_args.find("--output");
		auto pos3 = parsed_args.find("--passwd");
		auto pos4 = parsed_args.find("--passwd-file");
		
		if (pos == parsed_args.end() || pos->second.empty() || pos2 == parsed_args.end() || pos2->second.empty() ||
			(pos3 == parsed_args.end() && pos4 == parsed_args.end()) ||
			(pos3 != parsed_args.end() && pos4 != parsed_args.end()) ||
			(pos3->second.empty() && pos3->second.empty()) || (!pos3->second.empty() && !pos3->second.empty()))
		{
			help(stdout, argv[0]);
		}
		
		std::string input(pos->second), output(pos2->second), passwd;
		
		if (pos3 == parsed_args.end())
		{
			passwd = pos4->second;
		}
		else
		{
			passwd = pos3->second;
		}
		
		const char* resolved_input = ::realpath(input.c_str(), nullptr);
		if (resolved_input == nullptr)
		{
			error("Can not access file " + input + ". Maybe it does not exists.");
		}
		
		const char* resolved_passwd;
		if (pos4 != parsed_args.end())
		{
			resolved_passwd = ::realpath(passwd.c_str(), nullptr);
			if (resolved_passwd == nullptr)
			{
				error("Can not access file " + passwd + ". Maybe it does not exists.");
			}
		}
		
		if (!is_file_or_block(resolved_input))
		{
			std::cout << "Error. File " << resolved_input << " is not block device or regular file.\n";
			return 0;
		}
		
		promt(
				output, [](void*)
				{ exit(0); }
		);
		remove_file(output.c_str());
		if (pos4 == parsed_args.end())
		{
			xor_crypt(resolved_input, output, passwd);
		}
		else
		{
			xor_crypt_with_password_file(resolved_input, output, resolved_passwd);
		}
	}
	else if ((action == "info" || action == "i") && argc == 2)
	{
		std::cout << "< to enable fake progress bar press 'c' >\n"
					 "|  <1> | Program xor_crypto is xor encryptor.\n"
					 "|  <2> | Operation XOR works in such way:\n"
					 "|  <3> | +-------+-------+-------+\n"
					 "|  <4> | |input 1|input 2| output|\n"
					 "|  <5> | +-------+-------+-------+\n"
					 "|  <6> | |   0   |   0   |   0   |\n"
					 "|  <7> | +-------+-------+-------+\n"
					 "|  <8> | |   0   |   1   |   1   |\n"
					 "|  <9> | +-------+-------+-------+\n"
					 "| <10> | |   1   |   0   |   1   |\n"
					 "| <11> | +-------+-------+-------+\n"
					 "| <12> | |   1   |   1   |   0   |\n"
					 "| <13> | +-------+-------+-------+\n"
					 "| <14> | \n"
					 "| <15> | It means if we got two similar bits as input we produce 0, otherwise - 1.\n"
					 "| <16> | For example:\n"
					 "| <17> | 1001 XOR 1001 = 0000\n"
					 "| <18> | 1011 XOR 0111 = 1100\n"
					 "| <19> | \n"
					 "| <20> | It's interesting that if we know any two numbers we can obtain third.\n"
					 "| <21> | For example:\n"
					 "| <22> | 1001 XOR 1010 = 0011\n"
					 "| <23> |    a        b      c\n"
					 "| <24> | \n"
					 "| <25> | 1001 XOR 0011 = 1010\n"
					 "| <26> |    a        c      b\n"
					 "| <27> | \n"
					 "| <28> | 0011 XOR 1010 = 1001\n"
					 "| <29> |    c        b      a\n"
					 "| <30> | \n"
					 "| <31> | It means if we want to encrypt some data we can XOR our data with some secret number called password.\n"
					 "| <32> | To decrypt our data we must have two of three numbers: our data (encrypted or original) and password.\n"
					 "| <33> | We always store only one of them - data (encrypted or original), password we memorize.";
		std::cout.flush();
		
		while (std::cin.get() != 'c')
		{ }
		
		for (int i = 0; i < 33; ++i)
		{
			std::cout << RETURN_TO_BEGIN_OF_PREV_LINE;
		}
		
		std::cout.flush();
		
		double progress = 0;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
		while (true)
		{
			draw_progress_bar("Test progress bar:", progress);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			progress += 0.1;
			if (progress > 100.0)
			{
				progress = 0.0;
			}
		}
#pragma clang diagnostic pop
	}
	else if ((action == "install") && argc == 3)
	{
		completion_init(argv[2]);
		set_completion(argv[2], "action", new const char* [5]{"encrypt", "decrypt", "info", "help", "install"}, 5, "action");
		set_completion(argv[2], "input", new const char* [1]{"(ls -p | grep -v /)"}, 1, "input file or directory", "--action=encrypt --action=e");
		set_completion(argv[2], "input", new const char* [1]{"(ls -p | grep -v /)"}, 1, "input file", "--action=decrypt --action=d");
		set_completion(argv[2], "output", new const char* [1]{"(ls -p | grep -v /)"}, 1, "output file");
		set_completion(argv[2], "passwd", new const char* [0]{ }, 0, "password");
		set_completion(argv[2], "passwd-file", new const char* [1]{"(ls -p | grep -v /)"}, 1, "file with password");
	}
	else
	{
		help(stdout, argv[0]);
	}
	
	default_();
	
	return 0;
}
