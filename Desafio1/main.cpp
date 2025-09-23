#include <iostream>
#include <fstream>      // Libreria para abrir y leer archivos
#include <filesystem>   // Libreria para obtener la ruta de ejecucion
using namespace std;

// Función que rota los bits de un byte a la derecha n posiciones
unsigned char rotarDerecha(unsigned char byte, int n) {
    return (byte >> n) | (byte << (8 - n));  // Se hace corrimiento y se juntan los bits
}

// Función que aplica la desencriptacion a todo el buffer
void desencriptar(unsigned char* buffer, int size, int n, unsigned char K) {
    for (int i = 0; i < size; i++) {
        unsigned char c = buffer[i];   // Tomamos el byte actual
        c = c ^ K;                     // Primero le aplicamos un XOR con la clave
        c = rotarDerecha(c, n);        // Luego rotamos sus bits n posiciones a la derecha
        buffer[i] = c;                 // Guardamos el resultado en el mismo lugar
    }
}


// ======== DESCOMPRESIÓN RLE ========
char* decompressRLE(const unsigned char* data, int length, int* outLen) {
    // Reservar memoria con capacidad máxima (asumimos peor caso)
    char* result = (char*) malloc(length * 10);
    int pos = 0;

    for (int i = 0; i < length; i += 2) {
        unsigned char count = data[i];    // número de repeticiones
        unsigned char value = data[i+1];  // carácter repetido
        for (int j = 0; j < count; j++) {
            result[pos++] = value;
        }
    }

    *outLen = pos;
    return result;
}


// ======== DESCOMPRESIÓN LZ78 (versión simple) ========
char* decompressLZ78(const unsigned char* data, int length, int* outLen) {
    // Reservar memoria para salida
    char* result = (char*) malloc(length * 10);
    int pos = 0;

    // Diccionario (simplificado con arrays estáticos)
    char* dict[1024];
    int dictSize = 0;

    for (int i = 0; i < length; i += 2) {
        int index = data[i];       // índice del diccionario
        char nextChar = data[i+1]; // nuevo carácter

        // Recuperar la entrada
        if (index > 0 && index <= dictSize) {
            char* entry = dict[index - 1];
            int len = strlen(entry);
            memcpy(result + pos, entry, len);
            pos += len;
        }

        // Añadir el nuevo carácter
        result[pos++] = nextChar;

        // Guardar nueva entrada en el diccionario
        int entryLen = ((index > 0) ? strlen(dict[index-1]) : 0) + 1;
        char* newEntry = (char*) malloc(entryLen + 1);
        if (index > 0) strcpy(newEntry, dict[index-1]);
        else newEntry[0] = '\0';
        int len = strlen(newEntry);
        newEntry[len] = nextChar;
        newEntry[len+1] = '\0';

        dict[dictSize++] = newEntry;
    }

    *outLen = pos;
    return result;
}

// ======== VERIFICAR PISTA ========
bool containsFragment(const char* text, int len, const char* fragment) {
    int fragLen = strlen(fragment);
    for (int i = 0; i <= len - fragLen; i++) {
        if (memcmp(text + i, fragment, fragLen) == 0) {
            return true;
        }
    }
    return false;
}

int main() {
    // Mostrar en pantalla la carpeta donde se está ejecutando el programa (para verificar si el archivo se estaba leyendo)
    cout << "Ruta actual de ejecucion: "
         << filesystem::current_path() << endl;

    // Abrimos el archivo encriptado en modo binario
    ifstream archivo("C:/Users/SYSTICOM SOPORTE/Documents/GitHub/Desafio_1/Desafio1/Encriptado1.txt", ios::binary | ios::ate);
    if (!archivo) {  // Si no se puede abrir el archivo
        cout << "No se pudo abrir el archivo." << endl;
        return 1;    // Salimos del programa
    }

    // Obtenemos el tamaño total del archivo en bytes
    streamsize size = archivo.tellg();
    archivo.seekg(0, ios::beg);  // Volvemos al inicio del archivo

    // Reservamos memoria dinamica para guardar el archivo completo
    unsigned char* original = new unsigned char[size];
    archivo.read(reinterpret_cast<char*>(original), size);  // Leemos todo en memoria
    archivo.close();  // Cerramos el archivo porque ya no lo necesitamos

    cout << "Archivo leido, tamaño: " << size << " bytes\n";

    // Probamos todas las combinaciones posibles:
    // n = cantidad de bits de rotacion (1 a 7)
    // K = clave XOR (0 a 255)
    for (int n = 1; n < 8; n++) {
        for (int K = 0; K < 256; K++) {
            // Creamos una copia del buffer original (para no dañarlo)
            unsigned char* copia = new unsigned char[size];
            for (int i = 0; i < size; i++) copia[i] = original[i];

            // Aplicamos la desencriptacion con la combinacion actual (n,K)
            desencriptar(copia, size, n, (unsigned char)K);

            // Imprimimos en consola los primeros 50 caracteres resultantes
            cout << "n=" << n << ", K=" << K << " => ";
            for (int i = 0; i < 50 && i < size; i++) {
                unsigned char c = copia[i];
                if (c >= 32 && c <= 126) cout << c; // Si es un carácter visible lo mostramos
                else cout << "_";                   // Si no, ponemos un barra al piso
            }
            cout << endl;  // Fin de la linea

            delete[] copia;  // Liberamos la memoria de la copia
        }
    }

    // Al final liberamos la memoria del buffer original
    delete[] original;
    return 0;
}
