#pragma once

template<typename T>
class Manager
{
protected:
    Manager() = default;

public:
    Manager( const Manager& ) = delete;
    Manager( Manager&& ) = delete;

    virtual ~Manager() = default;

    Manager& operator=( const Manager& ) = delete;
    Manager& operator=( Manager&& ) = delete;

    static T& it()
    {
        static T instance;
        return instance;
    }
};