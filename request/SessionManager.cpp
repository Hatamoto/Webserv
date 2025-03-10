#include "SessionManager.hpp"

SessionManager::SessionManager() {
    std::cout << "SessionManager initialized: _sessions.size() = " << _sessions.size() << std::endl;
}

SessionManager::~SessionManager() {
	clearSessions();
    std::cout << "SessionManager destructed." << std::endl;
}

void SessionManager::clearSessions() {
	_sessions.clear();
	std::cout << "After clearSessions: _sessions.size() = " << _sessions.size() << std::endl;
}

void SessionManager::cleanUpExpiredSessions() {
    auto now = std::chrono::system_clock::now();

    for (auto it = _sessions.begin(); it != _sessions.end(); ) {
        const auto& sessionId = it->first;
        const auto& sessionData = it->second; // avoids using []operator for performance (no lookup necessary)

		// Possibly add culling of older sessions IF number of active sessions exceeds
		// a certain large number, to avoid bombarding with setting more and more cookies
		// by repeating a few requests then deleting the cookie at clients end
		// currently no way to know which sessions are 'really active'
        if (now > sessionData.expirationTime) {
            // Print details of the expired session
            std::cout << "Expired session: " << sessionId
                      << ", is dummy: " << (sessionData.isDummy ? "true" : "false") 
					  << " was deleted." << std::endl;

            // Erase the session from the map
            it = _sessions.erase(it); // safer in the loop than passing to another function.
        } else {
            ++it;
        }
    }

	// Cull old sessions if active sessions exceed the threshold
    if (_sessions.size() > MAX_ACTIVE_SESSIONS) {
		cullSessions();
    }
}

void SessionManager::cullSessions() {
	    if (_sessions.size() > MAX_ACTIVE_SESSIONS) {
        std::cout << "Culling sessions: Too many active sessions (" 
                  << _sessions.size() << ")." << std::endl;
		// Cull all dummy sessions first
		size_t dummySessionsCulled = 0;
		for (auto it = _sessions.begin(); it != _sessions.end();) {
            if (it->second.isDummy) {
                it = _sessions.erase(it);
				++dummySessionsCulled;
            } else {
                ++it;
            }
		}
		if (_sessions.size() <= TARGET_SESSIONS) {
			std::cout << "Culling complete: Culled " << dummySessionsCulled << " dummy sessions." << std::endl;
			return;
		}
		size_t sessionsToRemove = _sessions.size() - TARGET_SESSIONS;
        // Collect all sessions into a vector for sorting
        std::vector<std::pair<std::string, SessionData>> sessions(
            _sessions.begin(), _sessions.end()
        );
        // Sort by expiration time (oldest first)
        std::sort(sessions.begin(), sessions.end(), 
                  [](const auto& a, const auto& b) {
                      return a.second.expirationTime < b.second.expirationTime;
                  });
        for (size_t i = 0; i < sessionsToRemove; ++i) {
            _sessions.erase(sessions[i].first);
        }
		std::cout << "Culling complete: Culled " << sessionsToRemove + dummySessionsCulled << " sessions." << std::endl;
    }
}

static unsigned int seedWithTime() {
    std::random_device rd;
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return rd() ^ static_cast<unsigned int>(duration.count());
}

bool SessionManager::hasSession(const std::string& sessionId) const {
    return _sessions.find(sessionId) != _sessions.end();
}

std::string SessionManager::createDummySession() { // add more time before cleanup later
    std::string sessionId = generateSessionId();
    _sessions[sessionId] = { {}, std::chrono::system_clock::now(), std::chrono::system_clock::now() + std::chrono::minutes(1), true };
    return sessionId;
}

bool SessionManager::isValidSession(const std::string& sessionId) const {
    bool valid = _sessions.find(sessionId) != _sessions.end();
    std::cout << "Session validation for ID: " << sessionId << " - " << (valid ? "Valid" : "Invalid") << std::endl;
    return valid;
}

void SessionManager::validateAndExtendSession(const std::string& sessionId) {
	_sessions[sessionId].expirationTime = std::chrono::system_clock::now() + std::chrono::hours(1);
	_sessions[sessionId].isDummy = false;
}

SessionData& SessionManager::getSession(const std::string& sessionId) {
    return _sessions.at(sessionId); // Throws std::out_of_range if sessionId is not found
}

std::string SessionManager::generateSessionId() const {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    unsigned int seed = seedWithTime();
    static thread_local std::mt19937 gen(seed);
    static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string sessionId;

    // Loop to ensure uniqueness
    do {
        sessionId.clear();
        for (int i = 0; i < 16; ++i) {
            int index = dist(gen);
            sessionId += charset[index];
        }
    } while (_sessions.find(sessionId) != _sessions.end());
	std::cout << "Generated session ID: " << sessionId << std::endl;

    return sessionId;
}

void SessionManager::printSessions() const {
	for (const auto& [sessionId, sessionData] : _sessions) { // Use sessionData directly
		std::cout << "Session ID: " << sessionId << std::endl;

		// Iterate over the data map inside SessionData
		for (const auto& [key, value] : sessionData.data) {
			std::cout << "  " << key << ": " << value << std::endl;
		}

        // Print creation and expiration time in human-readable format
		std::time_t creationTimeT = std::chrono::system_clock::to_time_t(sessionData.creationTime);
		std::cout << "  Creation Time: " << std::put_time(std::localtime(&creationTimeT), "%Y-%m-%d %H:%M:%S") << std::endl;

		std::time_t expirationTimeT = std::chrono::system_clock::to_time_t(sessionData.expirationTime);
		std::cout << "  Expiration Time: " << std::put_time(std::localtime(&expirationTimeT), "%Y-%m-%d %H:%M:%S") << std::endl;

		std::cout << "  Considered dummy: " << sessionData.isDummy << std::endl;
	}
}

