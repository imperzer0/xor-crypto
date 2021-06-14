#include <map>
#include <thread>
#include "archive_and_encrypt.hpp"
#include "completions.hpp"

inline static void help(FILE* output_stream, const char* appname)
{
	fprintf(
			output_stream,
			"Usage: %s --action=encrypt --input=<input_path> --output=<encrypted_filename> (--passwd=<password> --passwd-file=<password-file>)\n"
			"       or\n"
			"       %s --action=decrypt --input=<input_filename> --output=<output_filename> (--passwd=<password> --passwd-file=<password-file>)\n"
			"       or\n"
			"       %s --action=info\n"
			"       or\n"
			"       %s --action=install <program_name>\n"
			"       or\n"
			"       %s --action=uninstall <program_name>\n",
			appname, appname, appname, appname, appname
	);
	exit(0);
}

std::map<std::string, std::string>& parse_args(int argc, char** const& argv)
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

constexpr const char* info_message[]{"< to enable fake progress bar press 'c' >\n"
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
									 "| <33> | We always store only one of them - data (encrypted or original), password we memorize."};

inline void encrypt_entry(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_input = ::realpath(input.c_str(), nullptr);
	if (resolved_input == nullptr)
	{
		error("Can not access file " + input + ". Maybe it does not exists.");
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
		xor_crypt(tmp_zip_file, output, passwd);
		remove(tmp_zip_file.c_str());
	}
	else if (is_file_or_block(resolved_input))
	{
		promt(
				output, [](void*)
				{ exit(0); }
		);
		remove_file(output.c_str());
		xor_crypt(resolved_input, output, passwd);
	}
	else
	{
		std::cout << "specified path: " << resolved_input << " is not file, block device or directory.\n";
	}
}

inline void encrypt_entry_with_file(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_input = ::realpath(input.c_str(), nullptr);
	if (resolved_input == nullptr)
	{
		error("Can not access file " + input + ". Maybe it does not exists.");
	}
	
	const char* resolved_passwd = ::realpath(passwd.c_str(), nullptr);
	if (resolved_passwd == nullptr)
	{
		error("Can not access file " + passwd + ". Maybe it does not exists.");
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
						remove_file(((std::string*)tmp_zip_file)->c_str());
						exit(0);
					},
					&tmp_zip_file
			);
		}
		remove_file(output.c_str());
		xor_crypt_with_password_file(tmp_zip_file, output, resolved_passwd);
		remove(tmp_zip_file.c_str());
	}
	else if (is_file_or_block(resolved_input))
	{
		promt(
				output, [](void*)
				{ exit(0); }
		);
		remove_file(output.c_str());
		xor_crypt_with_password_file(resolved_input, output, resolved_passwd);
	}
	else
	{
		std::cout << "specified path: " << resolved_input << " is not file, block device or directory.\n";
	}
}

inline void decrypt_entry(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_input = ::realpath(input.c_str(), nullptr);
	if (resolved_input == nullptr)
	{
		error("Can not access file " + input + ". Maybe it does not exists.");
	}
	
	if (is_file_or_block(resolved_input))
	{
		promt(
				output, [](void*)
				{ exit(0); }
		);
		remove_file(output.c_str());
		xor_crypt(resolved_input, output, passwd);
	}
	else
	{
		std::cout << "specified path: " << resolved_input << " is not block device or regular file.\n";
	}
}

inline void decrypt_entry_with_file(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_input = ::realpath(input.c_str(), nullptr);
	if (resolved_input == nullptr)
	{
		error("Can not access file " + input + ". Maybe it does not exists.");
	}
	
	const char* resolved_passwd = ::realpath(passwd.c_str(), nullptr);
	if (resolved_passwd == nullptr)
	{
		error("Can not access file " + passwd + ". Maybe it does not exists.");
	}
	
	if (is_file_or_block(resolved_input))
	{
		promt(
				output, [](void*)
				{ exit(0); }
		);
		remove_file(output.c_str());
		xor_crypt_with_password_file(resolved_input, output, resolved_passwd);
	}
	else
	{
		std::cout << "specified path: " << resolved_input << " is not block device or regular file.\n";
	}
}


inline void encrypt_entry_string(const std::string& input, const std::string& output, const std::string& passwd)
{
	promt(
			output, [](void*)
			{ exit(0); }
	);
	remove_file(output.c_str());
	xor_crypt_input_string(input, output, passwd);
}

