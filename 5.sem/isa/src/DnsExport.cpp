#include <unistd.h>     // Getopt
#include <list>         // List, iterator
#include <iostream>     // No comment needed...
#include <unistd.h>     // Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>       // No comment needed...
#include <string.h>     // bzero, bcopy
#include <netdb.h>      // gethostbyname...
#include <thread>       // Sending loop
#include <arpa/inet.h>  // inet_pton
#include <algorithm>    // check IPv6

#include "DnsPacket.cpp"
#include "DnsRecord.cpp"

using namespace std;

/*************************************************** GLOBALS ******************************************************/

// To identify linux ssl datalink
uint8_t datalink;

// List collecting all stats to print/send
list<DnsRecord> recordList;

// pcap_t structure better here for easy signal handling such as recordList
pcap_t* myPcap;

// Struct holding comand line parameters
typedef struct
{
    string file;
    string interface;
    string time;
    string syslogServer;
} rawParameters;
rawParameters parameters;

/*************************************************** CONSTS ******************************************************/

const string usage = "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n"
                     "file.pcap - file to sniff\n"
                     "interface - interface to sniff (or ANY for all interfaces)\n"
                     "syslog-server - Syslog server where the stats are sent\n"
                     "seconds - time period of sending stats to Syslog server\n";

const string help = "Invalid parameters!\n" + usage;
                    

#define SYSLOG_PORT 514
#define MESSAGE_SIZE 400

// to_ms parameter for pcap_open_live, same value is used by wireshark
#define TO_MS 100

// Main class
// Parse arguments and run sniffer
class DnsExport
{
    /*********************************************** PRIVATE Variables ********************************************/

    // For pcap if error occures
    char errBuf[PCAP_ERRBUF_SIZE];

    /*********************************************** PRIVATE Methods ********************************************/

    // Parse arguments
    void parseArg(int argc, char const *argv[])
    {
        int option;

        while ((option = getopt(argc, (char **)argv, "r:i:s:t:h")) != -1)
        {
            switch (option)
            {
                case 'r':
                    if (parameters.file.compare(""))
                    {
                        ERR_RET(help);
                    }
                   parameters.file = string(optarg);
                    break;
                case 'i':
                    if (parameters.interface.compare(""))
                    {
                        ERR_RET(help);
                    }
                    parameters.interface = string(optarg);
                    break;
                case 's':
                    if (parameters.syslogServer.compare(""))
                    {
                        ERR_RET(help);
                    }
                    parameters.syslogServer = string(optarg);
                    break;
                case 't':
                    if (parameters.time.compare(""))
                    {
                        ERR_RET(help);
                    }
                    parameters.time = string(optarg);
                    break;
                case 'h':
                    cout << usage;
                    exit(EXIT_SUCCESS);
                default:
                    ERR_RET(help);
            }
        }
    }

    // Signal handler for SIGUSR1 writing stats to stdout
    static void handleSig(int sigNum)
    {
        if (sigNum == SIGUSR1)
        {
            // Working just with interface sniffing
            if (!parameters.file.compare(""))
            {
                writeStats();
            }
        }
        else if (sigNum == SIGINT)
        {
            pcap_close(myPcap);
            exit(0);
        }
    }

    // Write stats to stdout
    static void writeStats()
    {
        list<DnsRecord>::iterator it; 

        // Create copy (safer multithreading)
        list<DnsRecord> listToWrite = recordList;

        for(it = listToWrite.begin(); it != listToWrite.end(); it++) 
        {
            cout << it->GetString() << endl;
        }
    }

