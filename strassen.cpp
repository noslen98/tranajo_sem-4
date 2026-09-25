// =============================================================
// Practica N.o 04 - TRABAJO DE INVESTIGACION
// Algoritmo de Strassen vs algoritmo clasico O(n^3)
// Compilar: g++ -std=c++17 -O2 -Wall -o strassen strassen.cpp && ./strassen
// =============================================================
#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
using Clock = std::chrono::high_resolution_clock;

// Multiplicacion clasica O(n^3) con orden de bucles i-k-j (cache-friendly).
// la, lb, lc son los "anchos de fila" de cada bloque dentro de su matriz madre.
void clasico(const double* A, const double* B, double* C, int n, int la, int lb, int lc) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) C[i * lc + j] = 0;
        for (int k = 0; k < n; k++) {
            double a = A[i * la + k];
            for (int j = 0; j < n; j++) C[i * lc + j] += a * B[k * lb + j];
        }
    }
}

// C = A + s*B  (s = 1 suma, s = -1 resta) sobre bloques de tamano n x n.
void combinar(const double* A, int la, const double* B, int lb, double* C, int n, double s) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            C[i * n + j] = A[i * la + j] + s * B[i * lb + j];
}

// Algoritmo de Strassen con umbral de corte: si n <= corte se usa el clasico.
void strassen(const double* A, const double* B, double* C,
              int n, int la, int lb, int lc, int corte) {
    if (n <= corte) { clasico(A, B, C, n, la, lb, lc); return; }

    int h = n / 2;                       // tamano de cada bloque
    size_t s = (size_t)h * h;
    const double *A11 = A,          *A12 = A + h,
                 *A21 = A + h * la, *A22 = A + h * la + h;
    const double *B11 = B,          *B12 = B + h,
                 *B21 = B + h * lb, *B22 = B + h * lb + h;

    std::vector<double> T1(s), T2(s), M[7];
    for (auto& m : M) m.assign(s, 0.0);

    // Los 7 productos de Strassen (en lugar de los 8 del metodo por bloques)
    combinar(A11, la, A22, la, T1.data(), h,  1);   // M1 = (A11+A22)(B11+B22)
    combinar(B11, lb, B22, lb, T2.data(), h,  1);
    strassen(T1.data(), T2.data(), M[0].data(), h, h, h, h, corte);

    combinar(A21, la, A22, la, T1.data(), h,  1);   // M2 = (A21+A22) B11
    strassen(T1.data(), B11, M[1].data(), h, h, lb, h, corte);

    combinar(B12, lb, B22, lb, T2.data(), h, -1);   // M3 = A11 (B12-B22)
    strassen(A11, T2.data(), M[2].data(), h, la, h, h, corte);

    combinar(B21, lb, B11, lb, T2.data(), h, -1);   // M4 = A22 (B21-B11)
    strassen(A22, T2.data(), M[3].data(), h, la, h, h, corte);

    combinar(A11, la, A12, la, T1.data(), h,  1);   // M5 = (A11+A12) B22
    strassen(T1.data(), B22, M[4].data(), h, h, lb, h, corte);

    combinar(A21, la, A11, la, T1.data(), h, -1);   // M6 = (A21-A11)(B11+B12)
    combinar(B11, lb, B12, lb, T2.data(), h,  1);
    strassen(T1.data(), T2.data(), M[5].data(), h, h, h, h, corte);

    combinar(A12, la, A22, la, T1.data(), h, -1);   // M7 = (A12-A22)(B21+B22)
    combinar(B21, lb, B22, lb, T2.data(), h,  1);
    strassen(T1.data(), T2.data(), M[6].data(), h, h, h, h, corte);

    // Recomposicion de los cuatro bloques del resultado
    for (int i = 0; i < h; i++)
        for (int j = 0; j < h; j++) {
            size_t k = (size_t)i * h + j;
            C[i * lc + j]             = M[0][k] + M[3][k] - M[4][k] + M[6][k];  // C11
            C[i * lc + j + h]         = M[2][k] + M[4][k];                      // C12
            C[(i + h) * lc + j]       = M[1][k] + M[3][k];                      // C21
            C[(i + h) * lc + j + h]   = M[0][k] - M[1][k] + M[2][k] + M[5][k];  // C22
        }
}

template <class F>
double mediana_ms(F funcion, int repeticiones) {
    std::vector<double> t;
    for (int r = 0; r < repeticiones; r++) {
        auto t0 = Clock::now();
        funcion();
        t.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
    }
    std::sort(t.begin(), t.end());
    return t[t.size() / 2];
}

int main() {
    std::mt19937 gen(7);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // ---- Parte A: clasico vs Strassen (umbral de corte fijo = 64) ----
    std::cout << "n, clasico(ms), strassen(ms), aceleracion, error_max\n";
    for (int n : {32, 64, 128, 256, 512, 1024, 2048}) {
        std::vector<double> A((size_t)n * n), B((size_t)n * n),
                            C1((size_t)n * n), C2((size_t)n * n);
        for (auto& x : A) x = dist(gen);
        for (auto& x : B) x = dist(gen);

        int R = n <= 256 ? 9 : (n <= 1024 ? 3 : 1);
        double t_cla = mediana_ms([&]{ clasico(A.data(), B.data(), C1.data(), n, n, n, n); }, R);
        double t_str = mediana_ms([&]{ strassen(A.data(), B.data(), C2.data(), n, n, n, n, 64); }, R);

        double err = 0;                                  // verificacion numerica
        for (size_t i = 0; i < C1.size(); i++) err = std::max(err, std::fabs(C1[i] - C2[i]));
        std::cout << n << ", " << t_cla << ", " << t_str << ", "
                  << t_cla / t_str << ", " << err << "\n";
    }

    // ---- Parte B: efecto del umbral de corte con n = 1024 ----
    std::cout << "\numbral, tiempo(ms)   [n = 1024]\n";
    const int n = 1024;
    std::vector<double> A((size_t)n * n), B((size_t)n * n), C((size_t)n * n);
    for (auto& x : A) x = dist(gen);
    for (auto& x : B) x = dist(gen);
    for (int corte : {8, 16, 32, 64, 128, 256, 512, 1024})
        std::cout << corte << ", "
                  << mediana_ms([&]{ strassen(A.data(), B.data(), C.data(), n, n, n, n, corte); }, 3)
                  << "\n";
    return 0;
}
