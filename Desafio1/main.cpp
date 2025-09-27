#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

// Rota un byte n veces a la derecha
static inline uint8_t rotacion_derecha(uint8_t b, int n) {
    return (uint8_t)(((b >> n) | (b << (8 - n))) & 0xFF);
}

// Lee un archivo binario completo y devuelve los bytes en memoria
unsigned char* leer_documento_bytes(const char* path, size_t &outSize) {
    outSize = 0;
    ifstream in(path, ios::binary);
    if (!in) return nullptr;
    in.seekg(0, ios::end);                // Mover al final para saber el tamaño
    std::streamoff fsize = in.tellg();
    if (fsize <= 0) { in.close(); return nullptr; }
    in.seekg(0, ios::beg);                // Volver al inicio
    size_t sz = (size_t)fsize;
    unsigned char* buf = new unsigned char[sz];
    in.read((char*)buf, (std::streamsize)sz); // Leer todos los bytes
    in.close();
    outSize = sz;
    return buf;
}

// Lee el archivo de pista y quita saltos de linea
char* leer_pista_bytes(const char* path, size_t &outLen) {
    outLen = 0;
    ifstream in(path, ios::binary);
    if (!in) return nullptr;
    in.seekg(0, ios::end);
    std::streamoff fsize = in.tellg();
    in.seekg(0, ios::beg);
    size_t sz = (size_t)fsize;
    char* tmp = new char[sz + 1];
    in.read(tmp, (std::streamsize)sz);
    tmp[sz] = '\0';
    in.close();

    // Copiar solo los caracteres validos, saltando los \r y \n
    char* clean = new char[sz + 1];
    size_t p = 0;
    for (size_t i = 0; i < sz; ++i) {
        if (tmp[i] == '\r' || tmp[i] == '\n') continue;
        clean[p++] = tmp[i];
    }
    clean[p] = '\0';
    delete[] tmp;
    outLen = p;
    return clean;
}

// Asegura que el buffer tenga espacio suficiente para escribir mas datos
void asegurar_capacidad(char*& buf, size_t& cap, size_t need) {
    if (cap >= need) return;              // Ai ya hay espacio suficiente, no hace nada
    size_t newcap = (cap == 0) ? 1024 : cap;
    while (newcap < need) newcap <<= 1;   // Duplicar hasta alcanzar el tamaño necesario
    char* nb = new char[newcap];
    if (buf) {
        memcpy(nb, buf, cap);             // Copiar contenido viejo al nuevo buffer
        delete[] buf;
    }
    buf = nb;
    cap = newcap;
}

// Descomprime datos con RLE (tripletas de count, value)
char* rle_triplet_count_value(const unsigned char* b, size_t len, size_t &outLen) {
    outLen = 0;
    char* out = nullptr;
    size_t cap = 0;

    if (len < 3) return nullptr;          // No se puede si hay menos de 3 bytes

    // leer de a 3 bytes: ignorar el primero, usar count y value
    for (size_t i = 0; i + 2 < len; i += 3) {
        uint8_t count = b[i + 1];         // Numero de repeticiones
        char value = (char)b[i + 2];      // Caracter a repetir
        if (count == 0) continue;
        size_t need = outLen + (size_t)count + 1;
        asegurar_capacidad(out, cap, need);
        for (uint8_t c = 0; c < count; ++c) out[outLen++] = value;
    }
    if (out) out[outLen] = '\0';
    return out;
}

// Descomprime datos con LZ78 usando bloques de 3 bytes (index, char)
char* lz78_3bytes(const unsigned char* b, size_t len, size_t &outLen) {
    outLen = 0;
    if (len < 3) return nullptr;

    const int MAX_ENTRIES = 256;          // Tamaño maximo del diccionario
    char** dict = new char*[MAX_ENTRIES];
    size_t* dict_len = new size_t[MAX_ENTRIES];

    for (int i = 0; i < MAX_ENTRIES; i++) {
        dict[i] = nullptr;
        dict_len[i] = 0;
    }

    char* out = nullptr;
    size_t cap = 0;
    size_t dict_size = 1;                 // Indice 0 vacio

    // leer de a 3 bytes
    for (size_t i = 0; i + 2 < len; i += 3) {
        uint8_t index = b[i + 1];
        uint8_t c = b[i + 2];

        const char* prev_str = "";
        size_t prev_len = 0;

        // Si el indice existe, tomar su contenido
        if (index < dict_size && dict[index] != nullptr) {
            prev_str = dict[index];
            prev_len = dict_len[index];
        }

        // Crear nueva cadena = anterior + caracter actual
        size_t new_len = prev_len + 1;
        char* new_str = new char[new_len + 1];
        if (prev_len > 0) memcpy(new_str, prev_str, prev_len);
        new_str[prev_len] = (char)c;
        new_str[new_len] = '\0';

        // Copiar al resultado final
        size_t need = outLen + new_len + 1;
        asegurar_capacidad(out, cap, need);
        memcpy(out + outLen, new_str, new_len);
        outLen += new_len;
        out[outLen] = '\0';

        // Guardar en el diccionario
        if (dict_size < MAX_ENTRIES) {
            dict[dict_size] = new_str;
            dict_len[dict_size] = new_len;
            dict_size++;
        } else {
            delete[] new_str;
        }
    }

    // Liberar memoria del diccionario
    for (int i = 1; i < dict_size; i++) {
        if (dict[i] != nullptr) {
            delete[] dict[i];
        }
    }
    delete[] dict;
    delete[] dict_len;

    return out;
}

