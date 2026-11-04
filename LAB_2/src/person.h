#pragma once

#include <ostream>
#include <string>

// ======================= АНКЕТА ==============================================

struct Person {
    std::string surname;
    std::string name;
    std::string patronymic;
    std::string birthDate;   // DD.MM.YYYY
    std::string phone;       // +7XXXXXXXXXX
    std::string email;       // <local@domain>
};

// Приводит одну часть ФИО к виду: первая буква каждого «слова» заглавная,
// остальные строчные; небуквенные символы удаляются.
// Корректно обрабатывает кириллицу в UTF-8.
std::string normalizeNamePart(const std::string& part);

std::string normalizeBirthDate(const std::string& input);
std::string normalizePhone(const std::string& input);
std::string normalizeEmail(const std::string& input);

Person inputPerson();

// Нормализует все поля «на месте». Возвращает true, если все поля
// непустые после нормализации. Если поле не прошло проверку, оно
// становится пустой строкой (это видно в anketa.txt).
bool validateAndNormalize(Person& p);

void printPerson(const Person& p);

// Сохранить анкету в поток: и исходные данные, и нормализованные,
// и итоговую строку результата.
void writePerson(std::ostream& os,
                 const Person& raw,
                 const Person& normalized,
                 bool valid);