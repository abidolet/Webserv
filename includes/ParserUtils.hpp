#include <string>
#include <vector>

class Server;

namespace Utils
{

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

	bool fileAccess(const std::string& path);

	void printServConfig(Server serv);

}
