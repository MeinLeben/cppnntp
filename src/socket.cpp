#include "socket.hpp"

namespace cppnntp {
	/**
	 * Constructor.
	 *
	 * @public
	 */
	socket::socket() : ssl_sock(NULL), tcp_sock(NULL) {
	}

	/**
	 * Destructor.
	 *
	 * @note Closes the socket connection.
	 * @public
	 */
	socket::~socket() {
		close();
	}

	/**
	 * Toggle cli output status.
	 * 
	 * @public
	 * 
	 * @param output = Turn cli output on or off.
	 */
	void socket::clioutput(const bool &output) {
		echocli = output;
	}

	/**
	 * Toggle compression status.
	 *
	 * @note This is used by class nntp's xfeaturegzip function.
	 * @public
	 *
	 * @param status = Set compression to true or false.
	 */
	void socket::togglecompression(const bool &status) {
		compression = status;
	}

	/**
	 * Return compression status.
	 *
	 * @public
	 */
	bool socket::compressionstatus() {
		return compression;
	}

	/**
	 * Close the socket.
	 *
	 * @public
	 */
	void socket::close() {
		if (tcp_sock != NULL) {
			tcp_sock->close();
			delete tcp_sock;
			tcp_sock = NULL;
		}
		if (ssl_sock != NULL) {
			ssl_sock->lowest_layer().close();
			delete ssl_sock;
			ssl_sock = NULL;
		}
	}

	/**
	 * Are we already connected to usenet?
	 *
	 * @public
	 *
	 * @return bool = Are we?
	 */
	 bool socket::is_connected() {
		if ((ssl_sock != NULL && ssl_sock->lowest_layer().is_open())
			|| (tcp_sock != NULL && tcp_sock->is_open()))
			return true;
		else
			return false;
	}

	/**
	 * Connects to usenet without SSL.
	 *
	 * @public
	 *
	 * @param  hostname = The NNTP server address.
	 * @param      port = The NNTP port.
	 * @return     bool = Did we connect?
	 */
	bool socket::connect(const std::string &hostname, const std::string &port) {
		// Check if we are already connected.
		if (is_connected())
			return true;

		if (port == "443" || port == "563") {
			throw NNTPSockException("Using a SSL port on the non SSL connect function.");
			return false;
		}

		// Store boost error codes.
		boost::system::error_code err;
		// Create an endpoint to connnect a socket to and another to compare.
		boost::asio::ip::tcp::resolver::results_type endpoints;

		// Resolver to resolve the query.
		boost::asio::ip::tcp::resolver resolver(io_service);

		try {
			endpoints = resolver.resolve(hostname, port, err);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}

		if (err)
			return false;

		tcp_sock = new unsecure(io_service);
		// loop through the available endpoints until we can connect without an error
		auto endpoint_iterator = endpoints.begin();
		while (endpoint_iterator != endpoints.end()) {
			// try to connect to the endpoint and do a handshake
			if (!tcp_sock->connect(*endpoint_iterator, err)) {
				// Verify the NNTP response and return.
				try {
					bool done = false;
					std::string resp = "";
					do {
						boost::array<char, 1024> buffer;
						size_t bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));

						if (echocli)
							std::cout.write(buffer.data(), bytesRead);

						if (resp == "")
							resp = resp + buffer[0] + buffer[1] + buffer[2];

						if (std::stoi(resp) == RESPONSECODE_READY_POSTING_ALLOWED)
							done = true;
						else if (std::stoi(resp) == RESPONSECODE_READY_POSTING_PROHIBITED)
							done = true;
						else {
							throw NNTPSockException("Wrong response code while connecting to usenet.");
							return false;
						}
					} while (!done);
				} catch (boost::system::system_error& error) {
					throw NNTPSockException(error.what());
					return false;
				}
				return true;
			}
			// Close the socket.
			tcp_sock->close();

