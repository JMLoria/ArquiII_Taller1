# Taller 1 - OpenMP y Threads

Este proyecto compara dos APIs de programación paralela para memoria compartida, OpenMP y std::thread

## Limpiar el resultados.csv

Antes de ejecutar los experimentos, limpia el archivo 'resultados.csv'

'''bash
> resultados.csv

## Compilar los programas
g++ -O3 -std=c++17 -fopenmp taller1_openmp.cpp -o taller1_openmp
g++ -O3 -std=c++17 -pthread taller1_threads.cpp -o taller1_threads

## Ejecutar varias veces y guardar resultados
> resultados.csv
for t in 1 2 4 8; do
    ./taller1_openmp --n 1000000 --bins 256 --threads $t --variant a --csv >> resultados.csv
    ./taller1_openmp --n 1000000 --bins 256 --threads $t --variant b --csv >> resultados.csv
    ./taller1_openmp --n 1000000 --bins 256 --threads $t --variant c --csv >> resultados.csv
    
    ./taller1_threads --n 1000000 --bins 256 --threads $t --variant a --csv >> resultados.csv
    ./taller1_threads --n 1000000 --bins 256 --threads $t --variant b --csv >> resultados.csv
    ./taller1_threads --n 1000000 --bins 256 --threads $t --variant c --csv >> resultados.csv
done

## Generar graficas 
python3 plot_results.py resultados.csv