    // Interface sniffer
    void sniffInterface()
    {
        // Open device for sniffing TO_MS = 100 ms is value used by wireshark
        if ((myPcap = pcap_open_live(parameters.interface.c_str(), BUFSIZ, true, TO_MS, this->errBuf)) == NULL)
        {
            ERR_RET("Unable to open interface for listening. Error: " << errBuf);
        }

        datalink = pcap_datalink(myPcap);

        int time;
        if (!(parameters.time.compare("")))
        {
            time = 60; // Default value
        }
        else
        {
            // Check for valid time
            if (parameters.time.find_first_not_of("0123456789") == string::npos)
            {
                time = stoi(parameters.time, NULL, 10);
            }
            else
            {
                ERR_RET("Time must be number in seconds!");
            }
        }

        if (parameters.syslogServer.compare(""))
        {
            // Start sending loop
            pthread_t thread;
            pthread_create(&thread, NULL, sendingLoop, (void*)&time);
        }

        // Start sniffing forever...
        if (pcap_loop(myPcap, -1, pcapHandler, NULL) < 0)
        {
            pcap_close(myPcap);
            ERR_RET("Error occured when sniffing.");
        }
    }

    // File sniffer
    void sniffFile()
    {
        // Open file for sniffing
        if ((myPcap = pcap_open_offline(parameters.file.c_str(), this->errBuf)) == NULL)
        {
            ERR_RET("Unable to open pcap file. Error:" << errBuf);
        }

        datalink = pcap_datalink(myPcap);

        // Sniff file
        if (pcap_loop(myPcap, -1, this->pcapHandler, NULL) < 0)
        {
            pcap_close(myPcap);
            ERR_RET("Error occured when reading pcap file.");
        }

        pcap_close(myPcap);

        // Send stats if syslog server defined, write to stdout otherwise
        if (!parameters.syslogServer.compare(""))
        {
            this->writeStats();
        }
        else
        {
            this->sendStats();
        }
    }

    // This callback is called in pcap_loop for each packet
    // Parse packet and add stats if valid and supported DNS packet
    static void pcapHandler(unsigned char* useless, const struct pcap_pkthdr* header, const uint8_t* packet)
    {
        // Create object and try to parse
        DnsPacket dnsPacket(packet, header, datalink);
        dnsPacket.Parse();

        // Add answers to record list if exist
        if (dnsPacket.Answers.size() > 0)
        {
            addRecords(dnsPacket.Answers);
        }
    }

    // Add dns answers to record list
    static void addRecords(list<DnsRR> answerList)
    {
        list<DnsRR>::iterator it; 
        for (it = answerList.begin(); it != answerList.end(); it++) 
        {
            list<DnsRecord>::iterator it2; 

            bool added = false;

            // Try to find record in list
            for (it2 = recordList.begin(); it2 != recordList.end(); it2++) 
            {
                if (it2->IsEqual(it->domainName, it->type, it->data))
                {
                    it2->AddRecord();
                    added = true;
                    break;
                }
            }

            // Create new record
            if (!added)
            {
                DnsRecord dnsRR(it->domainName, it->type, it->typeStr, it->data);
                recordList.push_back(dnsRR);
            }
        }
    }

    // Thread sending stats to syslog server
    static void* sendingLoop(void* time)
    {
        // Initial sleep - waiting for pcap to start and parse something
        chrono::milliseconds openLiveTO(20); // By given parameter
        this_thread::sleep_for(openLiveTO);

        // http://www.cplusplus.com/forum/beginner/91449/
        chrono::seconds interval(*((int*)time)); // By given parameter
        while (true)
        {
            this_thread::sleep_for(interval);
            sendStats();
        }
    }

    // Send stats to syslog server (over UDP)
    static void sendStats()
    {
        int sock;
        struct sockaddr_in serverAddress;
        struct sockaddr_in6 serverIPv6Address;
        bool ipv6 = false;

        // Check if IPv6 address
        if (count(parameters.syslogServer.begin(), parameters.syslogServer.end(), ':') > 1)
        {
            ipv6 = true;

            // Create socket
            if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) <= 0)
            {
                ERR_RET("Syslog - IPv6 socket creating error! Code: " << errno);
            }

