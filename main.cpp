#include "memory.h"

#include <iostream>
#include <thread>
#include <Windows.h>
#include <vector>
#include <sstream> 
#include <map>

int mapX(int x);
int mapY(int y);

// offsets for CSGO 
namespace offset
{
	// client
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDEA98C;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DFFF7C;

	// entity
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_bSpotted = 0x93D;
	constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr ::std::ptrdiff_t m_lifeState = 0x25F;
	constexpr ::std::ptrdiff_t entitySize = 0x10;
}

int main(){
	// init memory class
	const auto memory = Memory{ "csgo.exe" };
	//init Uart 
	HANDLE hSerial = CreateFileW(L"COM4", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open COM port" << std::endl;
		return 1;
	}

	
	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams)) {
		CloseHandle(hSerial);
		return 1;
	}

	// Set baud rate 115200 for ESP32
	dcbSerialParams.BaudRate = CBR_115200;

	if (!SetCommState(hSerial, &dcbSerialParams)) {
		// Handle error or dont 
		CloseHandle(hSerial);
		return 1;
	}
	

	// module addresses
	const auto client = memory.GetModuleAddress("client.dll");

	// radar toggle
	bool radarEnabled = false;
	bool toggleKeyPressed = false;

	std::cout << "Radar: " << (radarEnabled ? "ON" : "OFF") << std::endl;
	while (true)
	{
		//slow down because it is alot happening for the aul esp
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		// radar toggle key (Delete key)
		if (GetAsyncKeyState(VK_DELETE) & 0x8000)  // Check if the Delete key is pressed
		{
			if (!toggleKeyPressed)
			{
				toggleKeyPressed = true;
				radarEnabled = !radarEnabled;  

				std::cout << "Radar: " << (radarEnabled ? "ON" : "OFF") << std::endl;
			}
		}else{
			toggleKeyPressed = false;
		}

		if (!radarEnabled)
			continue;

		const auto localPlayer = memory.Read <std::uintptr_t>(client + offset::dwLocalPlayer);
		const auto localTeam = memory.Read <std::int32_t>(localPlayer + offset::m_iTeamNum);


		// Get the local player's position
		float localPlayerX = memory.Read<float>(localPlayer + offset::m_vecOrigin);
		float localPlayerY = memory.Read<float>(localPlayer + offset::m_vecOrigin + 4);

		int mappedX = mapX((int)localPlayerX);
		int mappedY = mapY((int)localPlayerY);

		// Prepare data string for local player
		std::string dataToSend = "L," + std::to_string(mappedX) + "," + std::to_string(mappedY) + ";";


		for (auto i = 1; i <= 32; ++i) {
			const auto entity = memory.Read<std::uintptr_t>(client + offset::dwEntityList + i * offset::entitySize);

			// Read the team number of the current entity
			if (memory.Read<std::int32_t>(entity + offset::m_iTeamNum) == localTeam) {
				continue;
			}

			if (memory.Read<std::int32_t>(entity + offset::m_lifeState))
				continue;

			// Mark the enemy as spotted in game 
			//memory.Write<bool>(entity + offset::m_bSpotted, true);

			float enemyPlayerX = memory.Read <float> (entity + offset::m_vecOrigin);
			float enemyPlayerY = memory.Read<float>(entity + offset::m_vecOrigin + 4);

			//mappign on the ESP seemed to cause it to crash 
			int mappedEnemyX = mapX((int)enemyPlayerX);
			int mappedEnemyY = mapY((int)enemyPlayerY);


			dataToSend += "E" + std::to_string(i) + "," + std::to_string(mappedEnemyX) + "," + std::to_string(mappedEnemyY);

			if (i < 32) {
				dataToSend += ';';
			}
		}
		

		dataToSend += '/';
		DWORD bytesWritten;
		DWORD bytesRead;
		//big buffer cause i can 
		char buffer[1024]{};
		//waits for the # so the buffer isnt overloaded 
		if (ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL)) {
			if (std::string(buffer).find('#') != std::string::npos) {
				if (!WriteFile(hSerial, dataToSend.c_str(), dataToSend.length(), &bytesWritten, NULL)) {
					std::cerr << "Failed to write to COM port" << std::endl;
				}
				else {
					//std::cout << "Successfully wrote " << bytesWritten << " bytes to COM port" << std::endl;
					//std::cout << dataToSend << std::endl;
				}
			}
		}

		

	}
	//clean up close your handles boys 
	CloseHandle(hSerial);
	return 0;	
}


//map Funtion to put it on a 320 * 240 display
int map(int value, int inMin, int inMax, int outMin, int outMax) {
	return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

int mapX(int x) {
	int mappedX = map(x, -2090, 1560, 70, 250);
	return mappedX;
}

int mapY(int y) {
	int mappedY = map(y, -1000, 3060, 20, 220);
	return mappedY;
}

