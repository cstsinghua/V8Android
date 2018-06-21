//
// Created by boyaa on 2018/6/5.
//

#ifndef SUNGAMEENGINE_PERSION_H
#define SUNGAMEENGINE_PERSION_H


#include <cstring>
#include <string>
#include <cstring>


class Person {
    private:
        int age;
        char const *name;

    public:
        char const * gender = "nan";

        Person( int age, char const *name) {
            this->age = age;
//            strncpy(this->name, name, sizeof(this->name));
            this->name = name;
        }

        int getAge() {
            return this->age;
        }

        void setAge( int age) {
            this->age = age;
        }

        char const *getName() {
            return this->name;
        }

        void setName(char const *name) {
//            strncpy(this->name, name, sizeof(this->name));
            this->name = name;
        }

};

#endif //SUNGAMEENGINE_PERSION_H
