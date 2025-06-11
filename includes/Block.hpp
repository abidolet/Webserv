#ifndef BLOCK_HPP
#define BLOCK_HPP


#include <vector>
#include <string>

class Block
{
public:
	Block(std::string name) : block_name(name) {};

	std::vector<std::string>	loadDirectives(const std::string& keyword);
	void						loadSingleDirective(const std::string& keyword, std::string& ref);

	bool blockAssert(std::vector<std::string> shouldBeFound, std::string blockName);


	std::string					block_name;

	std::vector<std::string>	directives;
	std::vector<Block>			inners;
};

#endif
