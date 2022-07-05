#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 1024

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

//формирование 256-битного ключа
void form_key(uint32_t* key, short int size_key) {
	uint64_t key_init = 0;//изначальный ключ 56 бит
	short int size_key_init = 56;
	short int rls = 2991;
	bool b = 0;
	//формируем ключ
	for (int i = 0; i < size_key_init; i++)
	{
		b = ((rls >> 12) ^ (rls >> 11)) & 0x1;
		rls = rls << 1;
		rls = rls + b;
		key_init = (key_init << 1) | b;
	}
	//дописываем ключ в 256 бит
	int k = 0;
	for (int i = 0; i < size_key; i++)
	{
		key[i] = 0;
		for (int j = 0; j < 32; j++) {
			key[i] = (key[i] << 1) | ((key_init >> k) & 1);//добавляем в начало ключа 1 бит
			if (j != 31) {//последний раз не сдвигаем
				key[i] = key[i] << 1;//сдвигаем на один разряд влево
			}
			if (k == size_key_init - 1) {//55 - номер последнего элемента sizeKey[]
				k = 0;
				continue;//пропускаем k++
			}
			k++;
		}
	}
}

//зашифрование шифром Магма
bool magma(uint32_t* key, string name_readFile) {
	string name_writeFile;
	unsigned long long int size_file_byte = 0;
	//определяем размер файла
	ifstream fin(name_readFile, ios::binary);
	fin.seekg(0, std::ios::end);
	size_file_byte = fin.tellg();
	fin.close();
	if (size_file_byte > 20480) {
		cout << "File error:\nSize over 20Kb";
		return false;
	}
	fin.open(name_readFile, ios::binary);
	ofstream fout(name_writeFile, ios::binary);
	if (!fin.is_open() || !fout.is_open()) {
		fin.close();
		fout.close();
		return false;
	}
	uint8_t remain_bytes = 0;
	double count_block = ceil((double)size_file_byte / 8);//количество считываемых 64-х битных блоков
	uint8_t pi_i = 0;//значения элементов массива pi 
	uint32_t r = 0, l = 0, trf = 0;//32 бита - левыйй и правый блоки
	uint8_t ch = 0;//8 бит - считанный байт
	uint8_t arr_index_key_en[32] = { 0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0 };
	uint8_t arr_index_key_de[32] = { 0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0,7,6,5,4,3,2,1,0,7,6,5,4,3,2,1,0 };
	uint8_t arr_index_key[32];

	for (int i = 0; i < 32; i++)//заполняем блоки,считывая 8 байт
	{
		arr_index_key[i] = arr_index_key_en[i];
	}
	
	
	uint8_t arr_pi[8][16] =
	{
		{12, 4, 6, 2, 10, 5, 11, 9, 14, 8, 13, 7, 0, 3, 15, 1},
		{6, 8, 2, 3, 9, 10, 5, 12, 1, 14, 4, 7, 11, 13, 0, 15},
		{11, 3, 5, 8, 2, 15, 10, 13, 14, 1, 7, 4, 12, 9, 6, 0},
		{12, 8, 2, 1, 13, 4, 15, 6, 7, 0, 10, 5, 3, 14, 9, 11},
		{7, 15, 5, 10, 8, 1, 6, 13, 0, 9, 3, 14, 11, 4, 2, 12},
		{5, 13, 15, 6, 9, 2, 12, 10, 11, 7, 8, 1, 4, 3, 14, 0},
		{8, 14, 2, 5, 6, 9, 1, 12, 15, 4, 11, 0, 13, 10, 3, 7},
		{1, 7, 14, 13, 0, 5, 8, 3, 4, 15, 10, 6, 9, 12, 11, 2}
	};
	for (int g = 0; g < count_block; g++)
	{
		for (int i = 0; i < 8; i++)//заполняем блоки,считывая по 1 байту
		{
			if (!fin.read((char*)&ch, sizeof(char)))
				break;//считаны все символы

			r <<= 8;
			r = r | ch;
			if (i == 3) {
				l = r;
				r = 0;
			}
		}
		if (((g + 1) == count_block) && (size_file_byte % 8 != 0)) {
			int remain_bit = 64 - (size_file_byte % 8) * 8;//сколько бит осталось незаполненных
			if (remain_bit <= 32) {//если левый блок заполнен, дозаполняем правый, прибавляя единицу и заполнение нулями
				r = ((r << 1) | 1) << (remain_bit - 1);//remain_bit-1, так как правый блок уже сдвинулся
			}
			else {//если левый блок не заполнен
				l = ((r << 1) | 1) << (remain_bit - 33);
				r = 0;
			}
		}
		for (int i = 0; i < 32; i++) {
			trf = r;
			r = key[arr_index_key[i]] + r;
			//преобразуем блок из таблицы пи
			for (int k = 0; k < 8; k++) {
				pi_i = r & 15;//берем первые четыре младших бита
				r = r >> 4;//сдвигаем вправо 
				r = r | (arr_pi[k][pi_i] << 28);//вставляем 4 бита в страшие разряды
			}

			r = (r << 11) | (r >> 21);//сдвигаем влево 
			r = r ^ l;
			l = trf;

		}
		trf = l;
		l = r;
		r = trf;
		ch = 0;
		for (int i = 0; i < 8; i++)//заполняем блоки,считывая по 1 байту
		{
			ch = (l >> 24) & 255;
			fout.write((char*)&ch, sizeof(char));
			l <<= 8;
			if (i == 3) {
				l = r;
			}
		}
	}
	fin.close();
	fout.close();
}

void string_to_char(string str, char* ch){
	for (int i = 0; i < str.size(); i++)
		ch[i] = str[i];
	ch[str.size()] = '\0';
}

void send_file(SOCKET* sock, const string& way) {
	
	char buff[DEFAULT_BUFLEN] = { 0 };
	int i = 0;
	
	ifstream file;
	file.open(way);

	if (!file.good()) {
		cout << "Error open file" << endl;
		exit(1);
	}
	
	uint32_t key[8];
	int size_key = 8;
	form_key(key, size_key);//формируем ключ 256 бит
	string path_enFile = way;
	magma(key, path_enFile);//зашифровываем
	file.open(path_enFile);
	while (file.get(buff[i]))
		i++;
	buff[i] = '\0';
	send(*sock, &buff[0], sizeof(buff), 0);
	file.close();
	
}


int main() {

	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult = NULL;
	SOCKET ClientSock = INVALID_SOCKET;

	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d/n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo("localhost", "22000", &hints, &addrResult);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d/n", iResult);
		WSACleanup();
		return 1;
	}

	ClientSock = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ClientSock == INVALID_SOCKET) {
		cout << "Socked failed" << endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}

	iResult = connect(ClientSock, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "Connect failed" << endl;
		closesocket(ClientSock);
		ClientSock = INVALID_SOCKET;
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}

	//Работа с файлом

	while (true) {
		string way;
		cout << "Cpecify the part to the file:" << endl;
		getline(cin, way); // /C++/test.txt
		if (way == "exit") {
			closesocket(ClientSock);
			WSACleanup();
			break;
		}
		send_file(&ClientSock, way);
	}
	

	return 0;
}
