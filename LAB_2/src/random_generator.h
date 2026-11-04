#pragma once

#include "person.h"

#include <random>
#include <string>
#include <vector>

/**
 * @brief Генератор случайных данных анкеты для тестирования валидации.
 */
class RandomPersonGenerator {
public:
    /**
     * @brief Создаёт генератор с недетерминированным начальным состоянием.
     */
    RandomPersonGenerator();

    /**
     * @brief Генерирует одну анкету со случайными значениями.
     *
     * ФИО берётся из словарей (равномерно), дата — год 1950–2005,
     * месяц 1–12, день 1–28 (без 29–31). Телефон — "+79" + 9 случайных цифр (11 цифр,
     * валиден для normalizePhone). Email — 6 случайных латинских букв
     * + "@example.com" в угловых скобках.
     *
     * @return Person с заполненными полями в «сыром» виде; все поля
     *         заведомо проходят validateAndNormalize.
     */
    Person generate();

private:
    std::mt19937 rng;  ///< Псевдослучайный генератор Mersenne Twister (32-бит).

    std::vector<std::string> surnames;     ///< Словарь фамилий.
    std::vector<std::string> names;        ///< Словарь имён.
    std::vector<std::string> patronymics;  ///< Словарь отчеств.

    /**
     * @brief Возвращает случайный элемент вектора.
     *
     * @param vec Непустой вектор.
     * @return const-ссылка на выбранный элемент.
     */
    template <typename T>
    const T& pick(const std::vector<T>& vec) {
        std::uniform_int_distribution<std::size_t> dist(0, vec.size() - 1);
        return vec[dist(rng)];
    }
};