# remote-file-sharing-system
A simple application for file sharing among remote hosts (as part of an assignment)

Usage:
To run as a server listening on port 4322 
./assignment1 s 4322 
To run as a client listening on port 4322 
./assignment1 c 4322 


Each instance provides the following commands:
CREATOR                             : Display name, name and Email
                                      of the creator of this program.
MYIP                                : Display IP Address of this process.
MYPORT                              : Display PORT number on which this process
                                      is listening for incoming connections.
REGISTER <server_IP> <port_no>      : Register with the server 
                                      and obtain the Server-IP List.
CONNECT <destination> <port_no>     : Establish a new TCP connection to the
                                      specified <destination> at the specified
                                      <port_no>.
LIST                                : Display list of all connections this 
                                      process is a part of.
UPLOAD <connection_id> <filename>   : Upload <file_name> to host specified on
                                      on <connection_id>
DOWNLOAD <connection_id1><file1> ...: Download a file from each specified host
                                      in the command
TERMINATE <connection_id>           : Terminate the connection specified by
                                      <connection_id>, obtained from the LIST
                                      command.
EXIT                                : Close all connections and terminate the
                                      process.

