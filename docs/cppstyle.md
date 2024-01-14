# C++ Style Guide
_Adapted from [JUCE](https://juce.com/discover/stories/coding-standards). Please try to follow these guidelines as much as possible._

If you're writing Lua code, please also see the [Lua Style Guide](lua-style.md), in addition to this document.

## Don't Repeat Yourself!
This principle pretty much summarises the essence of what it means to write good code, in all languages, at all levels. Other people have explained it better than we can do here – google for D.R.Y. and you’ll find many tutorials. If you only pay attention to one piece of advice in this document, make it this one!

## Layout & Whitespace
The following rules won't make any difference at all to what the code actually does, but aesthetics and consistency are really important in making your code understandable.

* No tab characters!
* Tabs are 4 spaces!
* Braces are indented in the [Allman style](http://en.wikipedia.org/wiki/Indent_style#Allman_style).

```c++
if (x == 0) {      // No!
    foobar();
    return "zero";
}

if (x == 0)        // Yes!
{
    foobar();
    return "zero";
}
```
* Always put a space before and after all binary operators! (except when declaring an operator, e.g.
```c++
 bool operator== (Foo))
x = 1+y - 2*z / 3;     // Bad
x = 1 + y - 2 * z / 3; // Good
```
* The ! operator should always be followed by a space, e.g. if (! foo)
* The ~ operator should be preceded by a space, but * not followed by one.
* The ++ and -- operators should have no spaces between the operator and its operand.
* Never put a space before a comma.
* Always put a space after a comma.
```c++
foo (x,y);   // No
foo (x ,y);  // No
foo (x , y); // No
foo (x, y);  // Yes
```
* Always put a space before an open parenthesis that contains text - e.g. foo (123);
* Never put a space before an empty pair of open/close parenthesis - e.g. foo();
* Don’t put a space before an open square bracket when used as an array index - e.g. foo[1]
* Leave a blank line before if, for, while, do statements when they’re preceded by another statement. E.g.
```c++
{
   int xyz = 123;

   if (xyz != 0)
       foo();

   foobar();
}
```
* In general, leave a blank line after a closing brace } (unless the next line is just another close-brace)
* Do not write if statements all-on-one-line…
* ...unless! If you have a set of consecutive if statements which are similar, and by aligning them up vertically it makes it clear to see the pattern of similarities and differences, e.g.
```c++
if (x == 1) return "one";
if (x == 2) return "two";
if (x == 3) return "three";
```
* Some people mandate using braces around if-statements, even very short, simple ones. Our rule is to omit them for trivially simple one-line statements where braces would add visual clutter without making things any clearer. However, if there are multiple lines, or if the expressions involved are very long, do add braces.
* In an if-else statement where there is more than one branch, all branches should be formatted the same way, i.e. either all of them use braces, or none of them use braces.
* Lambdas: here are examples of the preferred style:
```c++
auto myLambda = []()  { return 123; };

auto myLambda = [this, &x] (int z) -> float  { return x + z; };

auto longerLambda = [] (int x, int y) -> int
{
    // ...multiple lines of stuff...
};
```
* When writing a pointer or reference type, we always put a space after it, and never before it. e.g.
```c++
SomeObject* myObject = getAPointer();
SomeObject& myObject = getAReference();
```
* Yes - we know that many people would argue that a more correct technically correct layout for such a declaration would be:
```c++
SomeObject *myObject = getAPointer();
SomeObject &myObject = getAReference();
```
* But we think it makes more sense for the asterisk to stick to the type name, because the pointer-ness is a quality that belongs to the type, not to the variable. The only time that this can lead to any confusion is when you're declaring multiple pointers of the same type in the same statement – which leads on to the next rule…
* When declaring multiple pointers or references of the same type that involve the asterisk or ampersand character, never do so in a single statement, e.g. 
```c++
SomeObject* p1, *p2;
```
* instead split them out onto separate lines and write the type name again, to make it quite clear what's going on, and avoid the danger of missing out any vital asterisks.
```c++
SomeObject* p1;
SomeObject* p2;
```
* Or better still, use a smart-pointer or typedef to create a type-name that doesn’t require the asterisk or ampersand!
* We put the ‘const’ modifier before the type name, i.e.
```c++
const Thing& t; // yes!
Thing const& t; // no!
```
* Both do the same job, but the former style is closer to the way you'd verbally describe the type in English, i.e. verb-before-noun order. There are arguments in favour of using a trailing const because it makes things easier in very complex type names, but if you have a type name that contains multiple levels of const modifier keywords, you should probably have used typedefs to simplify it into a more manageable typename anyway.
* Template parameters should follow their type without a space, e.g. vector<int>
* ...but in a template statement, we do leave a space before the open bracket, e.g.
template <typename Type1, typename Type2>
* When splitting expressions containing operators (or the dot operator) across multiple lines each new line should begin with the operator symbol, e.g.
```c++
auto xyz = foo + bar        // This makes it clear at a glance
            + func (123)    // that all the lines must be continuations of
            - def + 4321;   // the preceding ones.

auto xyz = foo + bar +      // Not so good.. It takes more effort here
           func (123) -     // to see that "func" here is actually part
           def + 4321;      // of the previous line and not a new statement.

// Good:
auto t = AffineTransform::translation (x, y)
                         .scaled (2.0f)
                         .rotated (0.5f);

// Bad:
auto t = AffineTransform::translation (x, y).
    scaled (2.0f).
   rotated (0.5f);
```
* Long lines... Many coding standards enforce a maximum line-length of something like 80 characters, and forbid any exceptions to this. Obviously keeping lines short and clear is good as a general rule - we don't want to make our readers use their horizontal scrollbar too often. However, there are many cases where having strict line length limits produces confused-looking indentation where long expressions have to be awkwardly broken across multiple lines, or nudged backwards by a few spaces just to keep the line-end below the limit. In these cases longer lines can vastly improve readability and help to highlight errors. For example where you have a series of similar-but-slightly-different statements which can be made to line-up vertically by putting each one onto a single line, it makes the vertical similarities jump out, and makes the overall pattern of what the code is doing visually obvious.
* For short comments on one or two lines, prefer to use // where possible, rather than /* */, as this makes it easier to comment-out large blocks of code when debugging.
* Always leave a space before the text in a single line // comment
```c++
// yes!
//no!
```
* Multi-line comments are aligned vertically on the left, e.g.
```c++
/* This is correct
*/

/** This is also correct
*/

/* This is also ok!
 */
```
* Hexadecimal is usually written in lower-case, e.g. 0xabcdef
* Floating point literals: We always add at least one digit before and after the dot, e.g.
```c++
0.0     // yes!
0.0f    // yes!
0.      // no!
0.f     // no!
.1      // no!
.1f     // no!
```
* String concatenation: Please avoid overusing the typename String! For some reason I seem to constantly see this kind of bloat in people’s code, but can’t understand the reason.. e.g.
```c++
String w = String ("World");       // Why say it twice!??
auto w = String ("World");         // This is OK, but still longer than needed
String w ("World");                // This is the most compact way to write it

auto b = String ("Hello ") + w;    // NO! This is a waste of typing!
auto b = "Hello " + w;             // This does exactly the same thing (given that w is a String, not a char*)
```
## Naming Conventions
* Variable and method names are written with camel-case, and always begin with a lower-case letter, e.g. myVariableName
* Class names are also written in camel-case, but always begin with a capital letter, e.g. MyClassName
* Hungarian notation is not allowed. The internet already contains lots of arguments for/against this style, so we won't add to that debate here. But the majority of other coding standards, including the standard library itself, have moved on from mixing the type and the name together. The type may change due to templates or use of ‘auto’, and the name should reflect the purpose of the variable, not its type.
* We tend to avoid using underscores inside variable names, unless it’s necessary to turn a long unreadable name into something more readable.
* Leading or trailing underscores are never allowed in variable names! Leading underscores have a special status for use in standard library code, so to use them in use code looks quite jarring.
* If you really have to write a macro, it must be `ALL_CAPS_WITH_UNDERSCORES`. As they’re the only symbols written in all-caps, this makes them easy to spot.
* Since macros have no namespaces, their names must be guaranteed not to clash with macros or symbols used in other libraries or 3rd party code, so you should start them with something unique to your project. All the Element macros begin with `EL_`.
* For enums, use camel-case with the same capitalisation that you'd use for a class and its member variables, e.g.
```c++
enum class MyEnum
{
    enumValue1 = 0,
    enumValue2 = 1
};
```
* When writing templates, avoid using T or other one-character template parameters. It doesn't take much effort to give them a helpful name, and T can clash with macros defined in some careless 3rd party headers.
*Types, const-correctness, etc
*If a method can (and should!) be const, make it const! Herb Sutter has an interesting take on the meaning of const with respect to the thread-safety of a class that's worth reading about here.
* If you override a virtual method in a sub-class, ALWAYS mark it with the override specifier, and NEVER give it a redundant virtual keyword!
* If a method definitely doesn't throw an exception (be careful!), mark it as noexcept. Try to do this everywhere possible as it can have a dramatic effect on performance in some situations - apparently up to 10x in extreme cases.
* When returning a temporary object, e.g. a String, the returned object should be non-const, so that if the class has a move operator, the compiler can use it.
* If you have a local variable or function parameter which is not going to be changed then you should consider making it const. But the factors to consider here are that adding const makes the code more verbose, and that may add unnecessary visual clutter if the constness is unimportant or obvious. Or it could be a positive thing if it’s useful for the reader’s attention to be drawn to the fact that this variable is constant. A general rule is that in very short block of code, don’t worry about making local variables const. In longer blocks where a variable is used many times, you may want to make it const if you think it’s a helpful tip to the reader. Note that In almost all cases declaring a primitive value as const makes no difference whatsoever to the compiler’s code generation.
* If something is a compile-time constant, then always declare it constexpr!
* Remember that pointers can be const as well as primitives – for example, if you have a char pointer whose contents are going to be altered, you may still be able to make the pointer itself const, e.g. char* const foobar = getFoobar();.
* Do not declare all your local variables at the top of a function or method (i.e. in the old-fashioned C-style). Declare them at the last possible moment, to keep their scope as tightly limited as possible.
* When you're testing a pointer to see if it's null, never write if (myPointer). Always avoid that implicit cast-to-bool by writing it more fully:
```c++
if (myPointer != nullptr)
```
* Likewise, never ever write if (! myPointer), instead always write
```
if (myPointer == nullptr)
```
* The reasoning here is that it’s more readable because it matches the way you’d read out the expression in English - i.e. "if the variable myPointer is null".
* Avoid C-style casts except when converting between obviously primitive numeric types. Some people would say "avoid C-style casts altogether", but writing static_casts can be a bit long-winded when you just want to trivially cast an int to a float. But whenever a pointer or template or non-primitive type is involved, always use modern casts. And when you're reinterpreting data, always use reinterpret_cast.
* For 64-bit integer types: in JUCE we declare juce::int64 (and other types) but this was added before C++11 introduced int64_t. We’d encourage using either of these in preference to long long.
* Object lifetime and ownership
* Absolutely do NOT use delete, deleteAndZero, etc. There are very very few situations where you can't use a smart pointer or some other automatic lifetime management class.
* Do not use new unless there's no alternative. Whenever you type new, always treat it as a failure to find a better solution. If a local variable can be allocated on the stack rather than the heap, then always do so.
* Never use new or malloc to allocate a C++ array. Always prefer a juce::HeapBlock or some other container class.
* ..and just to make it doubly clear: You should (almost) never need to use malloc or calloc at all!
* If a parent class needs to create and own some kind of child object, always use composition as your first choice. If that's not possible (e.g. if the child needs a pointer to the parent for its constructor), then use a ScopedPointer or std::unique_ptr. Whenever possible, pass an object as a reference rather than a pointer. If possible, make it a const reference.
* Obviously avoid static and global variables. Sometimes there's no alternative, but if there is an alternative, then use it, no matter how much effort it involves.
* If allocating a local POD structure (e.g. an operating-system structure in native code), and you need to initialise it with zeros, use the = {}; syntax as your first choice for doing this. If for some reason that's not appropriate, use the zerostruct() function, or in case that isn't suitable, use zeromem(). Avoid memset().
* Treat Component::deleteAllChildren() as a last resort – never use it if there's a cost-free alternative.
* The juce::ScopedPointer class was written to be compatible with pre-C++11 compilers, so although it does offer C++11 move functionality for supported compilers, it's not as versatile as std::unique_ptr. So if you can use std::unique_ptr in your own code, that's probably a better bet. We may eventually migrate the JUCE codebase to std::unique_ptr.
* When returning heap objects from functions, most of the JUCE codebase predates the modern C++ style of returning a std::unique_ptr to indicate that the caller takes ownership. We will at some point be moving to that style, but in the meantime, functions are annotated to make clear how ownership is passed around.
* Passing string parameters into functions
* Special topic for this one, as it’s a common question and something that people do frequently, and is a bit fiddly to explain. In its current form, juce::String is ref-counted, so passing them by value is cheap.. However, it’s not as cheap as passing them by reference. When you want to pass a String into a function, you do it in various ways:
```c++
    void foo (const String&);
    void foo (String);
    void foo (String&&);
    void foo (StringRef);
```
* In most cases, it really doesn’t matter which one you choose, as the performance implications are going to be irrelevant most of the time. But if you want to choose the optimal method, the rules are (roughly):
* If the function is going to store the string somewhere, e.g. by assigning it to another string, or if you intend to modify it inside the function, then pass it by value. If you do this you may also want to std::move out of the parameter rather than copying from it.
* If the function is going to store the string and things are really performance-critical, then the most optimal way is to have two versions of the function for either a const String& or a String&&. You’d only need to go to this much trouble in really extreme situations though!
* If the function is only going to read the string, and if you don’t need all methods that String provides, then prefer to pass it as a StringRef. That way a string literal can be passed in without the overhead of creating a String object at all.
* If the function is just going to read the string but you need more String methods than StringRef provides, then just pass it as a const String&.

## Classes
* Declare a class's public section first, and put its constructors and destructor first. Any protected items come next, and then private ones. The order of items should roughly correspond to how likely a reader is to be interested in them, so the private implementation details come last.
* For simple, short inline classes, especially ones inside a cpp file that are only used locally for a limited purpose, they should probably use struct rather than class since you’re generally going to have all public members, and this will save a line of code doing the initial public:
* The layout for inherited classes is to put them to the right of the class name, vertically aligned. We leave a single space before the class name, and 2-3 spaces after the class name before the colon, e.g.
```c++
class Thing  : public Foo,
               private Bar
{
```
* Always include the public/private/protected keyword for each inherited class
* Put a class's member variables (which should almost always be private, of course), after all the public and protected method declarations.
* Any private methods should go towards the end of the class, after the member variables.
* If your class does not have copy-by-value semantics, you may want to use the JUCE_DECLARE_NON_COPYABLE macro. This should be the last item in your class's declaration. Or you can use the Foo (const Foo&) = delete; syntax alongside your other constructors if that works better.
* If your class is likely to be leaked, then you can use the JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR or JUCE_LEAK_DETECTOR macro.
* Constructors that take a single parameter should usually be marked explicit. Obviously there are cases where you do want implicit conversion, but always think about it carefully before writing a non-explicit constructor.
* Do NOT use NULL, null, or 0 for a null-pointer! (And for god's sake don't be tempted to use 0L, which is the worst of the lot!) Always use nullptr!
* All the C++ 'guru' books and articles are full of excellent and detailed advice on when it's best to use inheritance vs composition. If you're not already familiar with the received wisdom in these matters, then do some reading!

## Miscellaneous
* Never put an else statement after a return! The LLVM coding standards give a good explanation of this, but once you think about it, it's basic common sense. Since I first noticed this one, it has become one of my pet-hates when I see it done in other people's code!
```c++
if (foobar())
    return doSomething();
else                      // Never!
    doSomethingElse();

if (foobar())
    return doSomething();

doSomethingElse();        // Better!
```
* Early versions of JUCE used a T() macro to wrap string literals, but that has been deprecated for many years now. Just write your strings as plain old C++ string literals and the JUCE String and StringRef classes will deal with them.
* For extended unicode characters, the only fully cross-compiler way to embed these into your code is as a UTF-8 encoded C++ string written as escape character sequences (this means that the source file is still pure ascii, so text editors can't mangle the encoding). If you do that, you should wrap the literal in the CharPointer_UTF8 class, so that when cast to a String, everything's nice and clear about the format that's being used. The Projucer has a built-in tool that will convert unicode strings to valid C++ code and handle all this for you.
* Don't use macros! OK, obviously there are many situations where they're the right (or only) tool for the job, but treat them as a last resort. Certainly don't ever use a macro just to hold a constant value or to perform any kind of function that could have been done as a real inline function. And it goes without saying that you should give them names which aren't going to clash with other code. And #undef them after you've used them, if possible.
* When using the ++ or -- operators, never use post-increment if pre-increment could be used instead. Although it doesn't matter for primitive types, it's good practice to pre-increment since this can be much more efficient for more complex objects. In particular, if you're writing a for loop, always use pre-increment, e.g. for (int = 0; i < 10; ++i)
* When getting a possibly-null pointer and using it only if it's non-null, limit the scope of the pointer as much as possible – e.g. Do NOT do this:
```c++
auto* f = getFoo();

if (f != nullptr)
    f->doSomething();

// ...lots of intervening code...

f->doSomething(); // oops! f may be null!
```
* ..instead, always prefer to write it like this, which reduces the scope of the pointer, making it impossible to write code that accidentally uses a null pointer:
```c++
if (auto* f = getFoo())
    f->doSomething();

// f is out-of-scope here, so a null-pointer dereference is impossible.
```
* (This also results in cleaner, more compact code).
When passing small, POD objects into functions, you should always pass them by value, not by reference. Most experienced C++ coders, myself included, fall into the habit of always passing function parameters as const-references, e.g. const Foo&. This is usually the right thing to do for complex objects (e.g. Array, String, etc), but when you pass a reference, it prevents the compiler from using a whole slew of optimisation techniques on the call-site. For example, it means that often there's no way for the compiler to really know whether the function will modify the original value (via const_cast) or whether it will modify a memory address which is an offset from the location of the object. So, the best-practice advice from the guys who write the optimisers is: Always stick to pass-by-value if possible, and only use references if the price of calling the copy constructor is very high. This is particularly true in the case of small objects whose overall size is actually not much bigger than the size of a pointer. Some juce classes which should always be passed by value include: Point, Time, RelativeTime, Colour, all of the CharPointer_XYZ classes, Identifier, ModifierKeys, JustificationType, Range, PixelRGB, PixelARGB, Rectangle.
* We always prefer the std library versions of functions that have old C equivalents. So we never use fabs, sqrtf, powf etc, instead always use the polymorphic functions std::abs, std::sqrt, std::sin, std::cos, std::pow, etc.
* Most of the juce container classes use int as their index type, which differs from the standard library’s use of size_t, but it seems that over the years, the C++ standards committee have gradually come to feel that using an unsigned type was a mistake. However, the mismatch does make it a bit annoying to have to convert index values when interoperating with the STL. (But since a lot of loop iteration now happens with range-based-for, this isn’t quite so bad any more).
* We never use unsigned as a type on its own - always write unsigned int if that’s what you mean. The adjective on its own just seems like an unfinished sentence.
* JUCE has always defined a set of basic types int8, uint8, int16, uint16, int32, uint32, int64, uint64 - these are what we suggest using when a specific bit size is needed. Since the standard library has introduced std::uint32_t etc we also sometimes use those, but the juce ones are slightly shorter and have never caused problems with name clashes.
* Always prefer a range-based for-loop to iterate containers instead of a raw for-loop! Using STL algorithms and iterators is great, but we try not to use them religiously as in many simple cases a more straightforward approach ends up being more readable and more easily debugged.
* We do like the "almost-always-auto" style but there are some cases where it’s better to avoid:
```c++
    auto x = 0.0f; // OK: obviously a float
    auto x = 0.0;  // OK: obviously a double
    auto x = 0;    // Not OK! Nothing makes it obvious that by '0' you mean a signed
                   // int rather than an unsigned int, long, int64 etc.

    for (int i = 0; i < someNumber; ++i)  // OK: clear that you mean a signed int

    bool someCondition = false;           // OK: clearer than using auto

    auto someResult = thisReturnsABool(); // Use auto if the RHS is an expression
```
