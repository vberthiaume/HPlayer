#include "oscCom.h"
#include <net/if.h>
#include <arpa/inet.h>
#include <ofFileUtils.h>


/**
 * Constructor to set default values.
 */
oscCom::oscCom()
{
	portIN = 9000;
	portOUT = 9001;
	hostOUT = "localhost";
	base64 = false;
	cmdmap = "default";
	prefix = "";
    connected = false;
}


/**
 * Connect to the network, load configuration and set (default) commands
 *
 * \see getIP()
 */
void oscCom::connect()
{
	oscListener.setup(portIN);
	oscSender.setup(hostOUT, portOUT);
	connected = true;

	//set default commands
	xmlSettings settings("settings.xml");
	settings.conf_str("commands:default","play", "play");
	settings.conf_str("commands:default","playloop", "playloop");
	settings.conf_str("commands:default","load", "load");
	settings.conf_str("commands:default","stop", "stop");
	settings.conf_str("commands:default","pause", "pause");
	settings.conf_str("commands:default","resume", "resume");
	settings.conf_str("commands:default","getStatus", "getStatus");
	settings.conf_str("commands:default","next", "next");
	settings.conf_str("commands:default","prev", "prev");
	settings.conf_str("commands:default","volume", "volume");
	settings.conf_str("commands:default","mute", "mute");
	settings.conf_str("commands:default","unmute", "unmute");
	settings.conf_str("commands:default","loop", "loop");
	settings.conf_str("commands:default","unloop", "unloop");
	settings.conf_str("commands:default","blur", "blur");
	settings.conf_str("commands:default","zoom", "zoom");
	settings.conf_str("commands:default","info", "info");
	settings.conf_str("commands:default","host", "host");
	settings.conf_str("commands:default","quit", "quit");

	//load config
	commander.loadFile("settings.xml");
	commander.pushTag("commands");
	commander.pushTag(cmdmap);
}


/**
 * Map command string to OSC command.
 *
 * \see oscToString(ofxOscMessage m)
 */
string oscCom::cmd(string command)
{
	return commander.getValue(command,command);
}


/**
 * Execute all (qeued) (OSC) commands received and send status info
 * for valid ones.
 *
 * \param player to execute commands for
 */
