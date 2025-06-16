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

	bool	dirAssert(const std::vector<std::string> &shouldBeFound);
	void	nameAssert(const std::vector<std::string> &shouldBeFound);

	Block*	searchBlockByName(const std::string& name);

	std::string getName() { return block_name.substr(0, block_name.find(" ")); }

	std::string					block_name;

	std::vector<std::string>	directives;
	std::vector<Block>			inners;
};

#endif
