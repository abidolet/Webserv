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
            results.push_back(it->substr(it->find(" ") + 1, it->size() - it->find(" ") + 1));
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

bool Block::blockAssert(const std::vector<std::string> &shouldBeFound, const std::string &blockName)
{
    std::vector<Block>::iterator block = inners.begin();
    for ( ; block != inners.end(); ++block)
    {
        if (block->block_name.substr(0, block->block_name.find(" ")) != blockName)
            throw Parser::InvalidArgumentException(block->block_name, blockName);

        std::string	firstWord;
        std::vector<std::string>::iterator it = block->directives.begin();
        for ( ; it != block->directives.end(); ++it)
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
    }

    return true;
}