inline void encrypt_entry_string_with_file(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_passwd = ::realpath(passwd.c_str(), nullptr);
	if (resolved_passwd == nullptr)
	{
		error("Can not access file " + passwd + ". Maybe it does not exists.");
	}
	
	promt(
			output, [](void*)
			{ exit(0); }
	);
	remove_file(output.c_str());
	xor_crypt_input_string_with_password_file(input, output, resolved_passwd);
}

inline void decrypt_entry_string(const std::string& input, const std::string& output, const std::string& passwd)
{
	promt(
			output, [](void*)
			{ exit(0); }
	);
	remove_file(output.c_str());
	xor_crypt_input_string(input, output, passwd);
}

inline void decrypt_entry_string_with_file(const std::string& input, const std::string& output, const std::string& passwd)
{
	const char* resolved_passwd = ::realpath(passwd.c_str(), nullptr);
	if (resolved_passwd == nullptr)
	{
		error("Can not access file " + passwd + ". Maybe it does not exists.");
	}
	
	promt(
			output, [](void*)
			{ exit(0); }
	);
	remove_file(output.c_str());
	xor_crypt_input_string_with_password_file(input, output, resolved_passwd);
}

template <typename _char = char>
std::basic_string<_char>& read_password(std::basic_istream<_char>& input_stream = std::cin, std::basic_ostream<_char>& output_stream = std::cout)
{
	_char c;
	auto* console_input = new std::basic_string<_char>();
	while (true)
	{
		c = input_stream.get();
		if (c == '\b' || c == 0x7f)
		{
			if (!console_input->empty())
			{
				std::cout << "\b \b";
				console_input->pop_back();
			}
		}
		else if (c == '\n')
		{
			output_stream << '\n';
			break;
		}
		else
		{
			*console_input += c;
			output_stream << '*';
		}
	}
	return *console_input;
}

