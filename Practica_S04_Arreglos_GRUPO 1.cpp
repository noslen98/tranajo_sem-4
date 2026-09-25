// =============================================================
// Practica N.o 04 - Arreglos dinamicos, vectores y matrices (SIS210, UNAP 2026)
// Actividad 2: std::vector, capacity() y reserve()
// Actividad 3: localidad de cache (filas vs columnas)
// Compilar: g++ -std=c++17 -O2 -Wall -o arreglos Practica_S04_Arreglos_ApellidoNombre.cpp && ./arreglos
// =============================================================
#include <vector>
#include <deque>
#include <iostream>
#include <chrono>
#include <algorithm>
using Clock = std::chrono::high_resolution_clock;

double mediana(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
double ms_desde(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

void actividad2() {
    // ---------- 1. Momentos de redimensionamiento ----------
    std::vector<int> v;
    v.reserve(0);                       // empezar sin pre-reservar
    std::cout << "Momento de redimensionamiento:\n";
    std::cout << "  #   size      capacity   ratio   copias\n";
    size_t cap_anterior = 0;
    unsigned long long copias = 0;
    int redim = 0;
    for (int i = 0; i < 1'000'000; i++) {
        size_t antes = v.size();        // elementos que habra que copiar
        v.push_back(i);
        if (v.capacity() != cap_anterior) {
            copias += antes;
            redim++;
            std::cout << "  " << redim
                      << "   size=" << v.size()
                      << "   capacity=" << v.capacity()
                      << "   ratio=" << (cap_anterior > 0
                                         ? (double)v.capacity() / cap_anterior : 0)
                      << "   copias=" << antes << '\n';
            cap_anterior = v.capacity();
        }
    }
    std::cout << "Redimensionamientos: " << redim
              << "   Copias totales: " << copias
              << "   (2n = " << 2 * 1'000'000 << ")\n\n";

    // ---------- 2. Benchmark: push_back vs pre-reservar ----------
    const int N = 1'000'000, R = 11;    // R repeticiones -> mediana
    std::vector<double> sin_reserva, con_reserva, deque_front;
    for (int r = 0; r < R; r++) {
        std::vector<int> v1;
        auto t0 = Clock::now();
        for (int i = 0; i < N; i++) v1.push_back(i);
        sin_reserva.push_back(ms_desde(t0));

        std::vector<int> v2;
        v2.reserve(N);                  // evita todos los redimensionamientos
        t0 = Clock::now();
        for (int i = 0; i < N; i++) v2.push_back(i);
        con_reserva.push_back(ms_desde(t0));

        std::deque<int> d;              // referencia para la pregunta 8.3
        t0 = Clock::now();
        for (int i = 0; i < N; i++) d.push_front(i);
        deque_front.push_back(ms_desde(t0));
    }
    double ms_sin = mediana(sin_reserva), ms_con = mediana(con_reserva);
    std::cout << "Sin reserva:            " << ms_sin << " ms\n";
    std::cout << "Con reserve():          " << ms_con << " ms\n";
    std::cout << "Aceleracion:            " << ms_sin / ms_con << "x\n";
    std::cout << "deque::push_front:      " << mediana(deque_front) << " ms\n\n";

    // ---------- 3. Acceso aleatorio: vector vs deque (pregunta 8.3) ----------
    std::vector<int> vv(N);
    std::deque<int> dd(N);
    for (int i = 0; i < N; i++) { vv[i] = i; dd[i] = i; }
    std::vector<int> idx(N);
    unsigned s = 12345;
    for (int i = 0; i < N; i++) { s = s * 1103515245u + 12345u; idx[i] = (s >> 8) % N; }

    std::vector<double> t_vec, t_deq;
    long long suma = 0;                 // se imprime para que no se elimine el bucle
    for (int r = 0; r < R; r++) {
        auto t0 = Clock::now();
        for (int i = 0; i < N; i++) suma += vv[idx[i]];
        t_vec.push_back(ms_desde(t0));

        t0 = Clock::now();
        for (int i = 0; i < N; i++) suma += dd[idx[i]];
        t_deq.push_back(ms_desde(t0));
    }
    std::cout << "Acceso aleatorio vector: " << mediana(t_vec) << " ms\n";
    std::cout << "Acceso aleatorio deque:  " << mediana(t_deq) << " ms\n";
    std::cout << "Ratio deque/vector:      " << mediana(t_deq) / mediana(t_vec) << "x\n";
    std::cout << "(suma de control: " << suma << ")\n";
    
}

void actividad3() {
    const int R = 9;                                  // repeticiones -> mediana
    std::cout << "N, filas(ms), columnas(ms), ratio, plano_filas(ms), plano_cols(ms), ratio_plano\n";

    for (int N : {100, 500, 1000, 2000, 4000}) {
        // matriz de la guia: vector de vectores (cada fila es contigua)
        std::vector<std::vector<long long>> M(N, std::vector<long long>(N, 1LL));
        // variante adicional: un unico bloque de N*N elementos (todo contiguo)
        std::vector<long long> F((size_t)N * N, 1LL);

        std::vector<double> filas, cols, p_filas, p_cols;
        long long suma = 0, control = 0;   // 'control' evita que -O2 elimine los bucles

        for (int r = 0; r < R; r++) {
            // --- POR FILAS: j recorre memoria contigua (cache-friendly) ---
            suma = 0;
            auto t0 = Clock::now();
            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++)
                    suma += M[i][j];
            filas.push_back(ms_desde(t0));  control += suma;

            // --- POR COLUMNAS: salto de N*8 bytes en cada acceso ---
            suma = 0;
            t0 = Clock::now();
            for (int j = 0; j < N; j++)
                for (int i = 0; i < N; i++)
                    suma += M[i][j];
            cols.push_back(ms_desde(t0));   control += suma;

            // --- Mismas pruebas sobre el vector plano ---
            suma = 0;
            t0 = Clock::now();
            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++)
                    suma += F[(size_t)i * N + j];
            p_filas.push_back(ms_desde(t0)); control += suma;

            suma = 0;
            t0 = Clock::now();
            for (int j = 0; j < N; j++)
                for (int i = 0; i < N; i++)
                    suma += F[(size_t)i * N + j];
            p_cols.push_back(ms_desde(t0));  control += suma;
        }

        double f = mediana(filas), c = mediana(cols);
        double pf = mediana(p_filas), pc = mediana(p_cols);
        std::cout << N << ", " << f << ", " << c << ", " << c / f
                  << ", " << pf << ", " << pc << ", " << pc / pf
                  // usar 'control' evita que -O2 elimine los bucles medidos:
                  << "   (control = " << control << ", esperado = "
                  << 4LL * R * N * N << ")\n";
    }
    
}

int main() {
    std::cout << "=============== ACTIVIDAD 2 ===============\n";
    actividad2();
    std::cout << "\n=============== ACTIVIDAD 3 ===============\n";
    actividad3();
    return 0;
}
