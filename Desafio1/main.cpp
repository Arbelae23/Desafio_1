#include <iostream>
#include <fstream>
using namespace std;

// === FUNCIONES DE UTILIDAD ===

// Leer archivo en memoria dinámica
unsigned char* leerArchivo(const char* filename, int &size) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Error abriendo archivo: " << filename << endl;
        size = 0;
        return nullptr;
    }
    size = file.tellg();
    file.seekg(0, ios::beg);

    unsigned char* buffer = new unsigned char[size];
    file.read((char*)buffer, size);
    file.close();
    return buffer;
}

// Rotación a la izquierda
unsigned char rotarIzquierda(unsigned char b, int n) {
    return (b << n) | (b >> (8 - n));
}

// Rotación a la derecha (inversa)
unsigned char rotarDerecha(unsigned char b, int n) {
    return (b >> n) | (b << (8 - n));
}

// Desencriptar (rotación derecha + XOR)
unsigned char* desencriptar(unsigned char* data, int len, int n, unsigned char k) {
    unsigned char* out = new unsigned char[len];
    for (int i = 0; i < len; i++) {
        unsigned char temp = data[i] ^ k;
        out[i] = rotarDerecha(temp, n);
    }
    return out;
}

// === DESCOMPRESIÓN RLE ===
char* decompressRLE(unsigned char* data, int length, int* outLen) {
    if (length % 2 != 0) return nullptr; // debe ser par

    int capacity = length * 4;
    char* out = new char[capacity];
    int pos = 0;

    for (int i = 0; i + 1 < length; i += 2) {
        unsigned char count = data[i];
        unsigned char value = data[i + 1];
        for (int j = 0; j < count; j++) {
            if (pos >= capacity) { // redimensionar
                capacity *= 2;
                char* tmp = new char[capacity];
                for (int k = 0; k < pos; k++) tmp[k] = out[k];
                delete[] out;
                out = tmp;
            }
            out[pos++] = (char)value;
        }
    }
    *outLen = pos;
    return out;
}

// === DESCOMPRESIÓN LZ78 (simplificada) ===
char* decompressLZ78(unsigned char* data, int length, int* outLen) {
    const int dictCap = 256;
    int dictSize = 0;
    char** dict = new char*[dictCap];

    int cap = length * 8;
    char* output = new char[cap];
    int pos = 0;

    int i = 0;
    while (i + 1 < length) {
        unsigned char idx = data[i];
        unsigned char sym = data[i+1];
        i += 2;

        int entryLen = 0;
        char* entry = nullptr;

        if (idx == 0) {
            entryLen = 1;
            entry = new char[1];
            entry[0] = (char)sym;
        } else if (idx <= dictSize) {
            // calculamos longitud previa manualmente
            int prevLen = 0;
            while (dict[idx-1][prevLen] != '\0') prevLen++;
            entryLen = prevLen + 1;
            entry = new char[entryLen];
            for (int k = 0; k < prevLen; k++) entry[k] = dict[idx-1][k];
            entry[prevLen] = (char)sym;
        }

        if (entry) {
            for (int k = 0; k < entryLen; k++) {
                if (pos >= cap) {
                    cap *= 2;
                    char* tmp = new char[cap];
                    for (int t = 0; t < pos; t++) tmp[t] = output[t];
                    delete[] output;
                    output = tmp;
                }
                output[pos++] = entry[k];
            }
            if (dictSize < dictCap) {
                dict[dictSize++] = entry;
            } else {
                delete[] entry;
            }
        }
    }

    *outLen = pos;
    for (int d = 0; d < dictSize; d++) delete[] dict[d];
    delete[] dict;
    return output;
}

// === Buscar pista en texto ===
bool contienePista(const char* texto, int lenTexto, const char* pista, int lenPista) {
    for (int i = 0; i <= lenTexto - lenPista; i++) {
        bool ok = true;
        for (int j = 0; j < lenPista; j++) {
            if (texto[i+j] != pista[j]) { ok = false; break; }
        }
        if (ok) return true;
    }
    return false;
}

// === MAIN ===
int main() {
    // Leer mensaje y pista
    int msgLen = 0;
    unsigned char* mensaje = leerArchivo("C:/Users/SYSTICOM SOPORTE/Documents/GitHub/Desafio_1/Desafio1/Encriptado1.txt", msgLen);
    if (!mensaje) return 1;

    int pistaLen = 0;
    unsigned char* pistaBuf = leerArchivo("C:/Users/SYSTICOM SOPORTE/Documents/GitHub/Desafio_1/Desafio1/pista1.txt", pistaLen);
    if (!pistaBuf) { delete[] mensaje; return 1; }
    char* pista = new char[pistaLen+1];
    for (int i = 0; i < pistaLen; i++) pista[i] = pistaBuf[i];
    pista[pistaLen] = '\0';
    delete[] pistaBuf;

    // Probar todas las combinaciones n (1..7) y k (0..255)
    for (int n = 1; n <= 7; n++) {
        for (int k = 0; k < 256; k++) {
            // Desencriptar
            unsigned char* desencriptado = desencriptar(mensaje, msgLen, n, (unsigned char)k);

            // RLE
            int outLen = 0;
            char* rle = decompressRLE(desencriptado, msgLen, &outLen);
            if (rle && contienePista(rle, outLen, pista, pistaLen)) {
                cout << "Encontrado con RLE, n=" << n << " k=" << k << endl;
                cout << "Mensaje: ";
                for (int i = 0; i < outLen; i++) cout << rle[i];
                cout << endl;
                delete[] rle;
                delete[] desencriptado;
                goto fin; // terminamos
            }

            /*

            if (rle) {
                cout << "DEBUG (primeros 200 chars RLE): ";
                for (int i = 0; i < min(outLen, 200); i++) cout << rle[i];
                cout << endl;
            }
            */

            delete[] rle;

            // LZ78
            outLen = 0;
            char* lz = decompressLZ78(desencriptado, msgLen, &outLen);
            if (lz && contienePista(lz, outLen, pista, pistaLen)) {
                cout << "Encontrado con LZ78, n=" << n << " k=" << k << endl;
                cout << "Mensaje: ";
                for (int i = 0; i < outLen; i++) cout << lz[i];
                cout << endl;
                delete[] lz;
                delete[] desencriptado;
                goto fin;
            }

            /*

            if (lz) {
                cout << "DEBUG (primeros 200 chars LZ78): ";
                for (int i = 0; i < min(outLen, 200); i++) cout << lz[i];
                cout << endl;
            }
            */

            delete[] lz;

            delete[] desencriptado;
        }
    }

    cout << "No se encontró coincidencia con la pista." << endl;

fin:
    delete[] mensaje;
    delete[] pista;
    return 0;
}