// Busca si una pista existe dentro de un texto
bool pista_en_documento(const char* txt, size_t lenTxt, const char* pat, size_t lenPat) {
    if (!txt || !pat) return false;
    if (lenPat == 0) return true;         // Si la pista esta vacia, siempre es verdadero
    if (lenTxt < lenPat) return false;    // Si el texto es mas chico, no puede estar
    // Recorrer todo el texto comparando caracter por caracter
    for (size_t i = 0; i + lenPat <= lenTxt; ++i) {
        size_t j = 0;
        for (; j < lenPat; ++j) {
            if (txt[i + j] != pat[j]) break;
        }
        if (j == lenPat) return true;     // Si encontro todos los caracteres seguidos
    }
    return false;
}

int main() {
    const char* rutaBase = "C:/Users/USER/Documents/GitHub/Desafio_1/Desafio1/";

    int cantidad = 0;
    bool valido = false;
    while (!valido) {
        cout << "Cuantos documentos desea desencriptar? ";
        cin >> cantidad;
        if (cin.fail() || cantidad <= 0) {
            cin.clear();                // limpiar estado de error
            cin.ignore(10000, '\n');    // descartar lo que quedó en el buffer
            cout << "Debe ingresar un numero entero positivo.\n";
            valido = false;
            continue;
        }
        valido = true;
        for (int i = 1; i <= cantidad; i++) {
            char nombreEnc[256], nombrePista[256];
            sprintf(nombreEnc, "%sEncriptado%d.txt", rutaBase, i);
            sprintf(nombrePista, "%sPista%d.txt", rutaBase, i);

            ifstream f1(nombreEnc);
            ifstream f2(nombrePista);
            if (!f1 || !f2) {
                cout << "Solo existen los archivos hasta " << (i - 1) << ". Intente otra cantidad.\n";
                valido = false;
                break;
            }
        }
    }

    // Procesar cada documento
    for (int doc = 1; doc <= cantidad; doc++) {
        char fichero[256], pistaFile[256];
        sprintf(fichero, "%sEncriptado%d.txt", rutaBase, doc);
        sprintf(pistaFile, "%sPista%d.txt", rutaBase, doc);

        // Leer archivo encriptado
        size_t original_sz = 0;
        unsigned char* original = leer_documento_bytes(fichero, original_sz);
        if (!original) {
            cerr << "No se pudo leer fichero: " << fichero << "\n";
            continue;
        }

        // Leer pista
        size_t pista_len = 0;
        char* pista = leer_pista_bytes(pistaFile, pista_len);
        if (!pista) {
            cerr << "No se pudo leer la pista: " << pistaFile << "\n";
            delete[] original;
            continue;
        }

        cout << "\n Procesando Documento " << doc << " :\n";
        cout << "Archivo leido: " << original_sz << " bytes. Pista: \"" << pista << "\"\n";
        cout << "Probando combinaciones n=1..7, k=0..255\n";

        unsigned char* buf = new unsigned char[original_sz];
        bool encontrado = false;
        int n_encontrado = 0, k_encontrado = 0;
        char* mensaje_decodificado = nullptr;
        size_t mensaje_len = 0;
        const char* tipo_algoritmo = nullptr;

        // Primer intento: probar RLE
        for (int n = 1; n <= 7 && !encontrado; ++n) {
            for (int k = 0; k <= 255 && !encontrado; ++k) {
                for (size_t i = 0; i < original_sz; ++i) {
                    uint8_t temp = (uint8_t)(original[i] ^ (uint8_t)k);
                    buf[i] = rotacion_derecha(temp, n);
                }
                size_t rle_out_len = 0;
                char* rle_out = rle_triplet_count_value(buf, original_sz, rle_out_len);
                if (rle_out) {
                    if (pista_en_documento(rle_out, rle_out_len, pista, pista_len)) {
                        encontrado = true;
                        n_encontrado = n;
                        k_encontrado = k;
                        mensaje_decodificado = rle_out;
                        mensaje_len = rle_out_len;
                        tipo_algoritmo = "RLE";
                        break;
                    }
                    delete[] rle_out;
                }
            }
        }

        // Segundo intento: probar LZ78
        if (!encontrado) {
            for (int n = 1; n <= 7 && !encontrado; ++n) {
                for (int k = 0; k <= 255 && !encontrado; ++k) {
                    for (size_t i = 0; i < original_sz; ++i) {
                        uint8_t temp = (uint8_t)(original[i] ^ (uint8_t)k);
                        buf[i] = rotacion_derecha(temp, n);
                    }
                    size_t lz_out_len = 0;
                    char* lz_out = lz78_3bytes(buf, original_sz, lz_out_len);
                    if (lz_out) {
                        if (pista_en_documento(lz_out, lz_out_len, pista, pista_len)) {
                            encontrado = true;
                            n_encontrado = n;
                            k_encontrado = k;
                            mensaje_decodificado = lz_out;
                            mensaje_len = lz_out_len;
                            tipo_algoritmo = "LZ78";
                            break;
                        }
                        delete[] lz_out;
                    }
                }
            }
        }

        if (encontrado) {
            cout << "=== ENCONTRADO ===" << endl;
            cout << "Algoritmo: " << tipo_algoritmo << endl;
            cout << "Parametros: n=" << n_encontrado << ", k=" << k_encontrado
                 << " (0x" << hex << k_encontrado << dec << ")" << endl;
            cout << "\nMensaje decodificado:" << endl;
            cout << "====================" << endl;
            cout << mensaje_decodificado << endl;
            cout << "====================" << endl;
        } else {
            cout << "No se encontro la pista con ninguna combinacion." << endl;
        }

        delete[] buf;
        delete[] original;
        delete[] pista;
        if (mensaje_decodificado) delete[] mensaje_decodificado;
    }

    return 0;
}


