
#include <iostream>
#include <filesystem>
#include "cgi_request.hpp"

cgiRequest::cgiRequest(const std::string &path, const std::string &method, const std::string &queryString, const std::string &protocol, const std::string &bodyData, const std::string &contentType) : script_path(path), request_method(method), httpProtocol(protocol), body_data(bodyData)
{
	std::cout << "cgi request constructor" << std::endl;
	if (!queryString.empty())
	{
		query_str = queryString;
	}
	else
		query_str = "";
	setEnvironmentVariables(contentType);
}

cgiRequest::~cgiRequest()
{
	std::cout << "cgi request deconstructor" << std::endl;
}

void	cgiRequest::setEnvironmentVariables(const std::string &contentType)
{
	env["REQUEST_METHOD"] = request_method;
	env["SCRIPT_PATH"] = script_path;
	env["QUERY_STRING"] = query_str;
	env["HTTP_PROTOCOL"] = httpProtocol;

	if (request_method == "POST")
	{
		if (!contentType.empty())
			env["CONTENT_TYPE"] = contentType; // Use the actual content type from the request
		else
			env["CONTENT_TYPE"] = "application/x-www-form-urlencoded"; // Default fallback
		env["CONTENT_LENGTH"] = std::to_string(body_data.size());
	}
}

void	cgiRequest::printEnv(char **envp) // debugging purposes only
{
	for (int i = 0; envp[i]; i++)
	{
		std::cout << "env[" << i << "] = " << envp[i] << std::endl;
	}
}

void	cgiRequest::printCgiRequestData()
{
	std::cout << "Script path: " << script_path << std::endl;
	std::cout << "Request method: " << request_method << std::endl;
	std::cout << "Query string: " << query_str << std::endl;
	std::cout << "Http protocol: " << httpProtocol << std::endl;

	char **envp = buildEnv();

	if (envp)
	{
		printEnv(envp);
		for (int i = 0; envp[i]; i++)
		{
			delete[] envp[i];
		}
		delete[] envp;
	}
}

char	**cgiRequest::buildEnv()
{
	char	**envp = new char*[env.size() + 1];
	size_t	i = 0;

	for(const auto &pair : env)
	{
		std::string	envString = pair.first + "=" + pair.second;
		envp[i] = new char[envString.size() + 1];
		std::strcpy(envp[i], envString.c_str());
		i++;
	}
	envp[i] = nullptr;
	return envp;
}

void	cleanEnv(char **envp)
{
	if (envp)
	{
		for (int i = 0; envp[i]; i++)
		{
			delete[] envp[i];
		}
		delete[] envp;
	}
}

std::string	cgiRequest::getScriptPath()
{
	return script_path;
}

std::string	cgiRequest::getRequestMethod()
{
	return request_method;
}

std::string	cgiRequest::getQueryString()
{
	return query_str;
}

std::string	cgiRequest::getProtocol()
{
	return httpProtocol;
}

std::string cgiRequest::getBodyData()
{
	return body_data;
}

std::map<std::string, std::string>	cgiRequest::getEnv()
{
	return (env);
}

int	cgiRequest::isValidCgi()
{
	if (!std::filesystem::exists(script_path))
	{
		std::cerr << "Cgi script not found at: " << script_path << std::endl;
		return 1;
	}
	std::ifstream file;

	file.open(script_path);

	if (file)
	{
		std::cerr << "Great success\n";
		return 0;
	}
	else
	{
		std::cerr << "Cgi script not opening at: " << script_path << std::endl;
		return 2;
	}
}

static bool	ensureFolderExists(const std::string &folderPath)
{
	if (!std::filesystem::exists(folderPath))
	{
		if (!std::filesystem::create_directory(folderPath))
		{
			std::cerr << "Failed to create directory: " << folderPath << std::endl;
			return false;
		}
	}
	return true;
}