void oscCom::execute(mediaPlayer* player)
{
	if (!connected) return;
	if (oscListener.hasWaitingMessages()) oscDebug = "";
	
	//OSC GET
	while( oscListener.hasWaitingMessages() ) 
	{
        ofxOscMessage m;
        oscListener.getNextMessage( &m );       

        //RECEIVED ADDRESS
        //ofLog(OF_LOG_NOTICE,m.getAddress());

        //GET COMMAND FROM ADDRESS
        vector<string> address = ofSplitString(m.getAddress(),"/");
   		int key = 0;
   		if (address[key] == "") key++; 

   		string command = address[key];
   		string postman = "";

   		//DETECT DISPATCHER PREFIX IN THE ADDRESS
   		if (ofIsStringInString(command, ":") or command == "*")
   		{
   			postman = command;
   			command = address[key+1];

   			//reverse FROM / TO
   			if (ofIsStringInString(postman, ":")) 
   			{
   				vector<string> fromto = ofSplitString(postman,":");
   				postman = fromto[1]+":"+fromto[0];
   			}
   		}

   		bool validCmd = true;

   		//EXECUTE COMMAND
	  	if ((command == cmd("play")) or (command == cmd("playloop")) or (command == cmd("load")))
		{
			vector<string> playlist;
			string filepath;

			for(int k = 0; k < m.getNumArgs(); k++)
				if ((m.getArgType(k) == OFXOSC_TYPE_STRING) && (m.getArgAsString(k) != "")) 
				{
					filepath = m.getArgAsString(k);
					if (base64) filepath = ofxCrypto::base64_decode(filepath);	
								
					playlist.push_back(filepath);		
				}

			if (command == cmd("playloop")) player->loop = true;
			if (playlist.size() > 0) player->load(playlist);
			else player->load();
			if (command != cmd("load")) player->play();							
		}
		else if(command == cmd("stop"))
		{
			player->stop();
		}
		else if(command == cmd("pause"))
		{
			player->pause();
		}
		else if(command == cmd("resume"))
		{
			player->resume();
		}	
		else if(command == cmd("next"))
		{
			player->next();
		}
		else if(command == cmd("prev"))
		{
			player->prev();
		}
		else if(command == cmd("seek"))
		{
			if(m.getArgType(0) == OFXOSC_TYPE_INT32) player->seek(m.getArgAsInt32(0));
		}			
		else if(command == cmd("volume"))
		{
			if(m.getArgType(0) == OFXOSC_TYPE_INT32) player->volume = m.getArgAsInt32(0);
			else if(m.getArgType(0) == OFXOSC_TYPE_FLOAT) player->volume = (int) m.getArgAsFloat(0);
		}
		else if(command == cmd("mute"))
		{
			bool doMute = true;
			if ((m.getNumArgs() > 0) && (m.getArgType(0) == OFXOSC_TYPE_INT32)) doMute = (m.getArgAsInt32(0) == 1);
		
			player->mute = doMute;
		}
		else if(command == cmd("unmute"))
		{
			player->mute = false;
		}
		else if(command == cmd("loop"))
		{
			bool doLoop = true;
			if ((m.getNumArgs() > 0) && (m.getArgType(0) == OFXOSC_TYPE_INT32)) doLoop = (m.getArgAsInt32(0) == 1);
		
			player->loop = doLoop;
		}
		else if(command == cmd("unloop"))
		{
			player->loop = false;
		}
		else if(command == cmd("blur"))
		{
			if(m.getArgType(0) == OFXOSC_TYPE_INT32) player->blur = m.getArgAsInt32(0);
			else if(m.getArgType(0) == OFXOSC_TYPE_FLOAT) player->blur = (int) m.getArgAsFloat(0);
		}
		else if(command == cmd("zoom"))
		{
			if(m.getArgType(0) == OFXOSC_TYPE_INT32) player->zoom = m.getArgAsInt32(0);
			else if(m.getArgType(0) == OFXOSC_TYPE_FLOAT) player->zoom = (int) m.getArgAsFloat(0);
		}			
		else if(command == cmd("info"))
		{
			player->info = !player->info;
		}
		else if(command == cmd("host"))
		{
			if ((m.getArgType(0) == OFXOSC_TYPE_STRING) && (m.getArgAsString(0) != "")) hostOUT = m.getArgAsString(0);
		}
		else if(command == cmd("getStatus"))
		{
			//this->status(player,postman);
			//will be send since it's a valid command !
		}				
		else if(command == cmd("quit"))
		{
			ofLog(OF_LOG_NOTICE,"-HP- QUIT ");
			player->stop();
			std::exit(0);
		}

		//KXKM regie
		else if(command == cmd("synctest"))
		{
			this->statusKXKM(player);
		}
		else if(command == cmd("fullsynctest"))
		{
			this->ipKXKM(player);
		}

		//NOT VALID
		else 
		{
			validCmd = false;
		}

		//VALID CMD => send Status!
		if (validCmd) this->status(player,postman);

		//OSC LOG
		oscDebug += this->oscToString(m)+"\n";
		//ofLog(OF_LOG_NOTICE,"-HP- OSC "+oscDebug);
    }	
}


/**
 * Get (OSC) debug message.
 *
 * \return debug message string
 */
string oscCom::log() {
	return oscDebug;
}


/**
 * Map OSC message to string.
 *
 * \return string containing address, type and value
 * \see cmd(string command)
 */
string oscCom::oscToString(ofxOscMessage m) {
	
	string message = m.getAddress()+" ";
	for(int i = 0; i < m.getNumArgs(); i++)
	{
		message += m.getArgTypeName(i);
		message += ": ";

		if(m.getArgType(i) == OFXOSC_TYPE_INT32) 		message += ofToString(m.getArgAsInt32(i));
		else if(m.getArgType(i) == OFXOSC_TYPE_FLOAT) 	message += ofToString(m.getArgAsFloat(i));
		else if(m.getArgType(i) == OFXOSC_TYPE_STRING) 	message += m.getArgAsString(i);
		else message += " unknown";
	}
	
	return message;
}


