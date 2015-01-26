#include "mbed.h"
#include "EthernetInterface.h"
#include "Motor.h"
#include "PinConfig.h"
#include "cmdline.h"
#include "config.h"
#include <string>

DigitalOut Connected(LED1);
DigitalOut Process(LED2);
DigitalOut SysEnable(LED3);

Motor motor1(PIN_PWM1,PIN_DIR1,PIN_Encoder1);
Motor motor2(PIN_PWM2,PIN_DIR2,PIN_Encoder2);
Motor motor3(PIN_PWM3,PIN_DIR3,PIN_Encoder3);
Motor motor4(PIN_PWM4,PIN_DIR4,PIN_Encoder4);

Motor *motorTable[4] = {&motor1,&motor2,&motor3,&motor4};

DigitalOut Enable(PIN_EN);
TCPSocketConnection* client;

void Mecanum(int Vx,int Vy,int Omega);

int main (void)
{
		Connected = 1;
		Process = 1;
		SysEnable = 1;
    printf("setting up\n");
    EthernetInterface eth;
    eth.init(IP,NETMASK,GATEWAY);
    eth.connect();
    printf("IP Address is %s\n", eth.getIPAddress());
		Connected = 0;
		Process = 1;
	
    TCPSocketServer server;
    server.bind(PORT);
    server.listen();

    while (true) 
		{
				Process = 1;
        printf("\nWait for new connection...\n");
        client = new TCPSocketConnection();
        server.accept(*client);
        client->set_blocking(false, 10);
				Process = 0;
        printf("Connection from: %s\n\n", client->get_address());	
        char buffer[256],str[256];
				int n = client->receive(buffer, sizeof(buffer));
				if(n <= 0)
				{
					strcpy(str," ");
					client->send(str,strlen(str));
					client->close();
					continue;
				}
				buffer[n] = 0;
				int result = CmdLineProcess(buffer);
				if(result == 0)
				{
						strcpy(str,"Command Completed!");
				}
				else if(result == CMDLINE_BAD_CMD) 
				{
						strcpy(str,"Bad command!");
				} 
				else if(result == CMDLINE_TOO_MANY_ARGS) 
				{
						strcpy(str,"Too many arguments for command processor!");
				}
				else if(result == CMDLINE_TOO_FEW_ARGS)
				{
						strcpy(str,"Too few arguments for command processor!");
				}
				else if(result == CMDLINE_INVALID_ARG)
				{
						strcpy(str,"Invalid arguments for command processor!");
				} 
				client->send(str,strlen(str));
				client->close();
    }
}

void Mecanum(int Vx,int Vy,int Omega)
{
    float speed[4];
    speed[0] = Vx + Vy - (L1 + L2) * Omega;
    speed[1] = Vx - Vy + (L1 + L2) * Omega;
    speed[2] = Vx - Vy - (L1 + L2) * Omega;
    speed[3] = Vx + Vy + (L1 + L2) * Omega;
    for(int i=0; i<4; i++) 
		{
        speed[i] = speed[i] * Polar[i];
        motorTable[i]->SetSpeed(speed[i]);
    }
}

int ProcessEnable(int argc, char *argv[])
{
		if(argc>1) 
		{
        return CMDLINE_TOO_MANY_ARGS;
    }
		Enable = 0;
		SysEnable = 0;
		return 0;
}
	
int ProcessShutdown(int argc, char *argv[])
{
		if(argc>1) 
		{
        return CMDLINE_TOO_MANY_ARGS;
    }
		Enable = 1;
		SysEnable = 1;
		return 0;
}

int ProcessSetRobotSpeed(int argc, char *argv[])
{
    static int Vx,Vy,Omega;
    if(argc<3) {
        return CMDLINE_TOO_FEW_ARGS;
    }
    if(argc>3) {
        return CMDLINE_TOO_MANY_ARGS;
    }
    if(!strcmp(argv[1],"Vx")) 
		{
        Vx = atoi(argv[2]);
    } 
		else if(!strcmp(argv[1],"Vy")) 
		{
        Vy = atoi(argv[2]);
    } 
		else if(!strcmp(argv[1],"Omega"))
		{
        Omega = atoi(argv[2]);
    } 
		else 
		{
        return CMDLINE_INVALID_ARG;
    }
		printf("Vx:%d,Vy:%d,Omega:%d\n",Vx,Vy,Omega);
    Mecanum(Vx,Vy,Omega);
    return 0;
}

int ProcessSetMotorSpeed(int argc, char *argv[])
{
    if(argc<3) {
        return CMDLINE_TOO_FEW_ARGS;
    }
    if(argc>3) {
        return CMDLINE_TOO_MANY_ARGS;
    }
    int motorIndex = atoi(argv[1]) - 1;
    if(motorIndex <0 || motorIndex > 4) {
        return CMDLINE_INVALID_ARG;
    }
		int speed = atoi(argv[2]);
    motorTable[motorIndex]->SetSpeed(speed);
    return 0;
}

int ProcessGetEncoderChange(int argc, char *argv[])
{
    if(argc<2) {
        return CMDLINE_TOO_FEW_ARGS;
    }
    if(argc>2) {
        return CMDLINE_TOO_MANY_ARGS;
    }
    int motorIndex = atoi(argv[1]) - 1;
    if(motorIndex <0 || motorIndex > 4) {
        return CMDLINE_INVALID_ARG;
    }
    char s[10];
    sprintf(s,"%d",motorTable[motorIndex]->GetEncoderChange());
    client->send(s,strlen(s));
    return 0;
}

int ProcessHelp(int argc, char *argv[])
{
		char str[512];
    if(argc == 1)
    {
			strcpy(str,"\nThese commmand can be used:\n\
EnableSystem\n\
ShutdownSystem\n\
SetRobotSpeed\n\
GetEncoderChange\n\
type \"help <Command Name>\" for detail.\n\
======================\n\
Other command are under development\n\
For more information, please contact Timmy Yao(yaoht13@mails.tsinghua.edu.cn).");
    }   
		else if(argc == 2)
		{
			if(!strcmp(argv[1],"SetRobotSpeed"))
			{
				strcpy(str,"\nSetRobotSpeed <Vx|Vy|Omega> <Speed>\n\
Vx -- X axis speed\n\
Vy -- Y axis speed\n\
Omega -- Rotation speed");
			}
		}
		else
		{
			return CMDLINE_TOO_MANY_ARGS;
		}
		client->send(str,strlen(str));		
    return 0;
}

tCmdLineEntry g_psCmdTable[] = 
{
		{ "EnableSystem", ProcessEnable, "EnableSystem" },
		{ "ShutdownSystem", ProcessShutdown, "ShutdownSystem" },
    { "SetRobotSpeed", ProcessSetRobotSpeed, "SetRobotSpeed" },
    { "SetMotorSpeed", ProcessSetMotorSpeed, "SetMotorSpeed" },
    { "GetEncoderChange", ProcessGetEncoderChange, "GetEncoderChange" },
    { "help", ProcessHelp, "Help" },
    { 0, 0, 0 }
};