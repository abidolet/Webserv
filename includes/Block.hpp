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

	bool	blockAssert(const std::vector<std::string> &shouldBeFound, const std::string &blockName);
	Block*	searchBlockByName(const std::string& name);

	std::string					block_name;

	std::vector<std::string>	directives;
	std::vector<Block>			inners;
};

#endif