/**
 * Send current status for given player via OSC.
 *
 * \param player to send status for
 * \see status (mediaPlayer* player, string response_prefix)
 * \see statusKXKM(mediaPlayer* player)
 */
void oscCom::status(mediaPlayer *player)
{
	this->status(player, this->prefix);
}


/**
 * Send current status for given player via OSC.
 *
 * \param player to send status for
 * \param response_prefix custom (non-internal) prefix to send
 * \see status(mediaPlayer* player)
 * \see statusKXKM(mediaPlayer* player)
 */
void oscCom::status(mediaPlayer* player, string response_prefix)
{
	if (!connected) return;

	ofxOscMessage m;
	m.setAddress(response_prefix+"/status");
    
    //NAME
	m.addStringArg(player->name);

	string filepath = player->media();
	if (base64) filepath = ofxCrypto::base64_encode(filepath);

	if (player->isPlaying())
	{
        //STATUS
		if (player->isPaused()) m.addStringArg("paused");
		else m.addStringArg("playing");
		//FILE
		m.addStringArg(filepath);
        //POSITION
		m.addIntArg(player->getPositionMs());
        //DURATION
		m.addIntArg(player->getDurationMs());
        //LOOPING
		m.addIntArg( (player->loop) ? 1 : 0 );
	}
	else 
	{
		m.addStringArg("stoped");
		m.addStringArg(filepath);
		m.addIntArg(0);
		m.addIntArg(player->getDurationMs());
		m.addIntArg( (player->loop) ? 1 : 0 );
	}
	
    //VOLUME
	m.addIntArg(player->volume);
	
    //MUTED
	if (player->mute) m.addStringArg("muted");
	else m.addStringArg("unmuted");

    //ZOOM
	m.addIntArg(player->zoom);
    //BLUR
	m.addIntArg(player->blur);

	oscSender.sendMessage(m,false);
}


/**
 * \todo determine use or remove
 */
void oscCom::end(string file) 
{
	if (!connected) return;
	
	ofxOscMessage m;
	m.setAddress(this->prefix+"/end");
	m.addStringArg(file);
	oscSender.sendMessage(m,false);
}


/**
 * Get current IP.
 *
 * \return IP as string
 * \see connect()
 */
char* oscCom::getIP()
{
	int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    char interface[] = "eth0";
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    return(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}


/**
 * [KXKM](https://github.com/KomplexKapharnaum/KXKM-remoteplayer) centralized
 * control specific status command.
 *
 * \param player of which to send the data
 * \see status(mediaPlayer* player)
 */
void oscCom::statusKXKM(mediaPlayer* player)
{
	if (!connected) return;
	
	ofxOscMessage m;
	m.setAddress("/"+player->name);

	m.addStringArg("auto");  //auto or manu => external state ctrl
	m.addStringArg( (player->loop) ? "loop" : "unloop" );
	m.addStringArg("screen"); //screen or noscreen
	//m.addStringArg("normal"); //normal or faded
	
	
	if (player->isPlaying())
	{
		m.addStringArg("playmovie");
		m.addStringArg(ofFilePath::getFileName(player->media()));
	}
	else m.addStringArg("stopmovie");
	
	oscSender.sendMessage(m,false);
}

/**
 * Send player IP to [KXKM](https://github.com/KomplexKapharnaum/KXKM-remoteplayer).
 *
 * \param player of which to send the IP
 * \see status(mediaPlayer* player)
 */
void oscCom::ipKXKM(mediaPlayer* player) 
{
	if (!connected) return;

	ofxOscMessage m;
	m.setAddress("/"+player->name);

	m.addStringArg("initinfo"); 
	m.addStringArg(this->getIP());

	oscSender.sendMessage(m,false);
}
