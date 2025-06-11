#include "ParserTools.hpp"
#include <cmath>
#include "fstream"
#include <iostream>

namespace Tools
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

#if OLD
	// TODO: pour le closest a la place do calculer la value total de la str, juste calculer la diff entre chaque lettre, comme ca abc plus proche de abd que cba 
	// TODO  dans le system acutel abc == cba vu que y'a les meme lettres
	std::string findClosest(std::string str, std::vector<std::string> options)
	{
		std::string result;
		int bestMatch = 9999;

		std::vector<std::string>::iterator it = options.begin();
		for ( ; it != options.end(); ++it)
		{
			int totalDiff = 0;

			size_t i = 0;
			while (i < str.size() || i < it->size())
			{
				if (i < str.size() && i < it->size())
					totalDiff += abs(tolower((*it)[i]) - tolower(str[i]));

				i++;
			}
			if (totalDiff < bestMatch)
			{
				bestMatch = totalDiff;
				result = *it;
			}
		}

		return result;
	}
#endif

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
		return !stream.fail();
	}
}
