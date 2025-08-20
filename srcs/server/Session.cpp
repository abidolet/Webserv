#include "Webserv.hpp"
#include "ParserUtils.hpp"

Session::Session()
	: uid(0), visitCount(1) { }

Session::Session(unsigned int _uid)
{
	uid = _uid;
	visitCount = 1;
}

Session Session::stringToSession(std::string& str)
{
	std::vector<std::string> split = Utils::strsplit(str, ',');
	char* endl;
	Session session;
	
	size_t idx = str.find_first_of(",");
	session.uid = std::strtol(str.substr(0, idx).c_str(), &endl, 10);
	size_t last_i = idx + 1;
	session.visitCount = std::strtol(str.substr(last_i, str.size() - last_i).c_str(), &endl, 10);

	return session;
}

std::string Session::sessionToString()
{
	std::stringstream ss;
	ss << uid << "," << visitCount;
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

std::ostream& operator<<(std::ostream& stream, const Session& session)
{
	stream << ",uid" << session.uid << ",visitCount:" << session.visitCount;
	return stream;
}