			// Increment the iterator.
			++endpoint_iterator;
		}

		// Unable to connect.
		delete tcp_sock;
		tcp_sock = NULL;
		return false;
	}

	/**
	 * Connects to usenet using SSL.
	 *
	 * @public
	 *
	 * @param  hostname = The NNTP server address.
	 * @param      port = The NNTP port.
	 * @return     bool = Did we connect?
	 */
	bool socket::sslconnect(const std::string &hostname, const std::string &port) {
		// Check if we are already connected.
		if (is_connected())
			return true;

		if (port == "119") {
			throw NNTPSockException("Using a non-SSL port on the SSL connect function.");
			return false;
		}

		// Store boost error codes.
		boost::system::error_code err;
		// Create an endpoint to connnect a socket to and another to compare.
		boost::asio::ip::tcp::resolver::results_type endpoints;
		// SSL context.
		boost::asio::ssl::context context(ssl_context::sslv23);

		// Resolver to resolve the query.
		boost::asio::ip::tcp::resolver resolver(io_service);

		// Try to resolve the query.
		try {
			endpoints = resolver.resolve(hostname, port);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}

		if (err)
			return false;

		ssl_sock = new secure(io_service, context);
		// loop through the available endpoints until we can connect without an error
		auto endpoint_iterator = endpoints.begin();
		while (endpoint_iterator != endpoints.end()) {
			// try to connect to the endpoint and do a handshake
			if (!ssl_sock->lowest_layer().connect(*endpoint_iterator, err)
				&& !ssl_sock->handshake(boost::asio::ssl::stream_base::client, err)) {

				// Verify the NNTP response and return.
				try {
					bool done = false;
					std::string resp = "";
					do {
						boost::array<char, 1024> buffer;
						size_t bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

						if (echocli)
							std::cout.write(buffer.data(), bytesRead);

						if (resp == "")
							resp = resp + buffer[0] + buffer[1] + buffer[2];

						if (std::stoi(resp) == RESPONSECODE_READY_POSTING_ALLOWED)
							done = true;
						else if (std::stoi(resp) == RESPONSECODE_READY_POSTING_PROHIBITED)
							done = true;
						else {
							throw NNTPSockException("Wrong response code while connecting to usenet.");
							return false;
						}
					} while (!done);
				} catch (boost::system::system_error& error) {
					throw NNTPSockException(error.what());
					return false;
				}
				return true;
			}
			// Close the socket.
			ssl_sock->lowest_layer().close();

			// Increment the iterator.
			++endpoint_iterator;
		}

		// Unable to connect.
		delete ssl_sock;
		ssl_sock = NULL;
		return false;
	}

	/**
	 * Pass a command to usenet.
	 *
	 * @private
	 *
	 * @param  command = The command to pass to usenet.
	 * @return    bool = Did we succeed?
	 */
	bool socket::send_command(const std::string command) {
		if (!is_connected()) {
			return false;
		}

		try {
			// Add return + newline to the command.
			std::string cmd = command + "\r\n";

			// Send the command to usenet.
			if (tcp_sock != NULL)
				tcp_sock->write_some(boost::asio::buffer(cmd.c_str(), strlen(cmd.c_str())));
			else
				ssl_sock->write_some(boost::asio::buffer(cmd.c_str(), strlen(cmd.c_str())));
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return true;
	}

	/**
	 * Read a single line response from usenet, return the response
	 * code.
	 *
	 * @note   This is for commands where usenet returns 1 line.
	 * @private
	 *
	 * @return The response code.
	 */
	unsigned short socket::read_reponse() {
		unsigned short code = 0;
		try {
			bool done = false;
			// Create a string to store the response.
			std::string resp = "";
			// Int to store code.
			do {
				// Create an array, max 1024 chars to store the buffer.
				boost::array<char, 1024> buffer;
				// Store the buffer into the array, get the size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Prints the buffer sent from usenet.
				if (echocli)
					std::cout.write(buffer.data(), bytesRead);

				// Get the 3 first chars of the array, the response.
				if (resp == "")
					resp = resp + buffer[0] + buffer[1] + buffer[2];

				// Check if we got the response.
				if (std::stoi(resp) > 99)
					done = true;
				else {
					throw NNTPSockException("Wrong response code from usenet.");
					return code;
				}
			} while (!done);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return code;
		}
		return code;
	}

	/**
	 * Read the line sent back from usenet, check if the code
	 * sent back is good.
	 *
	 * @note   This is for commands where usenet returns 1 line.
	 * @private
	 *
	 * @param  response = The expected response from the NNTP server
	 *                    for the passed command.
	 * @return     bool = Did we succeed?
	 */
	bool socket::read_line(const responsecodes &response) {
		try {
			bool done = false;
			// Create a string to store the response.
			std::string resp = "";
			do {
				// Create an array, max 1024 chars to store the buffer.
				boost::array<char, 1024> buffer;
				// Store the buffer into the array, get the size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Prints the buffer sent from usenet.
				if (echocli)
					std::cout.write(buffer.data(), bytesRead);

				// Get the 3 first chars of the array, the response.
				if (resp == "")
					resp = resp + buffer[0] + buffer[1] + buffer[2];

				// Check if the response is good (convert resp to int).
				if (std::stoi(resp) == response)
					done = true;
				else
					return false;
			} while (!done);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return true;
	}

	/**
	 * Read the line sent back from usenet, check if the code
	 * sent back is good.
	 *
	 * @note   This is for commands where usenet returns 1 line and
	 * if you require the buffer.
	 * @private
	 *
	 * @param  response = The expected response from the NNTP server
	 *                    for the passed command.
	 * @param  finalbuffer = Pass a string reference to store the buffer.
	 * @return     bool = Did we succeed?
	 */
	bool socket::read_line(const responsecodes &response, std::string &finalbuffer) {
		try {
			bool done = false;
			// Create a string to store the response.
			std::string resp = "";
			do {
				// Create an array, max 1024 chars to store the buffer.
				boost::array<char, 1024> buffer;
				// Store the buffer into the array, get the size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Prints the buffer sent from usenet.
				if (echocli)
					std::cout.write(buffer.data(), bytesRead);

				// Append the current buffer to the final buffer.
				unsigned short iter = 0;
				while (iter < bytesRead)
					finalbuffer += buffer[iter++];

				// Get the 3 first chars of the array, the response.
				if (resp == "")
					resp = resp + buffer[0] + buffer[1] + buffer[2];

				// Check if the response is good (convert resp to int).
				if (std::stoi(resp) == response)
					done = true;
				else
					return false;
			} while (!done);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return true;
	}

	/**
	 * Read lines sent back from usenet until we find a period on
	 * the 3rd to last char of the buffer. Then verify the expected
	 * response code.
	 *
	 * @note This is for multi line reponses that end with (.\r\n),
	 * this only prints the buffer to screen, if you need the
	 * buffer, see the overloaded function.
	 * @private
	 *
	 * @param  response = The expected response from the NNTP server
	 *                    for the passed command.
	 * @return     bool = Did we succeed?
	 */
	bool socket::read_lines(const responsecodes &response) {
		// Read until we find the period.
		try {
			bool done = false;
			std::string resp = "";

			do {
				// Create an array, max 1024 chars to store the buffer.
				boost::array<char, 1024> buffer;

				// Store the buffer into the array, get the array size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Prints the buffer sent from usenet without .CRLF
				if (echocli)
					std::cout.write(buffer.data(), bytesRead-3);

				// Get the 3 first chars of the first buffer, the response.
				if (resp == "") {
					resp = resp + buffer[0] + buffer[1] + buffer[2];
					if (std::stoi(resp) != response)
						return false;
				}

				// Look for the terminator (.\r\n)
				if (buffer[bytesRead-3] == '.'
					&& buffer[bytesRead-2] == '\r'
					&& buffer[bytesRead-1] == '\n')
					done = true;
			} while (!done);

		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return true;
	}

	/**
	 * Read lines sent back from usenet until we find a period on
	 * the 3rd to last char of the buffer. Then verify the expected
	 * response code.
	 *
	 * @note This is for multi line reponses that end with (.\r\n) and
	 * the calling function requires the buffer.
	 * @private
	 *
	 * @param     response = The expected response from the NNTP server
	 *                       for the passed command.
	 * @param  finalbuffer = Pass a string reference to store the buffer.
	 * @param     compress = Will the buffer be gzip compressed
	 *                       (usually over/xover commands).
	 * @return        bool = Did we succeed?
	 */
	bool socket::read_lines(const responsecodes &response,
		std::string &finalbuffer, const bool &compress) {
		// Check if gzip compression is on and if the buffer should be compressed.
		if (compress && compression)
			return read_compressed_lines(response, finalbuffer);

		// Read until we find the period.
		try {
			bool done = false;
			std::string resp = "";
			do {
				// Create an array, max 1024 chars to store the buffer.
				boost::array<char, 1024> buffer;

				// Store the buffer into the array, get the array size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Append the current buffer to the final buffer.
				unsigned short iter = 0;
				while (iter < bytesRead)
					finalbuffer += buffer[iter++];

				// Get the 3 first chars of the first buffer, the response.
				if (resp == "") {
					resp = resp + buffer[0] + buffer[1] + buffer[2];
					if (std::stoi(resp) != response)
						return false;
				}

				// Look for the terminator (.\r\n)
				if (buffer[bytesRead-3] == '.'
					&& buffer[bytesRead-2] == '\r'
					&& buffer[bytesRead-1] == '\n')
					done = true;
			} while (!done);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return true;
	}

	/**
	 * Read lines sent back from usenet used when using gzip compress.
	 *
	 * @note For multi line commands that can be compressed (XOVER).
	 * @private
	 *
	 * @param  response = The expected response from the NNTP server
	 *                    for the passed command.
	 * @return     bool = Did we succeed?
	 */
	bool socket::read_compressed_lines(const responsecodes &response,
				std::string &finalbuffer) {
		// Read until we find the period.
		try {
			bool done = false;
			std::string resp = "";
			do {
				// Boost doesn't seem to like gzip data, so create a normal array.
				char buffer[1024];
				// Store the buffer into the array, get the size.
				size_t bytesRead;
				if (tcp_sock != NULL)
					bytesRead = tcp_sock->read_some(boost::asio::buffer(buffer));
				else
					bytesRead = ssl_sock->read_some(boost::asio::buffer(buffer));

				// Append the current buffer to the final buffer.
				unsigned short iter = 0;
				while (iter < bytesRead)
					finalbuffer += buffer[iter++];

				// Get the 3 first chars of the array, the response.
				if (resp == "") {
					resp = resp + buffer[0] + buffer[1] + buffer[2];
					if (std::stoi(resp) != response)
						return false;
				}

				// Look for the terminator (.\r\n)
				if (buffer[bytesRead-3] == '.'
					&& buffer[bytesRead-2] == '\r'
					&& buffer[bytesRead-1] == '\n')
					done = true;
			} while (!done);
		} catch (boost::system::system_error& error) {
			throw NNTPSockException(error.what());
			return false;
		}
		return parsecompressedbuffer(finalbuffer);
	}


	/**
	 * Decompress gzip buffer.
	 * 
	 * @public
	 */
	bool socket::parsecompressedbuffer(std::string &finalbuffer) {
		bool respfound = false;
		std::string newbuffer, respline = "";
		// Remove the response and .CRLF from the end.
		for (unsigned long i = 0; i < (finalbuffer.length()); i++) {
			// Skip response, it's the first line.
			if (!respfound) {
				if (finalbuffer[i] != '\n')
					respline += finalbuffer[i];
				else
					respfound = true;
			}
			else {
				// Remove .CRLF
				if (finalbuffer[i] == '.'
					&& finalbuffer[i+1] == '\r'
					&& finalbuffer[i+2] == '\n')
					break;
				// Tack on the rest.
				else
					newbuffer += finalbuffer[i];
			}
		}

		// Check if the first char is a x.
		if (newbuffer[0] != 120)
			return false;

		// Try to decompress the data, catch zlib errors.
		try {

			// Create a string stream to store the gzip data.
			std::stringstream ss(newbuffer);
			// Create a string stream to store the decompressed data.
			std::stringstream copyback;

			// Create a decompress object.
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			// Tell it to use zlib.
			in.push(boost::iostreams::zlib_decompressor());
			// Decompress the data.
			in.push(ss);
			// Copy the data.
			boost::iostreams::copy(in, copyback);

			// The response + the decompressed data + .CRLF
			finalbuffer = respline + '\n' + copyback.str() + '.' + '\r' + '\n';

			return true;
		 } catch (boost::iostreams::zlib_error& e) {
			throw NNTPSockException(e.what());
			return false;
		}
	}
}
