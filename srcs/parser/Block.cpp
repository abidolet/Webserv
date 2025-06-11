#include "Block.hpp"
#include "Parser.hpp"
#include "ParserTools.hpp"

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
        throw Parser::TooMuchDirectiveException("path", *this);
    ref = found[0];
}

bool Block::blockAssert(std::vector<std::string> shouldBeFound, std::string blockName)
{
    std::vector<Block>::iterator block = inners.begin();
    while (block != inners.end())
    {
        if (block->block_name.substr(0, block->block_name.find(" ")) != blockName)
            throw Parser::InvalidArgumentException(block->block_name, blockName);

        std::vector<std::string>::iterator it = block->directives.begin();
        std::string	firstWord;
        while (it != block->directives.end())
        {
            firstWord = it->substr(0, it->find(" "));

            size_t i = 0;
            for ( ; i < shouldBeFound.size() ; i++)
            {
                if (firstWord == shouldBeFound[i])
                    break;
            }
            if (i == shouldBeFound.size())
                throw Parser::InvalidArgumentException(firstWord, Tools::findClosest(firstWord, shouldBeFound));

            ++it;
        }
        ++block;
    }

    return true;
}
