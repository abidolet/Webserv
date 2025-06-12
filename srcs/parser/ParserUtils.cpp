#include "ParserUtils.hpp"
#include "Webserv.hpp"
#include "Log.hpp"

#include <cmath>
#include <fstream>

namespace Utils
{
	bool	should_add_line(std::string line)
	{
		for (size_t i = 0; i < line.size(); i++)
		{
			if (line[i] != ' ' && line[i] != '\t')
				return true;
		}
		return false;
	}

	std::vector<std::string> read_file(std::ifstream& stream)
	{
		std::vector<std::string> file;
		std::string	line;

		while (std::getline(stream, line))
		{
			file.push_back(line);
		}

		return file;
	}

	std::string strtrim(std::string str, const std::string set)
	{
		str = str.substr(str.find_first_not_of(set));
		str = str.substr(0, str.find_last_not_of(set) + 1);
		return str;
	}

	int getStrValue(std::string str)
	{
		int		value = 0;

		for (size_t i = 0; i < str.size(); i++)
		{
			value += str[i];
		}
		return value;
	}

	std::vector<std::string> strsplit(std::string str, const char c)
	{
		std::vector<std::string>	split;

		for (size_t i = 0; i < str.size() ; )
		{
			while (str[i] == c)
				i++;
			if (!str[i])
				break;
			std::string tmp;
			while (str[i] && str[i] != c)
			{
				tmp += str[i];
				i++;
			}
			split.push_back(tmp);
		}
		return split;
	}

	std::string strUnite(std::vector<std::string> strs, std::string separator)
	{
		std::string str;

		for (size_t i = 0; i < strs.size(); i++)
		{
			str += strs[i];
			if (i + 1 < strs.size())
				str += separator;
		}
		return str;
	}

	std::string findClosest(std::string str, std::vector<std::string> options)
	{
		int idx = 0;
		int	str_val = getStrValue(str);
		std::vector<std::string> strs;

		str = toLower(str);
		strs = toLowers(options);

		for (size_t i = 0; i < options.size(); i++)
		{
			if (std::abs(getStrValue(strs[i]) - str_val) < std::abs(getStrValue(strs[idx]) - str_val))
			{
				idx = i;
			}
		}
		return options[idx];
	}

	std::string toLower(std::string str)
	{
		for (size_t i = 0; i < str.size(); i++)
		{
			str[i] = std::tolower(str[i]);
		}
		return str;
	}

	std::vector<std::string> toLowers(std::vector<std::string> strs)
	{
		std::vector<std::string>::iterator it = strs.begin();
		while (it != strs.end())
		{
			for (size_t i = 0; i < it->size(); i++)
			{
				(*it)[i] = std::tolower((*it)[i]);
			}
			it++;
		}
		return strs;
	}

	bool is_number(std::string str)
	{
		for (size_t i = 0; i < str.size(); i++)
		{
			if (std::isdigit(str[i]) == false)
				return false;
		}
		return true;
	}

	bool is_number(std::vector<std::string> strs)
	{
		std::vector<std::string>::iterator it = strs.begin();

		while (it != strs.end())
		{
			for (size_t i = 0; i < it->size(); i++)
			{
				if (std::isdigit((*it)[i]) == false)
					return false;
			}
			it++;
		}
		return true;
	}

	bool fileAccess(const std::string& path)
	{
		std::ifstream stream;
		stream.open(path.c_str());

		bool value = !stream.fail();
		stream.close();
		return value;
	}

	void printServConfig(Server serv)
	{
		Log(Log::DEBUG) << "server config:" << Log::endl();
		Log(Log::DEBUG) << "\t|-> root:" << serv.root << Log::endl();
		Log(Log::DEBUG) << "\t|-> server_name:" << serv.server_name << Log::endl();
		Log(Log::DEBUG) << "\t|-> client_max_body_size:" << serv.client_max_body_size << Log::endl();
		Log(Log::DEBUG) << "\t|-> allowed methods:" << Utils::strUnite(serv.allowed_methods, ",") << Log::endl();

		// printing redirections
		{
			Log(Log::DEBUG) << "\t|->" << serv.error_pages.size() << "error_pages" << Log::endl();
			std::map<int, std::string>::iterator it = serv.error_pages.begin();
			for ( ; it != serv.error_pages.end(); ++it)
			{
				Log(Log::DEBUG) << "\t|\t|-> {" << it->first << "=>" << it->second  << "}" << Log::endl();
			}
		}

		// printing listens
		{
			Log(Log::DEBUG) << "\t|->" << serv.listen.size() << "listens" << Log::endl();
			std::map<std::string, int>::iterator it = serv.listen.begin();
			for ( ; it != serv.listen.end(); ++it)
			{
				Log(Log::DEBUG) << "\t|\t|-> {" << it->first << "=>" << it->second  << "}" << Log::endl();
			}
		}

		// printing each location
		{
			Log(Log::DEBUG) << "\t|" << Log::endl();
			Log(Log::DEBUG) << "\t|->" << serv.locations.size() << "locations" << Log::endl();
			std::vector<Location>::iterator it = serv.locations.begin();
			for ( ; it != serv.locations.end(); ++it)
			{
				Log(Log::DEBUG) << "\t|---- locations:" << it->root << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> path:" << it->path << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> root:" << it->root << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> index:" << it->index << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> allowed methods:" << Utils::strUnite(it->allowed_methods, ",") << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> cgi pass: [" << it->cgi_pass << "]" << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> cgi extension: [" << it->cgi_extension << "]" << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> is cgi:" << it->is_cgi  << Log::endl();
			}
		}
	}
}