            serverIPv6Address.sin6_family = AF_INET6;
            serverIPv6Address.sin6_port = htons(SYSLOG_PORT);
            inet_pton(AF_INET6, parameters.syslogServer.c_str(),  &serverIPv6Address.sin6_addr);
        }
        // IPv4 or hostname
        else
        {
            // Create socket
            if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
            {
                ERR_RET("Syslog -  IPv4 socket creating error! Code: " << errno);
            }

            // Get network host entry
            struct hostent* server;
            if ((server = gethostbyname(parameters.syslogServer.c_str())) == NULL)
            {
                ERR_RET("Syslog - host not found!");
            }

            // Initialize necessary structures
            bzero((char*) &serverAddress, sizeof(serverAddress));
            serverAddress.sin_family = AF_INET;
            bcopy((char*)server->h_addr, (char*)&serverAddress.sin_addr.s_addr, server->h_length);
            serverAddress.sin_port = htons(SYSLOG_PORT);        // Port is string, need conversion
        }

        // Get hostname
        string hostname;
        char tmp [1024] = "";
        if (!gethostname(tmp, sizeof tmp))
        {
            hostname = tmp;
        }

        // Get and send all mesagges
        list<string>messages = getMessages(hostname);
        list<string>::iterator it;

        for (it = messages.begin(); it != messages.end(); it++)
        {
            if (ipv6)
            {
                if (sendto (sock, it->c_str(), it->length() + 1, 0, (struct sockaddr*)&serverIPv6Address, sizeof(serverIPv6Address)) < 0)
                {
                    ERR_RET("Syslog - IPv6 sendto error! Code: " << errno);
                }
            }
            else
            {
                // Send packet
                if (sendto(sock, it->c_str(), it->length() + 1, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // + 1 for '\0' character 
                {
                    ERR_RET("Syslog -  IPv4 sendto error! Code: " << errno);
                }
            }
        }

        close(sock);           // Close socket
    }

    // Return statistics formated as syslog messages
    static list<string> getMessages(string hostname)
    {
        list<string> messages;
        string message;
        list<DnsRecord>::iterator it;
        list<DnsRecord> listTosend = recordList;    // Create copy (safer multithreading)

        // Create list of all messages
        for (it = listTosend.begin(); it != listTosend.end(); it++)
        {
            string record =  " | " + it->GetString();

            // We can add another record to one message if limit wasn't exceeded
            if ((record + message).size() > MESSAGE_SIZE)
            {
                message.erase(0, 3);        // Remove leading |
                message = "<134>1 " + getFormattedTime() + " " + hostname + " dns-export" + " - - - " + message;
                messages.push_back(message);
                message = "";
            }
            message += record;
        }

        // At the end add last message to the list if not ""
        if (message.compare(""))
        {
            message.erase(0, 3);        // Remove leading |
            message = "<134>1 " + getFormattedTime() + " " + hostname + " dns-export" + " - - - " + message;
            messages.push_back(message);
        }
        else if (messages.size() == 0)
        {
            message = "No DNS packets found";
            messages.push_back(message);
        }
        return messages;
    }

    // Returns the local date/time formatted as 2014-03-19 11:11:52
    // https://codereview.stackexchange.com/questions/11921/getting-current-time-with-milliseconds
    static string getFormattedTime()
    {
        timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;

        char buffer [80];
        strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&curTime.tv_sec));

        char currentTime[84] = "";
        sprintf(currentTime, "%s.%03dZ", buffer, milli);

        return currentTime;
    }

    /*********************************************** PUBLIC Methods ********************************************/

    public:
        // Only parsing arguments in constructor
        DnsExport(int argc, char const *argv[])
        {
            this->parseArg(argc, argv);
        }

        // Main method
        int Main()
        {
            // Handle SIUGSR1
            signal(SIGUSR1, handleSig);

            // Handle SIUGSR1
            signal(SIGINT, handleSig);

            // Error
            if (parameters.interface.compare("") && parameters.file.compare(""))
            {
                ERR_RET(help);
            }

            // Choose what to do
            if (parameters.interface.compare(""))
            {
                this->sniffInterface();
            }

            if (parameters.file.compare(""))
            {
                this->sniffFile();
            }
            
            return 0;
        }
};
