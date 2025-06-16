#include "Block.hpp"
#include "Parser.hpp"
#include "ParserUtils.hpp"

std::vector<std::string> Block::loadDirectives(const std::string& keyword)
{
	std::vector<std::string> results;
	std::string firstWord;

	std::vector<std::string>::iterator it = directives.begin();
	while (it != directives.end())
	{
		firstWord = it->substr(0, it->find(" "));
		if (firstWord == keyword)
		{
			if (it->find(" ") == (size_t)-1)
				throw Parser::InvalidDirectiveException(firstWord, "Directive has no value");

			std::string tmp = it->substr(it->find(" ") + 1, it->size() - it->find(" ") + 1);
			results.push_back(Utils::strtrim(tmp, " \t"));
		}
		++it;
	}
	return results;
}

void Block::loadSingleDirective(const std::string &keyword, std::string& ref)
{
	std::vector<std::string> found = loadDirectives(keyword);
	if (found.empty())
		return;
	if (found.size() > 1)
		throw Parser::TooMuchDirectiveException(keyword, *this);
	ref = found[0];
}

void Block::nameAssert(const std::vector<std::string> &shouldBeFound)
{
	std::vector<Block>::iterator block = inners.begin();
	for ( ; block != inners.end(); ++block)
	{
		std::string name = block->block_name.substr(0, block->block_name.find(" "));
		size_t i = 0;
		for ( ; i < shouldBeFound.size(); i++)
		{
			if (name == shouldBeFound[i])
				break;
		}
		if (i == shouldBeFound.size())
			throw Parser::InvalidArgumentException(name, Utils::findClosest(name, shouldBeFound));
	}
}

bool Block::dirAssert(const std::vector<std::string> &shouldBeFound)
{
	std::string	firstWord;
	std::vector<std::string>::iterator it = directives.begin();
	for ( ; it != directives.end(); ++it)
	{
		firstWord = it->substr(0, it->find(" "));

		size_t i = 0;
		for ( ; i < shouldBeFound.size() ; i++)
		{
			if (firstWord == shouldBeFound[i])
				break;
		}
		if (i == shouldBeFound.size())
			throw Parser::InvalidArgumentException(firstWord, Utils::findClosest(firstWord, shouldBeFound));
	}
	return true;
}

Block* Block::searchBlockByName(const std::string& name)
{
	std::vector<Block>::iterator it = inners.begin();
	for ( ; it != inners.end(); ++it)
	{
		if (it->block_name == name)
			return &(*it);
	}
	return NULL;
}
