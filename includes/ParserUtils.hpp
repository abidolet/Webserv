#ifndef PARSERUTILS_HPP
#define PARSERUTILS_HPP

#include <string>
#include <vector>

struct Server;


namespace Utils
{
	
	int LevenshteinDistance(const std::string& s1, const std::string& s2);
	std::string strUnite(std::vector<std::string> strs, std::string separator);
	std::vector<std::string> strsplit(std::string str, const char c);
	std::string findClosest(std::string str, std::vector<std::string> options);
	int getStrValue(std::string str);
	std::string strtrim(std::string str, const std::string set);
	std::vector<std::string> read_file(std::ifstream& stream);
	bool	should_add_line(std::string line);

	std::vector<std::string> toLowers(std::vector<std::string> strs);
	std::string toLower(std::string str);

	bool is_number(std::vector<std::string> strs);
	bool is_number(std::string str);
	bool isonly(std::string str, std::string set);

	bool fileAccess(const std::string& path);
	bool dirAccess(const std::string& path);
	std::string findFileFolder(const std::string& filepath);

	void printServConfig(Server serv);
	void printFile(const std::vector<std::string> &file);

}

#endif // PARSERUTILS_HPP
