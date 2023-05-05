#include <stdio.h>
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h>
#include <winsock.h>
#endif // _WIN32

/**
 *	Encodes a FreeD rotation value (24-bit float)
 *	@param value - The value to encode.
 *	@param output - The output packet to write to.
 */
void encodeFreeDRotation(float rotation, unsigned char** output)
{
	unsigned long long rot = (unsigned long long)(long long)(rotation * 32768);
	// FIXME: include a check for big endian vs little endian
	rot = _byteswap_uint64(rot);
	unsigned char* rotP = (unsigned char*)&rot;
	memcpy(*output, rotP + 5, 3);
	*output += 3;
}

/**
 *	Encodes a FreeD position value (24-bit float)
 *	@param value - The value to encode.
 *	@param output - The output packet to write to.
 */
void encodeFreeDPosition(float position, unsigned char** output)
{
	unsigned long long pos = (unsigned long long)(long long)(position * 64);
	// FIXME: include a check for big endian vs little endian
	pos = _byteswap_uint64(pos);
	unsigned char* posP = (unsigned char*)&pos;
	memcpy(*output, posP + 5, 3);
	*output += 3;
}

/**
 *	Encodes a FreeD integer (24-bit).
 *	@param value - The value to encode.
 *	@param output - The output packet to write to.
 */
void encodeFreeDInteger(int value, unsigned char** output)
{
	// FIXME: include a check for big endian vs little endian
	unsigned long long val = _byteswap_uint64((unsigned long long)value);
	unsigned char* posP = (unsigned char*)&val;
	memcpy(*output, posP + 5, 3);
	*output += 3;
}

/**
 *	Creates a FreeD checksum.
 *	@param packet - The packet to checksum.
 *	@param length - The length of the packet to checksum.
 *	@returns the checksum value
 */
unsigned char freeDChecksum(unsigned char* packet, size_t length)
{
	int checksum = 64;
	for (size_t i = 0; i < length; i++)
	{
		signed char packetSigned = (signed char)packet[i];
		checksum -= packetSigned;
	}
	checksum %= 256;
	if (checksum < 0)
	{
		checksum += 256;
	}
	return (unsigned char)(checksum & 0xFF);
}

/**
 *	The main entrypoint of the application.
 *	@param argc - the number of commandline arguments supplied to the application
 *	@param argv - the value of each commandline argument supplied to the application
 *	@returns an error code indicating success (0) or failure (non-zero)
 */
int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("Please supply at least two commandline arguments to the application.\n");
		printf("Usage:\n");
		printf("\t%s <ip address> <port>\n", argv[0]);
		return 1;
	}

	const char* ip = argv[1];
	const char* port = argv[2];

	// Start up WinSock (Windows only)
#ifdef _WIN32
	WSAData wsaData;
	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaError != 0)
	{
		printf("Failed to start Winsock: %d\n", wsaError);
		return 2;
	}
#endif // _WIN32

	// Open the socket.
	SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
	{
		printf("Failed to open socket: %lld\n", sockfd);
#ifdef _WIN32
		WSACleanup();
#endif // _WIN32
		return 3;
	}

	// Resolve the IP address.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	inet_pton(AF_INET, ip, &addr.sin_addr);

	// Tell the user that they need to provide input to close the application.
	int stdin_fd = _fileno(stdin);
	printf("Press ENTER to exit.\n");


	// Run until the user has decided to stop.
	while (true)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(stdin_fd, &read_fds);

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;

		int result = select(1, &read_fds, NULL, NULL, &timeout);
		if (result > 0 && FD_ISSET(stdin_fd, &read_fds))
		{
			// User input has been provided, break out.
			break;
		}

		// Start constructing the FreeD packet.
		const size_t freeDPacketSize = 29;
		unsigned char freeDPacket[freeDPacketSize] = { 0xD1 };
		unsigned char* packetPtr = freeDPacket + 1;
		*packetPtr = 0xFF; ++packetPtr; // camera ID

		encodeFreeDRotation(90.0f, &packetPtr); // pitch
		encodeFreeDRotation(0.0f, &packetPtr); // yaw
		encodeFreeDRotation(0.0f, &packetPtr); // roll
		encodeFreeDPosition(100.0f, &packetPtr); // x
		encodeFreeDPosition(200.0f, &packetPtr); // y
		encodeFreeDPosition(300.0f, &packetPtr); // z
		encodeFreeDInteger(0, &packetPtr); // zoom
		encodeFreeDInteger(0, &packetPtr); // focus
		*packetPtr = 0x00; ++packetPtr; // reserved
		*packetPtr = 0x00; ++packetPtr; // reserved
		*packetPtr = freeDChecksum(freeDPacket, freeDPacketSize); // checksum

		// Send the packet!
		sendto(sockfd, (const char*)freeDPacket, freeDPacketSize, 0, (sockaddr*)&addr, sizeof(addr));
	}

	// Close the socket.
	closesocket(sockfd);

	// Shut down WinSock (Windows only)
#ifdef _WIN32
	WSACleanup();
#endif // _WIN32
}