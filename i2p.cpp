#include <iostream>
#include <thread>
#include <cryptopp/integer.h>
#include <boost/filesystem.hpp>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "Log.h"
#include "base64.h"
#include "Transports.h"
#include "NTCPSession.h"
#include "RouterInfo.h"
#include "RouterContext.h"
#include "Tunnel.h"
#include "NetDb.h"
#include "HTTPServer.h"
#include "util.h"

void handle_sighup(int n)
{
  if (i2p::util::config::GetArg("daemon", 0) == 1)
  {
    static bool first=true;
    if (first)
    {
      first=false;
      return;
    }
  }
  LogPrint("Reloading config.");
  i2p::util::filesystem::ReadConfigFile(i2p::util::config::mapArgs, i2p::util::config::mapMultiArgs);
}

int main( int argc, char* argv[] )
{
  i2p::util::config::OptionParser(argc,argv);
#ifdef _WIN32
  setlocale(LC_CTYPE, "");
  SetConsoleCP(1251);
  SetConsoleOutputCP(1251);
  setlocale(LC_ALL, "Russian");
#endif


  LogPrint("\n\n\n\ni2pd starting\n");
  LogPrint("data directory: ", i2p::util::filesystem::GetDataDir().string());
  i2p::util::filesystem::ReadConfigFile(i2p::util::config::mapArgs, i2p::util::config::mapMultiArgs);

  struct sigaction sa;
  sa.sa_handler = handle_sighup;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGHUP,&sa,0) == -1)
  {
    LogPrint("Failed to install SIGHUP handler.");
  }

  if (i2p::util::config::GetArg("-daemon", 0) == 1)
  {
    pid_t pid;
    pid = fork();
    if (pid > 0)
    {
      g_Log.Stop();
      return 0;
    }
    if (pid < 0)
    {
      return -1;
    }

    umask(0);
    int sid = setsid();
    if (sid < 0)
    {
      LogPrint("Error, could not create process group.");
      return -1;
    }
  }

  if (i2p::util::config::GetArg("-log", 0) == 1)
  {
    std::string logfile = i2p::util::filesystem::GetDataDir().string();
    logfile.append("/debug.log");
    LogPrint("Logging to file enabled.");
    freopen(logfile.c_str(),"a",stdout);
  }

  //TODO: This is an ugly workaround. fix it.
  //TODO: Autodetect public IP.
  i2p::context.OverrideNTCPAddress(i2p::util::config::GetCharArg("-host", "127.0.0.1"),
      i2p::util::config::GetArg("-port", 17070));
  int httpport = i2p::util::config::GetArg("-httpport", 7070);

  i2p::util::HTTPServer httpServer (httpport);

  httpServer.Start ();	
  i2p::data::netdb.Start ();
  i2p::transports.Start ();	
  i2p::tunnel::tunnels.Start ();

  int running = 1;
  while (running)
  {
    std::this_thread::sleep_for (std::chrono::seconds(1000));
  }

  i2p::tunnel::tunnels.Stop ();	
  i2p::transports.Stop ();	
  i2p::data::netdb.Stop ();	
  httpServer.Stop ();
  if (i2p::util::config::GetArg("-log", 0) == 1)
  {
    fclose (stdout);
  }
  return 0;
}
