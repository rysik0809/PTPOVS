#include "file_io.hpp"
#include "word_counter.hpp"
#include <iostream>
#include <fstream>

int main() {

    // 2. Читаем его через наш класс
    try {
        file_io::fileBuffer file("voina_i_mir.txt");

        std::cout << "Размер: " << file.size() << " байт\n";

        wordCounter w;

        std::cout << w.count(file.view(), "Она") << '\n';
        
    } catch (const std::exception& e) {
        std::cerr << "Исключение: " << e.what() << '\n';
        return 1;
    }

}