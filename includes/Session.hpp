#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <vector>

struct Session
{
	size_t	uid;
	size_t	visitCount;

	Session();
	Session(unsigned int _uid);

	std::string			sessionToString();
	static Session		stringToSession(std::string &str);
	static Session*		find(std::vector<Session> &sessions, size_t uid);
};
std::ostream& operator<<(std::ostream& stream, const Session& session);

#endif