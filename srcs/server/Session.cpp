#include "Webserv.hpp"
#include "ParserUtils.hpp"

Session Session::registerSession(const std::string& key)
{
	Session session;

	session.key = key;
	session.uid = -1;
	session.visitCount = 0;

	return session;
}

Session Session::stringToSession(std::string& str)
{
	std::vector<std::string> split = Utils::strsplit(str, ',');
	if (split.size() != 3)
		throw std::runtime_error("broken session file ");
	char* endl;
	Session session;

	size_t idx = str.find_first_of(",");
	session.key = str.substr(0, idx);
	size_t last_i = idx;
	idx = str.find_first_of(",", last_i);
	session.uid = std::strtol(str.substr(last_i, idx).c_str(), &endl, 10);
	last_i = idx;
	session.visitCount = std::strtol(str.substr(last_i, str.size() - last_i).c_str(), &endl, 10);

	return session;
}

std::string Session::sessionToString(const Session& session)
{
	std::stringstream ss;
	ss << session.key << "," << session.uid << "," << session.visitCount << "\n";
	return ss.str();
}

Session* Session::find(std::vector<Session>& sessions, size_t uid)
{
	std::vector<Session>::iterator it = sessions.begin();
	for ( ; it != sessions.end(); ++it)
	{
		if (it->uid == uid)
			return &(*it);
	}
	return NULL;
}

Session* Session::find(std::vector<Session>& sessions, const std::string& key)
{
	std::vector<Session>::iterator it = sessions.begin();
	for ( ; it != sessions.end(); ++it)
	{
		if (it->key == key)
			return &(*it);
	}
	return NULL;
}