int	cgiRequest::execute()
{
	int status = 0;
	if ((status = isValidCgi()) == 0)
	{
		if (!ensureFolderExists("./html"))
			return 500;
		if (!ensureFolderExists("./html/tmp"))
			return 500;
		pid_t pid = fork();
		if (pid == -1)
		{
			std::cerr << "Failed to create child process for cgi script" << std::endl;
			return 500;
		}
		else if (pid == 0)
		{
			std::string	outputPath = "./html/tmp/cgi_output.html";
			int	outputFile = open(outputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (outputFile == -1)
			{
				std::cerr << "Failed to open output file for CGI script" << std::endl;
				exit(500);
			}
			dup2(outputFile, STDOUT_FILENO);
			close(outputFile);
			if (request_method == "POST" && !body_data.empty())
			{
				std::string	tmpFilePath = "./html/tmp/cgi_input.html";
				std::ofstream	tmpFile(tmpFilePath);
				if (!tmpFile)
				{
					std::cerr << "Failed to open temporary file for CGI script" << std::endl;
					exit(500);
				}
				tmpFile.write(body_data.c_str(), body_data.size());
				tmpFile.close();

				int	inputFile = open(tmpFilePath.c_str(), O_RDONLY);
				if (inputFile == -1)
				{
					std::cerr << "Failed to open input file for CGI script" << std::endl;
					exit(500);
				}
				dup2(inputFile, STDIN_FILENO);
				close(inputFile);
			}
			char	*args[] = {const_cast<char *>(script_path.c_str()), nullptr};
			char	**envp = buildEnv();
			// std::cerr << "running execve script: " << script_path.c_str() << std::endl; // debugging
			execve(script_path.c_str(), args, envp);
			perror("execve"); // Lisää tämä rivi, jotta näet tarkemman virheen syyn
			std::cerr << "Execve failed!" << std::endl;
			cleanEnv(envp);
			exit(500);
		}
		else
		{
			const int	timeout = 5;
			int			status = 0;

			pid_t		result = waitpid(pid, &status, WNOHANG);
			for (int i = 0; i < timeout && result == 0; i++)
			{
				sleep(1);
				result = waitpid(pid, &status, WNOHANG);
			}
			if (result == 0)
			{
				std::cerr << RED << "Timeout occurred. Killing CGI script process: " << pid << DEFAULT << std::endl;
				kill(pid, SIGKILL);
				waitpid(pid, &status, 0);
				return 504;
			}
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
				return 0;
			else
				return 500;
		}
	}
	else if (status == 1)
		return 404;
	else
		return 503;
	return 1;
}



std::string	findQueryStr(const std::string &path)
{
	std::string query;
	size_t i = path.find('?');
	if (i != std::string::npos)
		query = path.substr(i + 1);
	else
		query = "";
	// std::cout << "debugging" << std::endl;
	return(query);
}

std::string	findPath(const std::string &path)
{
	std::string	directPath;
	size_t		i = path.find('?');

	if (i != std::string::npos)
		directPath = path.substr(0, i);
	else
		directPath = path;
	directPath = '.' + directPath;
	return directPath;
}

void	handleCgiRequest(Request &req, const Socket &socket)
{
	std::string	queryString = findQueryStr(req.getUri());
	std::string	directPath = findPath(req.getUri());
	std::cout << "DIRECTPATH: " << directPath << std::endl;
	cgiRequest	cgireq(directPath, req.getMethod(), queryString, req.getVersion(), req.getBody(), req.getContentType());
	int			executeResult = cgireq.execute();
	if (executeResult != 0)
		req.setPath(socket.getServer().getErrorPage(executeResult));
	switch (executeResult)
	{
	case 0:
		req.setPath("/cgi_output.html");
		break;
	case 404:
		req.setState(State::CGI_NOT_FOUND);
		break;
	case 500:
		req.setState(State::CGI_ERROR);
		break;
	case 504:
		req.setState(State::TIMEOUT);
		break;
	default:
		req.setState(State::CGI_NOT_PERMITTED);
	}
}
