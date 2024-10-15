Overview

The implementation consists of methods to establish and manage a TCP server, which can communicate with multiple TCP clients. The server supports socket creation, binding, listening, accepting client connections, sending data, and closing connections, ensuring efficient client-server communication over IPv4.

Key Features:

    Socket Creation and Configuration: The server creates a TCP socket, configures its options to allow address reuse, and sets it to non-blocking mode.
    Binding and Listening: It binds the socket to a specific port and listens for incoming client connections.
    Accepting Client Connections: The server accepts connections from multiple clients using a dedicated thread.
    Data Transmission: Supports sending data to all connected clients, handling scenarios where data transmission might not complete immediately.
    Graceful Shutdown: Properly shuts down both server and client sockets, ensuring resource cleanup.
    Client Management: Maintains a list of connected clients and provides methods to retrieve client information.

Detailed Components:

    Socket Creation: Uses the socket() function to create a TCP socket, and sets socket options using setsockopt() for reuse of local addresses.
    Non-blocking Mode: Uses fcntl() to set the socket into non-blocking mode, which helps in handling multiple connections efficiently.
    Connection Handling: The server spawns a separate thread to handle incoming connections asynchronously, making use of select() to wait for client activity.
    Data Sending: Implements a robust mechanism to send data to multiple clients, ensuring data is sent even if it requires multiple iterations.
    Error Handling: Utilizes custom exceptions and standard error messages to handle and log issues during socket operations.
    Shutdown and Cleanup: Gracefully closes all connections by shutting down client and server sockets.

Example Usage:

    Starting the Server: Instantiate the TCPServer with a specific port, which automatically begins listening for incoming connections.
    Sending Data: Use the SockSend() method to broadcast data to all connected clients with a specified timeout.
    Client Information: Retrieve the list of currently connected clients using the GetClients() method.

This implementation provides a robust foundation for creating a TCP server capable of handling multiple clients in a non-blocking, asynchronous manner.
