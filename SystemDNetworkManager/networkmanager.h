#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#include <iostream>
#include <fstream>
#include <systemd/sd-bus.h>


using namespace std;
class NetworkManager
{
public:
    NetworkManager(const char *ifName);
    ~NetworkManager();
    int changeAddress(const char *ip);
    int changeNameServer(const char *dns);
    int changeNetMask(const char *msk);
    int changeGateway(const char *gw);
    int changeDhcp(const char *dhcp);
    int configWriteReload();
    int showCurrentConfig();
private:
    string ipAddr;
    string nameServer;
    string gwAddr;
    string netMask;
    string dhcpMode;
    string netIf;
    string cfgFilePath;
    int devId;
    string devPath;
    int networkReload();
};

#endif // NETWORKMANAGER_H
