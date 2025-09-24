/*


//------------------------- PROGRAMA PARA RLE----------------------------------------------------

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace std;

unsigned char rotate_right(unsigned char b, int n) {
    return ((b >> n) | (b << (8 - n))) & 0xFF;
}

int main(int argc, char** argv) {
    const char* filename = "C:/Users/SYSTICOM SOPORTE/Documents/GitHub/Desafio_1/Desafio1/Encriptado3.txt";
    if (argc >= 2) filename = argv[1];

    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Error: no se pudo abrir el archivo '" << filename << "'\n";
        return 1;
    }

    vector<unsigned char> buf((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    if (buf.empty()) {
        cout << "Archivo vacío.\n";
        return 0;
    }

    // XOR + Rotación inversa
    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] ^= 0x40;
        buf[i] = rotate_right(buf[i], 3);
    }

    // DEBUG: ver los primeros bytes después de XOR + rotación
    cout << "Primeros 20 bytes en HEX:\n";
    for (int i = 0; i < 20 && i < buf.size(); i++) {
        printf("%02X ", buf[i]);
    }
    cout << endl;

    // Descomprimir RLE
    cout << "\nTexto descomprimido:\n";
    for (size_t i = 0; i + 2 < buf.size(); i += 3) {
        unsigned char count = buf[i + 1]; // Cantidad
        unsigned char value = buf[i + 2]; // Valor

        for (int j = 0; j < count; ++j) {
            cout << value;
        }
    }
    cout << endl;

    return 0;
}

*/


#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>  // para memcpy
using namespace std;

// Rotar un byte n bits a la derecha
unsigned char rotate_right(unsigned char b, int n) {
    return ((b >> n) | (b << (8 - n))) & 0xFF;
}

int main(int argc, char** argv) {
    const char* filename = "C:/Users/SYSTICOM SOPORTE/Documents/GitHub/Desafio_1/Desafio1/Encriptado2.txt";
    if (argc >= 2) filename = argv[1];

    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Error: no se pudo abrir el archivo '" << filename << "'\n";
        return 1;
    }

    // Leer todo el archivo en memoria
    vector<unsigned char> buf((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    if (buf.empty()) {
        cout << "Archivo vacío.\n";
        return 0;
    }

    // XOR + Rotación inversa
    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] ^= 0x5A;
        buf[i] = rotate_right(buf[i], 3);
    }

    // ===== DESCOMPRESIÓN LZ78 =====
    // Usaremos un arreglo de punteros para el diccionario
    const int MAX_ENTRADAS = 4096; // tamaño máximo para diccionario
    char* diccionario[MAX_ENTRADAS];
    int longitudes[MAX_ENTRADAS];  // almacena la longitud de cada entrada
    int contador = 1;              // índice 0 reservado como vacío
    diccionario[0] = nullptr;
    longitudes[0] = 0;

    cout << "Texto descomprimido:\n";

    for (size_t i = 0; i + 2 < buf.size(); i += 3) {
        unsigned char index = buf[i + 1];  // Índice del diccionario
        unsigned char c = buf[i + 2];      // Carácter actual

        int longitud_nueva = longitudes[index] + 1;

        // Reservar memoria para la nueva entrada
        char* nueva = new char[longitud_nueva + 1]; // +1 para '\0'

        // Copiar la cadena anterior si el índice es > 0
        if (index > 0 && diccionario[index] != nullptr) {
            memcpy(nueva, diccionario[index], longitudes[index]);
        }

        // Agregar el nuevo carácter
        nueva[longitudes[index]] = (char)c;
        nueva[longitud_nueva] = '\0';

        // Mostrar resultado
        for (int j = 0; j < longitud_nueva; j++) {
            cout << nueva[j];
        }

        // Guardar en el diccionario
        diccionario[contador] = nueva;
        longitudes[contador] = longitud_nueva;
        contador++;

        if (contador >= MAX_ENTRADAS) {
            cerr << "\nError: diccionario lleno.\n";
            break;
        }
    }

    cout << endl;

    // Liberar memoria
    for (int i = 1; i < contador; i++) {
        delete[] diccionario[i];
    }

    return 0;
}










