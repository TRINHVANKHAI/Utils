#include "include/networkmanager.h"
#include "include/log.h"
#include "json/value.h"
#include "json/reader.h"

NetworkManager::NetworkManager(const char *ifName)
{
    sd_bus_error sdBusError = SD_BUS_ERROR_NULL;
    sd_bus_message *message = nullptr;
    sd_bus *sdBus = nullptr;
    const char *resp;
    int idx;
    string tmpmsg;
    Json::Value jResponse;
    Json::Reader reader;
    int errorCode = 0;
    netIf = ifName;
    errorCode = sd_bus_open_system(&sdBus);
    if (errorCode < 0) {
        log_error("sd_bus_default_system: %d \n", errorCode);
    }

    errorCode = sd_bus_call_method (
        sdBus,
        "org.freedesktop.network1",
        "/org/freedesktop/network1",
        "org.freedesktop.network1.Manager",
        "GetLinkByName",
        &sdBusError,
        &message,
        "s",
        netIf.c_str());

    if (errorCode < 0) {
        log_error("sd_bus_call_method: %s \n", sdBusError.message);
    }

    errorCode = sd_bus_message_read(message, "io", &idx, &resp);
    if (errorCode < 0) {
        log_error("sd_bus_message_read: %s \n", sdBusError.message);
    }

    devId = idx;
    devPath = resp;
    errorCode = sd_bus_call_method (
        sdBus,
        "org.freedesktop.network1",
        devPath.c_str(),
        "org.freedesktop.network1.Link",
        "Describe",
        &sdBusError,
        &message,
        "");
    if (errorCode < 0) {
        log_error("sd_bus_call_method: %s \n", sdBusError.message);
    }

    errorCode = sd_bus_message_read(message, "s", &resp);
    if (errorCode < 0) {
        log_error("sd_bus_message_read: %d \n", errorCode);
    }

    tmpmsg = resp;
    reader.parse(tmpmsg.c_str(), jResponse, true);
    cfgFilePath = jResponse["NetworkFile"].toStyledString();

    sd_bus_error_free(&sdBusError);
    sd_bus_message_unref(message);
    sd_bus_unref(sdBus);
}

NetworkManager::~NetworkManager()
{

}

int NetworkManager::changeAddress(const char *ip)
{
    ipAddr = ip;
    return 0;
}

int NetworkManager::changeNameServer(const char *dns)
{
    nameServer = dns;
    return 0;
}

int NetworkManager::changeGateway(const char *gw)
{
    gwAddr = gw;
    return 0;
}

int NetworkManager::changeDhcp(const char *dhcp)
{
    dhcpMode = dhcp;
    return 0;
}

int NetworkManager::changeNetMask(const char *msk)
{
    netMask = msk;
    return 0;
}

int NetworkManager::showCurrentConfig()
{
    sd_bus_error sdBusError = SD_BUS_ERROR_NULL;
    sd_bus_message *message = nullptr;
    sd_bus *sdBus = nullptr;
    const char *resp;

    string tmpmsg;

    Json::ArrayIndex i;
    Json::Value jResponse;
    Json::Value jAddrs;
    Json::Value jConf;
    Json::Reader reader;
    int errorCode = 0;

    errorCode = sd_bus_open_system(&sdBus);
    if (errorCode < 0) {
        log_error("sd_bus_default_system: %d \n", errorCode);
    }

    errorCode = sd_bus_call_method (
        sdBus,
        "org.freedesktop.network1",
        devPath.c_str(),
        "org.freedesktop.network1.Link",
        "Describe",
        &sdBusError,
        &message,
        "");

    if (errorCode < 0) {
        log_error("sd_bus_call_method: %s \n", sdBusError.message);
    }

    errorCode = sd_bus_message_read(message, "s", &resp);
    if (errorCode < 0) {
        log_error("sd_bus_message_read: %d \n", errorCode);
    }

    tmpmsg = resp;
    reader.parse(tmpmsg.c_str(), jResponse, true);
    jAddrs = jResponse["Addresses"];

    for(i=0; i<jAddrs.size(); i++) {
        jConf = jAddrs[i];

        string cfgSrc  = jConf["ConfigSource"].toStyledString();
        Json::Value curAddr = jConf["Address"];
        int curMask = jConf["PrefixLength"].asInt();

        if ((cfgSrc.substr(1,6).compare("static")==0) &&
            (curMask == 24)) {
            log_info("static");
            log_info("Address: %d.%d.%d.%d/%d\n",
                     curAddr[0].asInt(),
                     curAddr[1].asInt(),
                     curAddr[2].asInt(),
                     curAddr[3].asInt(),
                     curMask);

        } else if ((cfgSrc.substr(1,6).compare("DHCPv4")==0) &&
                   (curMask == 24)) {
            log_info("DHCPv4");
            log_info("Address: %d.%d.%d.%d/%d\n",
                     curAddr[0].asInt(),
                     curAddr[1].asInt(),
                     curAddr[2].asInt(),
                     curAddr[3].asInt(),
                     curMask);
        }

    }

    sd_bus_error_free(&sdBusError);
    sd_bus_message_unref(message);
    sd_bus_unref(sdBus);
    return 0;
}

