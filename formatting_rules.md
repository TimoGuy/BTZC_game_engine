# Some formatting rules for a future `.clang-fromat` or my own formatting tool.

## Basics

- Cutoff is at col 100
- `#define` macro definitions are left alone.
- /* */ comments are left alone.


## Uncategorized
```cpp
//- List_type list{123, 1, 5};
    List_type list{ 123, 1, 5 };
// ^^@NOTE^^: Curly brace declarations can have really wild things going on with spacing and such,
//   but as long as there's (1) a space/end-or-start-of-line after '{' and before '}', (2) does not
//   overflow the col 100 rule, (3) all ',' have a space/end-or-start-of-line afer, then the
//   formatting is up to the user (i.e. the formatter doesn't change anything).

//- static std::vector<vec3s> const k_end_cap_ab_y{
//-     {  1,          0,  0          },
//-     {  0.92387953, 0,  0.38268343 },
//-     {  0.70710678, 0,  0.70710678 },
//-     {  0.38268343, 0,  0.92387953 },
//-     {  0,          0,  1          },
//-     { -0.38268343, 0,  0.92387953 },
//-     { -0.70710678, 0,  0.70710678 },
//-     { -0.92387953, 0,  0.38268343 },
//-     { -1,          0,  0          },
//-     { -0.92387953, 0, -0.38268343 },
//-     { -0.70710678, 0, -0.70710678 },
//-     { -0.38268343, 0, -0.92387953 },
//-     {  0,          0, -1          },
//-     {  0.38268343, 0, -0.92387953 },
//-     {  0.70710678, 0, -0.70710678 },
//-     {  0.92387953, 0, -0.38268343 },
//-     {  1,          0,  0          },
//- };
    static std::vector<vec3s> const k_end_cap_ab_y{
        {  1,          0,  0          },
        {  0.92387953, 0,  0.38268343 },
        {  0.70710678, 0,  0.70710678 },
        {  0.38268343, 0,  0.92387953 },
        {  0,          0,  1          },
        { -0.38268343, 0,  0.92387953 },
        { -0.70710678, 0,  0.70710678 },
        { -0.92387953, 0,  0.38268343 },
        { -1,          0,  0          },
        { -0.92387953, 0, -0.38268343 },
        { -0.70710678, 0, -0.70710678 },
        { -0.38268343, 0, -0.92387953 },
        {  0,          0, -1          },
        {  0.38268343, 0, -0.92387953 },
        {  0.70710678, 0, -0.70710678 },
        {  0.92387953, 0, -0.38268343 },
        {  1,          0,  0          },
    };
// ^^@NOTE^^: Left alone. (This also puts burden on the user to format these, but if you want it
//   auto formatted, then just make a mistake in the formatting and it'll get fixed)

//- List_type list{ { 69, 0, 0 }, { 4, 5, 1 } };
    List_type list{ { 69, 0, 0 }, { 4, 5, 1 } };

//- List_type list{{ 69, 0, 0 }, { 4, 5, 1 } };
    List_type list{
        { 69, 0, 0 },
        { 4, 5, 1 }
    };

// vv@NOTEvv: Use context clues for formatting lists too.
//- List_type2 list{ Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4 } };
    List_type2 list{
        Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4 }
    };

//- List_type2 list{
//-     Koko_data_type{
//-         "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4 }
//- };
    List_type2 list{
        Koko_data_type{
            "hello_there",
            "and you are?",
            "Oh I wonder how you got here",
            1,
            2,
            3,
            4
        }
    };

// vv@NOTEvv: Overflow inner data will cause stacked list tho.
//- List_type2 list{
//-     Large_params_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4, 123, 145, "And this is a very very long string I think would be fun" }
//- };
    List_type2 list{
        Large_params_data_type{
            "hello_there",
            "and you are?",
            "Oh I wonder how you got here",
            1,
            2,
            3,
            4,
            123,
            145,
            "And this is a very very long string I think would be fun"
        }
    };

// vv@NOTEvv: Leave strings alone tho. Of course break down the list initialization as much as
//   possible tho of course.
//- List_type2 list{
//-     Large_params_data_type{
//-         "hello_there",
//-         "and you are?",
//-         "Oh I wonder how you got here",
//-         1,
//-         2,
//-         3,
//-         4,
//-         123,
//-         145,
//-         "And this is a very very long string I think would be fun. Okay now I'm making this string so much longer that it overflows!"
//-     }
//- };
    List_type2 list{
        Large_params_data_type{
            "hello_there",
            "and you are?",
            "Oh I wonder how you got here",
            1,
            2,
            3,
            4,
            123,
            145,
            "And this is a very very long string I think would be fun. Okay now I'm making this string so much longer that it overflows!"
        }
    };
// ^^@NOTE^^: Stays the same. (Breaking strings will be users job)



// vv@NOTEvv: Same principle with comments after lines. But, don't break down stuff for the comment.
//   If the first non-whitespace char of the comment starts >=100, then ignore everything for that
//   comment.
//- List_type2 list{
//-     Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2 }  // This is a comment.
//- };
    List_type2 list{
        Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2 }  // This
                                                                                               // is
                                                                                               // a
                                                                                               // comment.
    };
//- List_type2 list{
//-     Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here" }  // This is a comment.
//- };
    List_type2 list{
        Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here" }  // This is
                                                                                         // a
                                                                                         // comment.
    };

//- List_type2 list{
//-     Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4 }  // This is a comment.
//- };
    List_type2 list{
        Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 2, 3, 4 }  // This is a comment.
    };

//- List_type2 list{
//-     Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 223 }  // This is a comment.
//- };
    List_type2 list{
        Koko_data_type{ "hello_there", "and you are?", "Oh I wonder how you got here", 1, 223 }  // This is a comment.
    };

```


