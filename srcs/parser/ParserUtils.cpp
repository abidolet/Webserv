#include "ParserUtils.hpp"
#include "Webserv.hpp"
#include "Log.hpp"

#include <cmath>
#include <dirent.h>
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
			if (!line.empty())
			{
				std::string tmp = strtrim(line, " \t");
				if (!tmp.empty())
					file.push_back(tmp);
			}
		}

		return file;
	}

	std::string strtrim(std::string str, const std::string set)
	{
		if (str.find_first_not_of(set) == (size_t)-1)
				return "";
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
		int		idx = 0;
		uint	min = -1;

		std::vector<std::string> lowers;

		str = toLower(str);
		lowers = toLowers(options);

		for (size_t i = 0; i < options.size(); i++)
		{
			uint distance = LevenshteinDistance(str, lowers[i]);
			if (distance < min)
			{
				min = distance;
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

	bool isonly(std::string str, std::string set)
	{
		for (size_t j = 0; j < str.size(); j++)
		{
			size_t i = 0;
			for ( ; i < set.size(); i++)
			{
				if (set[i] == str[j])
					break;
			}
			if (i == set.size())
				return false;
		}
		return true;
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

	bool dirAccess(const std::string& path)
	{
		DIR* dir = opendir(path.c_str());
		if (dir)
		{
			closedir(dir);
			return true;
		}
		return false;
	}

	void printServConfig(Server serv)
	{
		Log(Log::DEBUG) << "[server config]:" << Log::endl();
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
			std::vector<Listen>::iterator it = serv.listen.begin();
			for ( ; it != serv.listen.end(); ++it)
			{
				Log(Log::DEBUG) << "\t|\t|-> {" << it->addr << "=>" << it->port  << "}" << Log::endl();
			}
		}

		// printing cookies
		{
			if (serv.cookies.size() > 0)
			{
				Log(Log::DEBUG) << "\t|->" << serv.cookies.size() << "cookies" << Log::endl();
				std::vector<std::string>::iterator it = serv.cookies.begin();
				for ( ; it != serv.cookies.end(); ++it)
				{
					Log(Log::DEBUG) << "\t|\t|-> {" << *it << "}" << Log::endl();
				}
			}
			else
				Log(Log::DEBUG) << "\t|->" << "no cookies" << BOLD << RED << "ﾍ( ´Д`)ﾉ" << Log::endl();
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
				if (!it->upload_dir.empty())
					Log(Log::DEBUG) << "\t|\t|-> upload dir:" << it->upload_dir << Log::endl();
				Log(Log::DEBUG) << "\t|\t|-> dir listing:" << (it->directoryListing ? "on" : "off")  << Log::endl();
				if (it->redirection.first != -1)
					Log(Log::DEBUG) << "\t|\t|-> redirection:" << it->redirection.first << "=>" << it->redirection.second << Log::endl();
				if (it->is_cgi)
				{
					Log(Log::DEBUG) << "\t|\t|-> cgi pass: [" << it->cgi_pass << "]" << Log::endl();
					Log(Log::DEBUG) << "\t|\t|-> cgi extension: [" << it->cgi_extension << "]" << Log::endl();
				}
			}
		}
	}

	std::string findFileFolder(const std::string& filepath)
	{
		return filepath.substr(0, filepath.find_last_of("/") + 1);
	}

	void printFile(const std::vector<std::string> &file)
	{
		for (size_t i = 0; i < file.size(); i++)
		{
			Log(Log::DEBUG) << file[i] << Log::endl();
		}
	}
}



