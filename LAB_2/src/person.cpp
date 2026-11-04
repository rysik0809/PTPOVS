#include "person.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <regex>

namespace 
{

std::size_t utf8CharLen(unsigned char c) 
{
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

bool isRussianLetter(unsigned char b0, unsigned char b1) 
{
    if (b0 == 0xD0 && b1 == 0x81) return true;                
    if (b0 == 0xD0 && b1 >= 0x90 && b1 <= 0xBF) return true;  
    if (b0 == 0xD1 && b1 >= 0x80 && b1 <= 0x8F) return true;  
    if (b0 == 0xD1 && b1 == 0x91) return true;                
    return false;
}

void cyrillicToUpper(std::string& s) 
{
    if (s.size() != 2) return;

    unsigned char b0 = static_cast<unsigned char>(s[0]);
    unsigned char b1 = static_cast<unsigned char>(s[1]);
    if (b0 == 0xD0 && b1 >= 0xB0 && b1 <= 0xBF) 
    {
        s[1] = static_cast<char>(b1 - 0x20);
    } else if (b0 == 0xD1 && b1 >= 0x80 && b1 <= 0x8F) {
        s[0] = static_cast<char>(0xD0);
        s[1] = static_cast<char>(b1 + 0x20);
    } else if (b0 == 0xD1 && b1 == 0x91) {
        s[0] = static_cast<char>(0xD0);
        s[1] = static_cast<char>(0x81);
    }
}

void cyrillicToLower(std::string& s) 
{
    if (s.size() != 2) return;

    unsigned char b0 = static_cast<unsigned char>(s[0]);
    unsigned char b1 = static_cast<unsigned char>(s[1]);
    if (b0 == 0xD0 && b1 >= 0x90 && b1 <= 0x9F) 
    {
        s[1] = static_cast<char>(b1 + 0x20);
    } else if (b0 == 0xD0 && b1 >= 0xA0 && b1 <= 0xAF) {
        s[0] = static_cast<char>(0xD1);
        s[1] = static_cast<char>(b1 - 0x20);
    } else if (b0 == 0xD0 && b1 == 0x81) {
        s[0] = static_cast<char>(0xD1);
        s[1] = static_cast<char>(0x91);
    }
}

} // namespace

std::string normalizeNamePart(const std::string& part) 
{
    std::string result;
    std::size_t i = 0;
    bool capitalizeNext = true;

    while (i < part.size()) 
    {
        unsigned char c = static_cast<unsigned char>(part[i]);
        std::size_t len = utf8CharLen(c);
        if (i + len > part.size())
            break;

        std::string ch = part.substr(i, len);
        bool keep     = false;
        bool isLetter = false;

        if (len == 1) 
        {
            if (std::isalpha(c)) 
            {
                keep = true;
                isLetter = true;
            } else if (c == '-') {
                keep = true;
            }
        } else if (len == 2) {
            unsigned char b0 = static_cast<unsigned char>(ch[0]);
            unsigned char b1 = static_cast<unsigned char>(ch[1]);
            if (isRussianLetter(b0, b1)) 
            {
                keep = true;
                isLetter = true;
            }
        }

        if (keep) {
            if (isLetter) {
                if (capitalizeNext) {
                    if (len == 1 && std::islower(c)) {
                        ch[0] = static_cast<char>(std::toupper(c));
                    } else if (len == 2) {
                        cyrillicToUpper(ch);
                    }
                    capitalizeNext = false;
                } else {
                    if (len == 1 && std::isupper(c)) {
                        ch[0] = static_cast<char>(std::tolower(c));
                    } else if (len == 2) {
                        cyrillicToLower(ch);
                    }
                }
            } else if (len == 1 && c == '-') {
                capitalizeNext = true;
            }
            result += ch;
        }

        i += len;
    }

    return result;
}

std::string normalizeBirthDate(const std::string& input) 
{
    static const std::regex dateRe(R"((\d{1,4})\D+(\d{1,2})\D+(\d{1,4}))");
    std::smatch m;
    if (!std::regex_search(input, m, dateRe))
        return "";

    std::string p1 = m[1], p2 = m[2], p3 = m[3];
    int year, month, day;

    try 
    {
        if (p1.size() >= 4) 
        {
            year  = std::stoi(p1);
            month = std::stoi(p2);
            day   = std::stoi(p3);
        } else {
            day   = std::stoi(p1);
            month = std::stoi(p2);
            year  = std::stoi(p3);
        }
    } catch (...) {
        return "";
    }

    if (year < 1900 || year > 2100 || month < 1 || month > 12 ||
        day < 1 || day > 31)
        return "";

    char buf[11];
    std::snprintf(buf, sizeof(buf), "%02d.%02d.%04d", day, month, year);
    return std::string(buf);
}

std::string normalizePhone(const std::string& input) 
{
    std::string digits;
    for (char c : input) 
    {
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
    }
    if (digits.size() == 11 && digits[0] == '8')
        digits[0] = '7';
    if (digits.size() == 10)
        digits = "7" + digits;
    if (digits.size() != 11 || digits[0] != '7')
        return "";
    return "+" + digits;
}

std::string normalizeEmail(const std::string& input) 
{
    std::string s = input;
    s.erase(std::remove(s.begin(), s.end(), '<'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '>'), s.end());
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    static const std::regex emailRe(
        R"(^[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$)");
    if (!std::regex_match(s, emailRe))
        return "";
    return "<" + s + ">";
}

Person inputPerson() 
{
    Person p;
    std::cout << "Введите фамилию: ";
    std::getline(std::cin, p.surname);
    std::cout << "Введите имя: ";
    std::getline(std::cin, p.name);
    std::cout << "Введите отчество: ";
    std::getline(std::cin, p.patronymic);
    std::cout << "Введите дату рождения (например, 2002-03-01): ";
    std::getline(std::cin, p.birthDate);
    std::cout << "Введите телефон (например, +7996-8432-431): ";
    std::getline(std::cin, p.phone);
    std::cout << "Введите email (например, sidorovVV@pochta.ru): ";
    std::getline(std::cin, p.email);
    return p;
}

bool validateAndNormalize(Person& p) 
{
    p.surname    = normalizeNamePart(p.surname);
    p.name       = normalizeNamePart(p.name);
    p.patronymic = normalizeNamePart(p.patronymic);
    p.birthDate  = normalizeBirthDate(p.birthDate);
    p.phone      = normalizePhone(p.phone);
    p.email      = normalizeEmail(p.email);

    if (p.surname.empty() || p.name.empty() || p.patronymic.empty()) 
    {
        std::cerr << "Ошибка: ФИО содержит недопустимые символы.\n";
        return false;
    }
    if (p.birthDate.empty()) 
    {
        std::cerr << "Ошибка: некорректная дата рождения.\n";
        return false;
    }
    if (p.phone.empty()) 
    {
        std::cerr << "Ошибка: некорректный телефон.\n";
        return false;
    }
    if (p.email.empty()) 
    {
        std::cerr << "Ошибка: некорректный email.\n";
        return false;
    }
    return true;
}

void printPerson(const Person& p) 
{
    std::cout << p.surname << " " << p.name << " " << p.patronymic << " "
              << p.birthDate << " " << p.phone << " " << p.email << "\n";
}

void writePerson(std::ostream& os,
                 const Person& raw,
                 const Person& normalized,
                 bool valid) 
{
    os << "=== Анкета ===\n\n";

    os << "Исходные данные:\n";
    os << "  Фамилия:        " << raw.surname << "\n";
    os << "  Имя:            " << raw.name << "\n";
    os << "  Отчество:       " << raw.patronymic << "\n";
    os << "  Дата рождения:  " << raw.birthDate << "\n";
    os << "  Телефон:        " << raw.phone << "\n";
    os << "  Email:          " << raw.email << "\n\n";

    os << "Нормализованные данные:\n";
    os << "  Фамилия:        " << normalized.surname << "\n";
    os << "  Имя:            " << normalized.name << "\n";
    os << "  Отчество:       " << normalized.patronymic << "\n";
    os << "  Дата рождения:  " << normalized.birthDate << "\n";
    os << "  Телефон:        " << normalized.phone << "\n";
    os << "  Email:          " << normalized.email << "\n\n";

    os << "Итоговая строка:\n";
    os << "  " << normalized.surname << " " << normalized.name << " "
       << normalized.patronymic << " " << normalized.birthDate << " "
       << normalized.phone << " " << normalized.email << "\n\n";

    os << "Статус: " << (valid ? "OK" : "ОШИБКА ВАЛИДАЦИИ") << "\n";
}