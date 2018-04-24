#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WIN32_WINNT 0x501
#define DEFAULT_BUFLEN 1024

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA wsaData;    //stworzenie nowego obiektu WSADATA

SOCKET ClientSocket;

//funkcja z napisami menu
void MenuNapisy() {
    cout << endl << "\tMenu glowne serwera." << endl << endl;
    cout << "1. Ustaw port nasluchiwania." << endl;
    cout << "2. Ustaw debuggowanie kienta." << endl;
    cout << "3. Uruchom serwer." << endl;
}

//funkcja wysylajaca dane do klienta, argumentami sa gniazdo klienta, pytanie get wyslane przez niego i iformacja o debugowaniu napisow
void Wyslij(SOCKET ClientSocket, char *zadanieKienta, bool debug) {
    if (debug) cout << zadanieKienta << endl;
    char plik[DEFAULT_BUFLEN];
    for( int i = 0; i < DEFAULT_BUFLEN; i++) plik[i] = 0;//usuwamy znaki z tablicy sciezki pliku

    int index;

    for (int i = 0; zadanieKienta[i + 5] != 32; i++ ) {
        plik[i] = zadanieKienta[i + 5];//pobieramy sciezke pliku z zadania get klienta
    }

    if (debug) cout << endl << "Pytanie GET: " << plik << endl << endl;

    //brak sciezki w zadaniu klienta, wiec wysylamy plik index.html
    if (plik[0] == 0) {
        ifstream file( "index.html", ios::binary | ios::ate );//otwieramy strumien pliku
        if ( file.is_open() ){
            long filesize = file.tellg();
            char * buffer = new char[filesize];//zmienna bufora o dlugosci pliku
            file.seekg(ios::beg);
            file.read( buffer, filesize );//odczytujemy dane z pliku do bufora
            if (debug) cout << buffer << endl;
            send( ClientSocket, buffer, strlen(buffer), 0 );//wysylamy
            file.close();//zamykamy plik
        }
    }else if (string(plik) == "favicon.ico" ) {
        ifstream file( "favicon.ico", ios::binary | ios::ate );
        if ( file.is_open() ){
            long filesize = file.tellg();
            char * buffer = new char[filesize];
            file.seekg(ios::beg);
            file.read( buffer, filesize );
            if (debug) cout << buffer << endl;
            send( ClientSocket, buffer, strlen(buffer), 0 );
            file.close();
        }
    }else {
        FILE *File;
        char *Buffer;
        unsigned long Size;
        File = fopen(plik, "rb"); //otiweramy plik
        if(!File){
            Buffer = "Nie ma takiego pliku na srwerze.\n";
            printf("Error while readaing the file\n");
            send( ClientSocket, Buffer, strlen(Buffer), 0 );//wysylamy informacje o braku pliku
            return; //koniec funkcji
        }

        fseek(File, 0, SEEK_END);
        Size = ftell(File);
        fseek(File, 0, SEEK_SET);

        Buffer = new char[Size];
        int len = Size;
        fread(Buffer, Size, 1, File);
        char cSize[DEFAULT_BUFLEN];
        sprintf(cSize, "%i", Size);

        fclose(File);
        //wysylanie danych dopoki nie zostana wyslane wszystkie
        while (len > 0) {
            int amount = send(ClientSocket, Buffer, len, 0);
            if (amount <= 0) {
                // tutaj bedzie blad
            } else {
                len -= amount;
                Buffer += amount;
            }
        }
    }
}

