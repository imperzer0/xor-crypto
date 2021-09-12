#include <map>
#include <thread>
#include "archive_and_encrypt.hpp"
#include <fish-completions>
#include <parse-arguments>

#include "functions.hpp"

namespace anm
{
	static constexpr const char* action = "--action";
	static constexpr const char* input = "--input";
	static constexpr const char* output = "--output";
	static constexpr const char* passwd = "--passwd";
	static constexpr const char* passwd_file = "--passwd-file";
}

int main(int argc, const char** argv)
{
	setting1();
	setting2();
	
	if (argc < 2)
	{
		help(stdout, argv[0]);
	}
	
	arguments args(argc, argv);
	
	if (!args.exists_with_value(anm::action))
	{
		help(stdout, argv[0]);
	}
	std::string action = args.get_arg_value(anm::action);
	
	if (action == "encrypt" && argc >= 4 && argc <= 6)
	{
		if (!args.exists_with_value(anm::input) || !args.exists_with_value(anm::output))
		{
			help(stdout, argv[0]);
		}
		
		std::string input(args.get_arg_value(anm::input)), output(args.get_arg_value(anm::output)), passwd;
		
		if (!args.exists_with_value(anm::passwd) && !args.exists_with_value(anm::passwd_file))
		{
			std::cout << "type password: ";
			std::string console_input = read_password();
			passwd = console_input;
		}
		
		bool is_from_stdin = args.get_arg_value(anm::input) == "&stdin";
		
		if (is_from_stdin)
		{
			input = read_text();
			if (args.exists_with_value(anm::passwd))
			{
				passwd = args.get_arg_value(anm::passwd);
				encrypt_entry_string(input, output, passwd);
				if (args.exists_with_value(anm::passwd_file))
				{
					passwd = args.get_arg_value(anm::passwd_file);
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
			else if (args.exists_with_value(anm::passwd_file))
			{
				passwd = args.get_arg_value(anm::passwd_file);
				encrypt_entry_string_with_file(input, output, passwd);
			}
			else
			{
				encrypt_entry_string_with_file(input, output, passwd);
			}
		}
		else
		{
			if (args.exists_with_value(anm::passwd))
			{
				passwd = args.get_arg_value(anm::passwd);
				encrypt_entry(input, output, passwd);
				if (args.exists_with_value(anm::passwd_file))
				{
					passwd = args.get_arg_value(anm::passwd_file);
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
			else if (args.exists_with_value(anm::passwd_file))
			{
				passwd = args.get_arg_value(anm::passwd_file);
				encrypt_entry_with_file(input, output, passwd);
			}
			else
			{
				encrypt_entry_with_file(input, output, passwd);
			}
		}
	}
	else if (action == "decrypt" && argc >= 4 && argc <= 6)
	{
		if (!args.exists_with_value(anm::input) || !args.exists_with_value(anm::output))
		{
			help(stdout, argv[0]);
		}
		
		std::string input(args.get_arg_value(anm::input)), output(args.get_arg_value(anm::output)), passwd;
		
		if (!args.exists_with_value(anm::passwd) && !args.exists_with_value(anm::passwd_file))
		{
			std::cout << "type password: ";
			std::string console_input = read_password();
			passwd = console_input;
		}
		
		bool is_from_stdin = args.get_arg_value(anm::input) == "&stdin";
		
		if (is_from_stdin)
		{
			input = read_text();
			if (args.exists_with_value(anm::passwd))
			{
				passwd = args.get_arg_value(anm::passwd);
				decrypt_entry_string(input, output, passwd);
				if (args.exists_with_value(anm::passwd_file))
				{
					passwd = args.get_arg_value(anm::passwd_file);
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
			else if (args.exists_with_value(anm::passwd_file))
			{
				passwd = args.get_arg_value(anm::passwd_file);
				decrypt_entry_string_with_file(input, output, passwd);
			}
			else
			{
				decrypt_entry_string_with_file(input, output, passwd);
			}
		}
		else
		{
			if (args.exists_with_value(anm::passwd))
			{
				passwd = args.get_arg_value(anm::passwd);
				decrypt_entry(input, output, passwd);
				if (args.exists_with_value(anm::passwd_file))
				{
					passwd = args.get_arg_value(anm::passwd_file);
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
			else if (args.exists_with_value(anm::passwd_file))
			{
				passwd = args.get_arg_value(anm::passwd_file);
				decrypt_entry_with_file(input, output, passwd);
			}
			else
			{
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
		completions comp(argv[2]);
		comp.set("help", nullptr, "print help");
		comp.set(
				"action", new const char* []{"encrypt", "decrypt", "info", "help", "install-completions", "uninstall-completions", nullptr},
				"action"
		);
		comp.set(
				"input", new const char* []{"(ls -p | grep -v /)", "\\\\&stdin", nullptr}, "input file or directory",
				new const char* []{"--action=encrypt", nullptr}
		);
		comp.set(
				"input", new const char* []{"(ls -p | grep -v /)", "\\\\&stdin", nullptr}, "input file",
				new const char* []{"--action=decrypt", nullptr}
		);
		comp.set("output", new const char* []{"(ls -p | grep -v /)", nullptr}, "output file");
		comp.set("passwd", nullptr, "password");
		comp.set("passwd-file", new const char* []{"(ls -p | grep -v /)", nullptr}, "file with password");
	}
	else if ((action == "uninstall-completions") && argc == 3)
	{
		completions comp(argv[2]);
		std::string str("complete -c ");
		str += argv[2];
		str += " ";
		comp.uninstall();
	}
	else
	{
		help(stdout, argv[0]);
	}
	
	default_();
	
	return 0;
}
