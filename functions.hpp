//
// Created by imper on 9/12/21.
//

#ifndef XOR_CRYPTO_FUNCTIONS_HPP
#define XOR_CRYPTO_FUNCTIONS_HPP

inline static void help(FILE* output_stream, const char* appname)
{
	size_t len = strlen(appname);
	
	char* spacing = new char[len + 1];
	
	for (int i   = 0; i < len; ++i)
	{
		spacing[i] = ' ';
	}
	spacing[len] = 0;
	
	fprintf(
			output_stream,
			"Usage:\n"
			"      %s --action=encrypt --input=<input_path>             --- Encrypt file or input using password.\n" // 1
			"      %s--output=<encrypted_filename> (--passwd=<password> --- Password has no length restrictions.\n" // s
			"      %s--passwd-file=<file>)                              --- If you want to encrypt data with token,\n" // s
			"      %s                                                   --- specify /dev/sdX1 partition of token\n" // s
			"      %s                                                   --- device in parameter --passwd-file.\n" // s
			"      %s                                                   --- Also if you want to specify additional\n" // s
			"      %s                                                   --- password pass it in --passwd parameter\n" // s
			"      or\n"
			"      %s --action=decrypt --input=<input_filename>         --- Decrypt file using the same password.\n" // 2
			"      %s--output=<output_filename> (--passwd=<password>    --- Password has no length restrictions.\n" // s
			"      %s--passwd-file=<password-file>)                     --- There are the same features as in en-\n" // s
			"      %s                                                   --- crypt\n" // s
			"      or\n"
			"      %s --action=info                                     --- Get more information how program works\n" // 3
			"      %s                                                   --- and test progress bar.\n" // s
			"      or\n"
			"      %s --action=install-completions <program_name>       --- Installs completions (if run from sudo\n" // 4
			"      %s                                                   --- - for all users, otherwise - only for\n" // s
			"      %s                                                   --- current user).\n" // s
			"      or\n"
			"      %s --action=uninstall-completions <program_name>     --- Uninstalls completions (if run from sudo\n" // 5
			"      %s                                                   --- - for all users, otherwise - only for\n" // s
			"      %s                                                   --- current user).\n\n" // s
			"      NOTE: If you've not specified --passwd or --passwd-file parameters for encrypting or decrypting,\n" // s
			"            program will ask password from stdin\n", // s
			// 1        s        s        s        s        s        s
			appname, spacing, spacing, spacing, spacing, spacing, spacing,
			// 2        s        s        s
			appname, spacing, spacing, spacing,
			// 3        s
			appname, spacing,
			// 4        s        s
			appname, spacing, spacing,
			// 5        s        s
			appname, spacing, spacing
	);
	exit(0);
}

constexpr const char* info_message[]{
		"< to enable fake progress bar press 'c' >\n"
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
		"| <33> | We always store only one of them - data (encrypted or original), password we memorize."
};

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

#endif //XOR_CRYPTO_FUNCTIONS_HPP