int main() {
    cout << "Program - serwer." << endl;

    cout << "Program wykorzystuje do swojej pracy WINSOCK." << endl;

    int wyborMenu;
    char port[9] = "27015";
    string odpowiedz = "To jest odpowiedz serwera.";
    bool menu = true;
    bool debug = true;

    while(menu){
        MenuNapisy();
//wyswietlenie menu
        while (!(cin >> wyborMenu) || wyborMenu > 4 || wyborMenu < 1) {
            cout << "Wprowadz numer menu." << endl;
            cin.clear(); //kasowanie flagi b��du strumienia
            cin.sync(); //kasowanie zb�dnych znak�w z bufora
        }
        switch(wyborMenu) {
        case 1:
            cout << "Ustawianie portu nasluchiwania." << endl;
            cout << "Obecny port to: " << port << endl;
            cout << "Wprowadz nowy port nasluchiwania: " << endl;
            while (!(cin >> wyborMenu) || wyborMenu > 65535  || wyborMenu < 0){
                cout << "Wprowadz numer menu." << endl;
                cin.clear(); //kasowanie flagi b��du strumienia
                cin.sync(); //kasowanie zb�dnych znak�w z bufora
            }
            sprintf(port,"%d",wyborMenu);
            cout << "Ustawiony port to: " << port << endl;
            break;

        case 2:
            cout << "Ustawianie trybu debuggowania." << endl;
            if (debug) cout << "Debugowanie jest wlaczone." << endl;
            else cout << "Debugowanie jest wylaczone." << endl;
            cout << "1. Wlacz." << endl;
            cout << "2. Wylacz." << endl;
            while (!(cin >> wyborMenu) || wyborMenu > 2 || wyborMenu < 1){
                cout << "Wprowadz numer menu." << endl;
                cin.clear(); //kasowanie flagi b��du strumienia
                cin.sync(); //kasowanie zb�dnych znak�w z bufora
            }
            if (wyborMenu == 1) debug = true;
            else debug = false;
            break;

        case 3:
            cout << "Uruchamianie serwera." << endl;
            menu = false;
            break;
        }
    }
//koniec menu, tworzenie serwera
    int iResult;

    char sendbuf[DEFAULT_BUFLEN]= "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";   //odpowiedz serwera po nawiazaniu polaczenia
    char recvbuf[DEFAULT_BUFLEN];
    for ( int i = 0; i < DEFAULT_BUFLEN; i++) recvbuf[i] = 0;//usuwanie znakow z bufora odczytu dancyh

    cout << "Inicjacja Winsock." <<endl;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    } else cout << "Inicjacja Winsock ok." << endl;

    cout << "Tworzenie gniazda Winsock dla serwera." << endl;

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET){
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    } else cout << "Poprawnie utworzono nowe gniazdo Winsock." << endl;

    cout << "Bindowanie gniazda." << endl;
     // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR){
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    } else cout << "Bindowanie ok." << endl;

    char nazwaKomputera[255];
    gethostname(nazwaKomputera, 255);
    struct hostent *host_entry;
    host_entry=gethostbyname(nazwaKomputera);
    char * szLocalIP;
    szLocalIP = inet_ntoa (*(struct in_addr *)*host_entry->h_addr_list);

    cout << "Nazwa serwera: " << nazwaKomputera  << endl;
    cout << "Numer IP serwera: " << szLocalIP  << endl;

    freeaddrinfo(result);

    cout << "Nasluchiwanie na porcie: " << port << endl;

    if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        printf( "Listen failed with error: %ld\n", WSAGetLastError() );
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

//petla nasluchiwania
    while(true) {
        ClientSocket = INVALID_SOCKET;

        // Accept a client socket
        cout << "Czekanie na klienta..." << endl;
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        //cout << "Nawiazano polaczenie." << endl;
        iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);   //pobieranie danych wyslanych przez klient
        cout << "Otrzymano " << iResult << "b danych." << endl;
        if (debug) cout << "Zapytanie wyslane przez kienta: " << endl << recvbuf << endl;

        cout << "Wysylanie danych..." <<endl;
        iResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
        Wyslij(ClientSocket, recvbuf, debug);   //funkcja wysylajaca dane

        cout << "Zamykanie polaczenia." << endl << endl;
        shutdown(ClientSocket, SD_SEND);
        for ( int i = 0; i < DEFAULT_BUFLEN; i++) recvbuf[i] = 0;
    }

  return 0;
}