## Function declarations

```cpp
// Jojo.h
class Jojo
{
public:
//- Jojo(Array&& an_array) : m_my_num{ 6 }, m_my_array(std::move(an_array)) {}
    Jojo(Array&& an_array)
        : m_my_num{ 6 }
        , m_my_array(std::move(some_array))
    {
    }

//- ~Jojo() = default;
//- Jojo(const Jojo& other) = default;
//- Jojo(Jojo&& other) noexcept = default;
//- Jojo& operator=(const Jojo& other) = default;
//- Jojo& operator=(Jojo&& other) noexcept = default;
    ~Jojo()                                = default;
    Jojo(const Jojo& other)                = default;
    Jojo(Jojo&& other) noexcept            = default;
    Jojo& operator=(const Jojo& other)     = default;
    Jojo& operator=(Jojo&& other) noexcept = default;

//- void my_short_func();
    void my_short_func();

//- void my_short_defined_func0() {}
    void my_short_defined_func0()
    {
    }

//- void my_short_defined_func1() { m_jojo++; }
    void my_short_defined_func1()
    {
        m_jojo++;
    }

//- void my_short_defined_func2() { m_jojo++; m_koko--; }
    void my_short_defined_func2()
    {
        m_jojo++;
        m_koko--;
    }

//- void my_func_w_a_lot_of_params(uint8_t jojo1, char jojo2, int32_t jojo3, size_t jojo5, Some_random_type& out_jojo6);
    void my_func_w_a_lot_of_params(uint8_t jojo1,
                                   char jojo2,
                                   int32_t jojo3,
                                   size_t jojo5,
                                   Some_random_type& out_jojo6);

//- Hawsoo::Some_third_party_namespace_that_idk::Some_data_type my_looooooooooooooooooooooooong_return_type_func();
    Hawsoo::Some_third_party_namespace_that_idk::Some_data_type
    my_looooooooooooooooooooooooong_return_type_func();

//- void my_looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_func(float_t digger, int32_t lock_in_rate) const override;
    void my_looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_func(
        float_t digger,
        int32_t lock_in_rate) const override;
};
```
