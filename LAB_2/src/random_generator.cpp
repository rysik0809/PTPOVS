#include "random_generator.h"

#include <cstdio>

RandomPersonGenerator::RandomPersonGenerator()
    : rng(std::random_device{}()),
      surnames{
          "Иванов", "Петров", "Сидоров", "Смирнов", "Кузнецов",
          "Попов", "Васильев", "Соколов", "Михайлов", "Новиков",
          "Фёдоров", "Морозов", "Волков", "Алексеев", "Лебедев"
      },
      names{
          "Иван", "Пётр", "Сергей", "Алексей", "Дмитрий",
          "Андрей", "Михаил", "Николай", "Владимир", "Егор"
      },
      patronymics{
          "Иванович", "Петрович", "Сергеевич", "Алексеевич", "Дмитриевич",
          "Андреевич", "Михайлович", "Николаевич", "Владимирович", "Егорович"
      }
{
}

Person RandomPersonGenerator::generate() {
    Person p;
    p.surname    = pick(surnames);
    p.name       = pick(names);
    p.patronymic = pick(patronymics);

    std::uniform_int_distribution<int> yearDist(1950, 2005);
    std::uniform_int_distribution<int> monthDist(1, 12);
    std::uniform_int_distribution<int> dayDist(1, 28);
    char dateBuf[11];
    std::snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%04d",
                  dayDist(rng), monthDist(rng), yearDist(rng));
    p.birthDate = dateBuf;

    // +79XXXXXXXXX — типичный российский мобильный
    std::uniform_int_distribution<int> digitDist(0, 9);
    std::string phone = "+79";
    for (int i = 0; i < 9; ++i)
        phone += std::to_string(digitDist(rng));
    p.phone = phone;

    std::string local;
    std::uniform_int_distribution<int> letterDist(0, 25);
    for (int i = 0; i < 6; ++i)
        local += static_cast<char>('a' + letterDist(rng));
    p.email = "<" + local + "@example.com>";

    return p;
}