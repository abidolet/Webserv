
#include "Webserv.hpp"
#include "ParserUtils.hpp"

#include <sys/stat.h>
#include <dirent.h>

File getFile(std::string filename, struct dirent* infos)
{
	struct stat st;
	stat(filename.c_str(), &st);

	return (File) {
		.name=infos->d_name,
		.size=(st.st_size),
	};
}

std::vector<File> getFilesInDir(const std::string path)
{
	std::vector<File> files;

	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		Log(Log::WARNING) << "Directory listing is enable but dir can't be open";
		return files;
	}
	struct dirent* info;
	while ((info = readdir(dir)) != NULL)
	{
		files.push_back(getFile(path + "/" + info->d_name, info)); // TODO: maybe add other info
	}
	closedir(dir);
	return files;
}

std::string getElt(const File& file, const std::string& path)
{
	std::string processed = Utils::processPath(path);
	if (processed.size() != 1)
		processed += "/";
	Log() << processed + " | " + file.name << Log::endl();
	std::stringstream ss;

	std::string uri = processed + file.name;
	ss << "<div class='elt'><a href='" << uri << "'>" \
	 << file.name << "</a> <p>" << file.size << "bytes</p></div>";
	return ss.str();
}

std::string getURL(HttpRequest& request)
{
	if (request.path.find(request.location.path) == (size_t)-1)
	{
		Log(Log::ERROR) << "path not found in request wtf" << Log::endl();
		return "/";
	}

	int idx = request.path.find(request.location.path) + request.location.path.size();
	std::string tmp = request.path.substr(idx);
	std::string final = request.location.root + tmp;
	return (final);
}

std::string getDirectoryListing(HttpRequest& request)
{
	std::stringstream ss;
	std::vector<File> files = getFilesInDir(request.path);

	ss << "<html>";
	ss << "<head>";
	ss << "	<style>";
	ss << "	.elt {display:flex; flex-direction:row; gap: 65px; align-items: center; justify-content: space-between; width: 33svw;}";
	ss << "	body {display:flex; flex-direction:column;}";
	ss << "	</style>";
	ss << "</head>";
	ss << "<body>";
	ss << "	<h1>" << request.path << "</h1>";
	ss << "	<div class='elt'><p>name </p> <p> | </p> <p>size</p></div>";
	for (size_t i = 0; i < files.size(); i++)
	{
		ss << getElt(files[i], getURL(request));
	}
	ss << "</body>";
	ss << "</html>";

	return (generatePage(200, ss.str(), ".html"));
}