int NetworkManager::networkReload()
{
    sd_bus_error sdBusError = SD_BUS_ERROR_NULL;
    sd_bus_message *message = nullptr;
    sd_bus *sdBus = nullptr;
    char *resp;
    int i;
    int status = 0;
    int errorCode = 0;

    errorCode = sd_bus_default_system(&sdBus);
    if (errorCode < 0) {
        log_error("sd_bus_default_system: %d \n", errorCode);
        return -1;
    }

    errorCode = sd_bus_match_signal (
        sdBus,
        nullptr,
        "org.freedesktop.network1",
        devPath.c_str(),
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        nullptr,
        &status);
    if (errorCode < 0) {
        log_error("sd_bus_match_signal: %d \n", errorCode);
        return -1;
    }

    errorCode = sd_bus_call_method (
        sdBus,
        "org.freedesktop.network1",
        "/org/freedesktop/network1",
        "org.freedesktop.network1.Manager",
        "Reload",
        &sdBusError,
        &message,
        "");
    if (errorCode < 0) {
        log_error("sd_bus_call_method: %d \n", errorCode);
        return -1;
    }
    for(i=0;i<100;i++) {
        errorCode = sd_bus_wait(sdBus, 100000);
        if(errorCode < 0) {
            log_error("sd_bus_wait: %d \n", errorCode);
        }
        errorCode = sd_bus_process(sdBus, &message);
        if(errorCode < 0) {
            log_error("sd_bus_process: %d \n", errorCode);
        }
        errorCode = sd_bus_get_property_string (
            sdBus,
            "org.freedesktop.network1",
            devPath.c_str(),
            "org.freedesktop.network1.Link",
            "AdministrativeState",
            &sdBusError,
            &resp);
        if(errorCode < 0) {
            log_error("sd_bus_get_property_string: %d \n", errorCode);
        }
        if(strcmp(resp, "configured")==0) break;
        free(resp);
    }

    sd_bus_error_free(&sdBusError);
    sd_bus_message_unref(message);
    sd_bus_unref(sdBus);

    return 0;
}

int NetworkManager::configWriteReload()
{

    ofstream cfgFile;
    string cfgFileName = "10-" + netIf + ".network";
    cfgFilePath = "/etc/systemd/network/" + cfgFileName;

    cfgFile.open(cfgFilePath.c_str());
    if(!cfgFile.is_open()) {
        log_error("Can't open the file %s \n", cfgFilePath.c_str());
        return -1;
    }
    if(dhcpMode.compare("static")==0) {
        cfgFile << "[Match]" << endl;
        cfgFile << "Name=" << netIf << endl;
        cfgFile << "KernelCommandLine=!nfsroot" << endl;
        cfgFile << "" << endl;
        cfgFile << "[Network]" << endl;
        cfgFile << "Address=" << ipAddr << "/" << netMask << endl;
        cfgFile << "Gateway=" << gwAddr << endl;
        cfgFile << "" << endl;
    } else if (dhcpMode.compare("dhcp")==0) {
        cfgFile << "[Match]" << endl;
        cfgFile << "Name=" << netIf << endl;
        cfgFile << "KernelCommandLine=!nfsroot" << endl;
        cfgFile << "" << endl;
        cfgFile << "[Network]" << endl;
        cfgFile << "DHCP=yes" << endl;
        cfgFile << "" << endl;
    }
    cfgFile.flush();
    cfgFile.close();
    networkReload();
    showCurrentConfig();
    return 0;
}

