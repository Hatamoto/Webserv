
#ifndef REQUEST_HPP
# define REQUEST_HPP


#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <iomanip>
#include <regex>
#include "../utils.hpp"
#include "../parsing/LocationConfig.hpp"
#include "../sockets/socket.hpp"


void checkline(std::string line);

enum class State {
    REQUEST_LINE,
    HEADERS,
    UNCHUNK,
    BODY,
    MULTIPARTDATA,
    COMPLETE,
    ERROR,
    INCOMPLETE
};

struct MultipartData
{
	std::string					name;
	std::string					filename;
	std::string					contentType;
	std::vector<char>			data = {};
	std::string					boundary;
    std::map<std::string, std::string>    headers;
};

class Request {
    public:
        Request();
        ~Request();
        Request(const std::string &rawRequest);

        void                                reset();
        void                                printRequest();
        void                                parseRequest(std::string &rawReques, const Socket &socket);
        void                                setPath(std::string newPath);
        void                                setState(State state);
        void                                setReceived(bool state);
        std::string                         getMethod() const;
        std::string                         getUri() const;
        std::string                         getVersion() const;
        std::string                         getBody() const;
        std::map<std::string, std::string>  getHeaders() const;
		std::map<std::string, std::string> 	getCookies() const;
        const std::vector<MultipartData>    &getMultipartData() const { return multipartData; };
        State                               getState();
        std::string                         getContentType() const;
        bool                                isMultiPart() const;
		bool								hasHeader(const std::string& headerName) const;
        void                                validateHeaders();
        LocationConfig                      getLocation() const;
        std::map<std::string, std::string>  getQueryParams() const;
        int                                 getErrorCode() const;
        void                                setErrorCode(const int code);
        Request &operator=(const Request &rhs);

    private:
        Request(const Request &other);
        size_t                            maxBodySize;
        bool                                received;
        bool                                chunked;
        bool                                _isMultiPart;
        State                               currentState;
        size_t                              contentLength;
        size_t                              body_size;
        int                                 errorCode;
        std::string                         requestBuffer;
        std::string                         rawRequest;
        std::string                         body;
        std::string                         rawChunkedData;
        std::string                         boundary;
        std::string                         method;
        std::string                         uri;
        std::string                         version;
        std::string                         contentType;
        std::istringstream                  requestStream;
        std::map<std::string, std::string>  headers;
        std::vector<MultipartData>          multipartData;
        std::map<std::string, std::string>  queryParams;
        LocationConfig                      location;

        void                                prepareRequest();
        void                                parseMultipartData();
        void                                parseHeaders();
        void                                parseBody();
        void                                handleError(int code, const std::string& errorMsg);
        void                                parseRequestLine(const Socket &socket);
        void                                parseQueryString();
        void                                printMultipartdata();
        void                                createMultipartBody(MultipartData &multipartData, std::istringstream &rawMultipartData);
        void                                createMultipartHeaders(MultipartData &multipartData, std::istringstream &rawDataStream);
        void                                parseChunks();
        bool                                isValidHeaderKey(const std::string& key);
        bool                                isValidHeaderValue(const std::string& key, const std::string& value);
        MultipartData                       createData(std::string &rawData);
        LocationConfig                      findLocation(const std::string &uri, const Socket &socket);
        bool                                isMethodAllowed(const std::vector<std::string>& allowedMethods, const std::string& method);
        bool                                isValidRequestLine(const std::string& requestLine);
} ;

# endif
