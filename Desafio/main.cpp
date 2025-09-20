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

int main() {
    // Mostrar en pantalla la carpeta donde se está ejecutando el programa (para verificar si el archivo se estaba leyendo)
    cout << "Ruta actual de ejecucion: "
         << filesystem::current_path() << endl;

    // Abrimos el archivo encriptado en modo binario
    ifstream archivo("C:/Users/USER/Documents/GitHub/Desafio_1/Desafio/Encriptado1.txt", ios::binary | ios::ate);
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