std::string& read_text()
{
	std::cout << "Type text here to encode. Once you've finished type ESC to start process.\n";
	struct winsize sz;
	ioctl(stdout->_fileno, TIOCGWINSZ, &sz);
	size_t size = sz.ws_col;
	std::cout << '+';
	for (int i = 1; i < size - 1; ++i)
	{
		std::cout << '=';
	}
	std::cout << "+\n| ";
	char c;
	auto* console_input = new std::string();
	std::vector<std::string> lines;
	lines.emplace_back();
	while (true)
	{
		c = std::cin.get();
		if (c == 0x1b)
		{
			std::cout << "\n";
			break;
		}
		else if (c == '\n')
		{
			*console_input += c;
			std::cout << "\n| ";
			lines.back() += c;
			lines.emplace_back();
		}
		else if (c == '\b' || c == 0x7f)
		{
			if (!lines.back().empty())
			{
				lines.back().pop_back();
				std::cout << "\b \b";
			}
			else
			{
				lines.pop_back();
				std::cout << "\r  " << RETURN_TO_BEGIN_OF_PREV_LINE << "\033[" << lines.back().size() + 1 << "C";
			}
			console_input->pop_back();
		}
		else
		{
			*console_input += c;
			lines.back() += c;
			std::cout << c;
		}
	}
	return *console_input;
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
	
	if (action == "encrypt" && argc >= 4 && argc <= 6)
	{
		pos = parsed_args.find("--input");
		auto pos2 = parsed_args.find("--output");
		auto pos3 = parsed_args.find("--passwd");
		auto pos4 = parsed_args.find("--passwd-file");
		
		if (pos == parsed_args.end() || pos->second.empty() ||
			pos2 == parsed_args.end() || pos2->second.empty())
		{
			help(stdout, argv[0]);
		}
		
		std::string input(pos->second), output(pos2->second), passwd;
		
		if ((pos3 == parsed_args.end() || pos3->second.empty()) && (pos4 == parsed_args.end() || pos4->second.empty()))
		{
			std::cout << "type password: ";
			std::string console_input = read_password();
			parsed_args["--passwd"] = console_input;
			pos3 = parsed_args.find("--passwd");
		}
		
		bool is_from_stdin = pos->second == "&stdin";
		
		if (is_from_stdin)
		{
			input = read_text();
			if (pos3 != parsed_args.end())
			{
				passwd = pos3->second;
				encrypt_entry_string(input, output, passwd);
				if (pos4 != parsed_args.end())
				{
					passwd = pos4->second;
					std::string tmp_file(output);
					tmp_file += ".0";
					struct stat st;
					while (!::stat(tmp_file.c_str(), &st))
					{
						tmp_file += ".0";
					}
					encrypt_entry_string_with_file(output, tmp_file, passwd);
					remove_file(output.c_str());
					::rename(tmp_file.c_str(), output.c_str());
				}
			}
			else if (pos4 != parsed_args.end())
			{
				passwd = pos4->second;
				encrypt_entry_string_with_file(input, output, passwd);
			}
		}
		else
		{
			if (pos3 != parsed_args.end())
			{
				passwd = pos3->second;
				encrypt_entry(input, output, passwd);
				if (pos4 != parsed_args.end())
				{
					passwd = pos4->second;
					std::string tmp_file(output);
					tmp_file += ".0";
					struct stat st;
					while (!::stat(tmp_file.c_str(), &st))
					{
						tmp_file += ".0";
					}
					encrypt_entry_with_file(output, tmp_file, passwd);
					remove_file(output.c_str());
					::rename(tmp_file.c_str(), output.c_str());
				}
			}
			else if (pos4 != parsed_args.end())
			{
				passwd = pos4->second;
				encrypt_entry_with_file(input, output, passwd);
			}
		}
	}
	else if (action == "decrypt" && argc >= 4 && argc <= 6)
	{
		pos = parsed_args.find("--input");
		auto pos2 = parsed_args.find("--output");
		auto pos3 = parsed_args.find("--passwd");
		auto pos4 = parsed_args.find("--passwd-file");
		
		if (pos == parsed_args.end() || pos->second.empty() ||
			pos2 == parsed_args.end() || pos2->second.empty())
		{
			help(stdout, argv[0]);
		}
		
		std::string input(pos->second), output(pos2->second), passwd;
		
		if ((pos3 == parsed_args.end() || pos3->second.empty()) && (pos4 == parsed_args.end() || pos4->second.empty()))
		{
			std::cout << "type password: ";
			std::string console_input = read_password();
			parsed_args["--passwd"] = console_input;
			pos3 = parsed_args.find("--passwd");
		}
		
		bool is_from_stdin = pos->second == "&stdin";
		
		if (is_from_stdin)
		{
			if (pos3 != parsed_args.end())
			{
				passwd = pos3->second;
				decrypt_entry_string(input, output, passwd);
				if (pos4 != parsed_args.end())
				{
					passwd = pos4->second;
					std::string tmp_file(output);
					tmp_file += ".0";
					struct stat st;
					while (!::stat(tmp_file.c_str(), &st))
					{
						tmp_file += ".0";
					}
					decrypt_entry_string_with_file(output, tmp_file, passwd);
					remove_file(output.c_str());
					::rename(tmp_file.c_str(), output.c_str());
				}
			}
			else if (pos4 != parsed_args.end())
			{
				passwd = pos4->second;
				decrypt_entry_string_with_file(input, output, passwd);
			}
		}
		else
		{
			if (pos3 != parsed_args.end())
			{
				passwd = pos3->second;
				decrypt_entry(input, output, passwd);
				if (pos4 != parsed_args.end())
				{
					passwd = pos4->second;
					std::string tmp_file(output);
					tmp_file += ".0";
					struct stat st;
					while (!::stat(tmp_file.c_str(), &st))
					{
						tmp_file += ".0";
					}
					decrypt_entry_with_file(output, tmp_file, passwd);
					remove_file(output.c_str());
					::rename(tmp_file.c_str(), output.c_str());
				}
			}
			else if (pos4 != parsed_args.end())
			{
				passwd = pos4->second;
				decrypt_entry_with_file(input, output, passwd);
			}
		}
	}
	else if (action == "info" && argc == 2)
	{
		std::cout << info_message;
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
	else if (action == "install-completions" && argc == 3)
	{
		completion_init(argv[2]);
		set_completion(argv[2], "help", new const char* [0]{ }, 0, "print help");
		set_completion(argv[2], "action", new const char* [5]{"encrypt", "decrypt", "info", "help", "install"}, 5, "action");
		set_completion(argv[2], "input", new const char* [2]{"(ls -p | grep -v /)", "%stdin%"}, 2, "input file or directory", "--action=encrypt");
		set_completion(argv[2], "input", new const char* [2]{"(ls -p | grep -v /)", "%stdin%"}, 2, "input file", "--action=decrypt");
		set_completion(argv[2], "output", new const char* [2]{"(ls -p | grep -v /)", "%stdout%"}, 2, "output file");
		set_completion(argv[2], "passwd", new const char* [0]{ }, 0, "password");
		set_completion(argv[2], "passwd-file", new const char* [1]{"(ls -p | grep -v /)"}, 1, "file with password");
	}
	else if ((action == "uninstall-completions") && argc == 3)
	{
		std::string str("complete -c ");
		str += argv[2];
		completion_remove_all_lines_with(str);
	}
	else
	{
		help(stdout, argv[0]);
	}
	
	default_();
	
	return 0;
}
