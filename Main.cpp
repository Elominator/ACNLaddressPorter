/*
		ACNL code.bin Adress Porter
		By Elominator
*/

#include <iostream>
#include <list>
#include <fstream>
#include <streambuf>
#include <string>
#include <vector>
using namespace std;

/*
	Notes:
	Functions start with E92D
	and sometimes stop with E8BD
*/

void InputPaths(std::string& sourcepath, vector<string>& destPaths)
{
	cout << "Please Enter the Path to Source file: ";
	cin >> sourcepath;
	cout << "Please Enter all Paths to Destination files (stop with q): ";
	while (true) {
		string temp;
		cin >> temp;
		if (temp == "q") {
			//cout << "OK!";
			break;
		}
		destPaths.push_back(temp);
	}
}

void EnterAdress(int& adress)
{
	cout << "Please enter Adress: ";
	cin >> std::hex >> adress;
	adress += adress % 4;
	adress -= 0xF0000;
}

vector<unsigned char> getVectorData(std::string& sourcepath)
{
	std::ifstream input(sourcepath, std::ios::binary);
	vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
	return buffer;
}

void adressOut(int adress, std::vector<unsigned char>& source)
{
	cout << endl << adress+0xF0000 << ": ";
	for (int j = 0; j < 4; j++) {
		cout << (int)source[adress + j] << " ";
	}
}

void calcOffset(std::vector<unsigned char>& source, int adress, int& offset)
{
	for (int i = 0; (int)source[adress  - i + 7] != 0xE9 && adress  - i >= 0; i += 4) {
		if ((int)source[adress -  i + 3] == 0xE9) {
			offset = i;
		}
	}
}

void calcFunctionLength(int offset, std::vector<unsigned char>& source, int adress, int& functionLength)
{
	for (int i = 4 - offset; (int)source[adress  + i + 3] != 0xE9;i += 4) {
		functionLength = i+offset;
	}
}

void fuctionOut(int functionStart, int functionLength, int adress, std::vector<unsigned char>& source)
{
	for (int i = functionStart; i <= functionLength+functionStart; i += 4) {

		adressOut(i, source);
	}
}

void calcFunctionStart(int& functionStart, int adress, int offset)
{
	functionStart = adress - offset;
}

void calcFunction(std::vector<unsigned char>& source, int adress, int& offset, int& functionLength, int& functionStart)
{
	calcOffset(source, adress, offset);
	calcFunctionLength(offset, source, adress, functionLength);
	calcFunctionStart(functionStart, adress, offset);
}

void getVectors(std::vector<unsigned char>& source, std::string& sourcepath, std::vector<std::string>& destPaths, std::vector<std::vector<unsigned char>>& destination)
{
	source = getVectorData(sourcepath);
	for (int i = 0; i < destPaths.size(); i++) {
		destination.push_back(getVectorData(destPaths[i]));
	}
}

int searchForAsm(std::vector<unsigned char>& destination, int functionStart, int functionLength, vector<int>& foundFunction, std::vector<unsigned char>& source)
{
	//cout << "Start search...";
	int found = 0;
		for (int i = 3; i < destination.size(); i += 4) {
			for (int j = functionStart + 3; j <= functionStart + functionLength; j += 4) {
				if ((j - functionStart + 1) / 4 == functionLength / 4) {
					foundFunction.push_back(i - functionLength + 1);
					//cout << "\nFound Function: " << foundFunction + 0xF0000 << endl;
					found++;
				}
				if ((int)destination[i] == (int)source[j]) {
					i += 4;
				}
				else {
					break;
				}
			}
		}
		return found;

}

int searchForMemory(std::vector<unsigned char>& destination, std::vector<int>& foundPort, std::vector<unsigned char>& source, int adress)
{

	int found = 0;
	int range = 20;			//max searched function range; standard 20
	int failrate = 5;		//max differences in a row; standard 5
	int failures = 0;

		for (int j = 0; j < destination.size(); j++) {
			for (int k = 0; k <= range; k++) {
				if (k == range) {
					//cout << "Found for range " << range << " and failrate of "<< failrate <<" at :" <<std::hex<< j +0xF0000<< endl;
					foundPort.push_back(j + 0xF0000);
					found++;
				}
				if (j + k >= destination.size()) {
					break;
				}
				if (adress + k >= source.size()) {
					break;
				}
				if ((int)source[adress + k] != (int)destination[j + k]) {
					if (failures > failrate) {
						break;
					}
					else {
						failures++;
					}
				}
			}
			failures = 0;
		}
		return found;
}

int main() {
	string sourcepath;
	vector<string> destPaths;
	int adress;
	int offset = 0;		//offset from function start to adress
	int functionLength = 0;
	int functionStart = 0;

	vector<vector<unsigned char>> destination;
	vector<unsigned char> source;
	InputPaths(sourcepath, destPaths);

  /*
	sourcepath = "D:\\Dokumente\\ExeFsElf\\US.elf";
	destPaths.push_back("D:\\Dokumente\\ExeFsElf\\EUR.elf");
	destPaths.push_back("D:\\Dokumente\\ExeFsElf\\JAP.elf");
  */

	cout << "Initializing... "<<endl;

	getVectors(source, sourcepath, destPaths, destination);

	while (true) {
		vector<int> foundFunction;
		vector<int> foundPort;

		EnterAdress(adress);
		if (adress >= source.size()) {
			cout << "Adress out of range!\n";
			continue;
		}
		calcFunction(source, adress, offset, functionLength, functionStart);


		vector<int> anzahlFound;
		for (int i = 0; i < destination.size(); i++) {
			anzahlFound.push_back( searchForAsm(destination[i], functionStart, functionLength, foundFunction, source));
			if (anzahlFound[i]==0) {
				//cout << "MemorySearch...\n";
				anzahlFound[i]=searchForMemory(destination[i], foundPort, source, adress);
			}
		}
		vector<int> destinationNumber;
		for (int i = 0; i < anzahlFound.size(); i++) {
			for (int j = 0; j < anzahlFound[i]; j++) {
				destinationNumber.push_back(i);
			}
		}
		for (int i = 0; i < foundFunction.size(); i++) {
			foundPort.push_back(foundFunction[i] + offset + 0xF0000);
		}
		for (int i = 0; i < foundPort.size(); i++) {
			cout << "Ported Adresse for destination "<< destinationNumber[i] <<": " << std::hex << foundPort[i]<< endl;
		}

	}
}
