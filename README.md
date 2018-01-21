## ![](cixl.png?raw=true) <a href="https://liberapay.com/basic-gongfu/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a>
#### Cixl - a minimal, decently typed scripting language

This project aims to produce a minimal, decently typed scripting language for embedding in and extending from C. The language is implemented as a straight forward 3-stage (parse/compile/eval) interpreter that is designed to be as fast as possible without compromising on simplicity, transparency and flexibility. The codebase has no external dependencies and is currently hovering around 7 kloc including tests and standard library.

### Getting Started
To get started, you'll need a reasonably modern C compiler with GNU-extensions and CMake installed. Building on macOS unfortunately doesn't work yet, as it's missing support for POSIX memory streams and timers. Most Linuxes and BSDs should be fine, I don't know enough about Windows to have a clue. A basic REPL is included, it's highly recommended to run it through ```rlwrap``` for a less nerve wrecking experience.

```
git clone https://github.com/basic-gongfu/cixl.git
cd cixl
mkdir build
cd build
cmake ..
make
rlwrap ./cixl

Cixl v0.8.5, 18765 bmips

Press Return twice to evaluate.

   1 2 3
...
[1 2 3]

   quit
```

### Status
Examples should work in the most recent version and run clean in ```valgrind```, outside of that I can't really promise much at the moment. Current work is focused on profiling and filling obvious gaps in functionality.

### Stack
The stack is accessible from user code, just like in Forth. Basic stack operations have dedicated operators; ```%``` for copying the last value, ```_``` for dropping it, ```~``` for flipping the last two values and ```|``` for clearing the stack.

```
   | 1 2 3 %
...
[1 2 3 3]

   _
...
[1 2 3]

   ~
...
[1 3 2]

   |
...
[]
```

### Expressions
But unlike Forth, functions scan forward until enough arguments are on the stack to allow reordering parameters and operations in user code to fit the problem being solved.

```
   | 1 + 2
...
[3]

   1 2 +
...
[3 3]

   + 1 2
...
[6 1 2]

   + +
...
[9]
```

The ```,``` operator may be used to cut the stack into discrete pieces and force functions to scan forward.

```
   | 1 + 2
...3 + 4
...
[6 4]

   1 + 2,
...3 + 4
...
[3 7]
```

While ```$``` may be used to pull values across cuts.

```
   1 2,
...
[1 2]

   $ + 3
...
[1 5]
```

### Variables
Named variables may be defined once per scope using the ```let:``` macro.

```
   | let: foo 'bar';
...$foo
...
['bar'@2]

   let: foo 'baz';
...
Error in row 1, col 10:
Attempt to rebind variable: 'foo'
```

Multiple names may be bound at the same time by enclosing them in parens.

```
   | let: (x y z) 1 2, 3 + 4;
...$x $y $z
...
[1 2 7]
```

Types may be specified for documentation and type checking.

```
   | let: (x y Int z Str) 1 2 3;
...$x $y $z
...
Error in row 1, col 5:
Expected type Str, actual: Int
```

Variables pull values from the current stack, which means that the same construct may be used to name existing values.

```
   | 1 2 3
...let: (x y z);
...$z $y $x
[3 2 1]
```

The same functionality may be accessed symbolically.

```
   | get-var `foo
...
[#nil]

   | let: foo 42;
...get-var `foo
...
[42]
```

Bindings in the current scope may be explicitly removed.

```
   | let: x 42;
...delete-var `x
...let: x 'foo';
...$x
...
['foo'@2]
```

### Equality
Two flavors of equality are provided.

Value equality:
```
   | [1 2 3] = [1 2 3]
...
[#t]
```

And identity:
```
   | 'foo' == 'foo'
...
[#f]
```

```
   | 42 == 42
...
[#t]
```

### Symbols
Symbols are immutable singleton strings that support fast equality checks.

```
   | `foo
...
[`foo]

   = `foo
...
[#t]

   | `foo = `bar
...
[#f]

   | 'baz' sym
...
[`baz]

   str
...
['baz'@1]
```

Runtime-unique symbols may be generated by calling ```new```.

```
   | new Sym, new Sym
...
[`S7 `S8]
```

### References
Some values are reference counted; strings, vectors, lambdas etc. Reference counted values display the number of references following ```@``` when printed. Doubling the copy operator results in a deep copy where applicable and defaults to regular copy where not.

```
   | [1 2 3] %
...
[[1 2 3]@2 [1 2 3]@2]

   | [1 2 3] %%
...
[[1 2 3]@1 [1 2 3]@1]
```

References may be created manually, which enables using reference semantics for value types among other things.

```
   | let: r #nil ref;
...$r
...
[Ref(#nil)@2]

   put-ref 42
...$r
...
[Ref(42)@2]

   get-ref
...
[42]
```

### Scopes
Code enclosed in parens is evaluated in a separate scope, the last value on the stack is automatically returned on scope exit.

```
   | (1 2 3)
...
[3]
```

Variables in the parent scope may be referenced from within, but variables defined inside are not visible from the outside.

```
   | let: foo 1;
...(let: foo 2; $foo)
...$foo
...
[2 1]
```

### Console
```say``` and ```ask``` may be used to perform basic console IO.

```
   | say 'Hello'  
...ask 'What\'s your name? '
...
Hello
What's your name? Sifoo
['Sifoo'@1]
```

Code may be loaded from file using ```load```, it is evaluated in the current scope.

test.cx:
```
+ 2
```

```
   | 1, load 'test.cx'
...
[3]
```

### Serialization
Most values support being written to files and read back in. Calling ```write``` on a value will write it's serialized representation to the specified stream.

```
   | now
...
[Time(2018/0/12 1:25:12.123436182)]

   , #out write $
...
([2018 0 12 1 25 12 123436182] time)
[]
```

While calling ```read``` will parse and evaluate one value at a time from the specified stream.

```
   | read #in
...
([2018 0 12 1 25 12 123436182] time)
[Time(2018/0/12 1:25:12.123436182)]
```

### Files

Files may be opened for reading/writing by calling ```fopen```, the type of the returned file depends on the specified mode. Valid modes are the same as in C, r/w/a(+). Files are closed automatically when the last reference is dropped.

```
   | fopen 'test.out' `a+
...
[RWFile(0x5361130)@1]

   now ~ write
...
[]
```

### Comments
Two kinds of code comments are supported, line comments and block comments.

```
   | 1 // Line comments terminate on line breaks
...+ 2
[3]

   | 1 /* While block comments may span
...multiple lines */
...+ 2
...
[3]
```

### Errors
Error handling is a work in progress, but two functions are provided for signalling errors. ```fail``` may be used to signal an error with specified error message.

```
   fail 'Going down!'
...
Error in row 1, col 6:
Going down!
```

While ```check``` may be used to signal an error when the specified condition doesn't hold.

```
   | 1 = 2 check
...
Error in row 1, col 7:
Check failed
```

### Lambdas
Putting braces around a block of code defines a lambda that is pushed on the stack.

```
   | {1 2 3}
...
[Lambda(0x52d97d0)@1]

   call
...
[1 2 3]
```

Lambdas inherit the defining scope.

```
   | (let: x 42; {$x}) call
...
[42]
```

### Conditions
Any value may be treated as a boolean; some are always true; integers test true for anything but zero; empty strings test false etc. The ```?``` operator may be used to transform any value to its boolean representation.

```
   | 0?
...
[#f]
```

While the ```!``` operator negates any value.

```
   | 42!
...
[#f]
```

```if```, ```else``` and ```if-else``` may be used to branch on a condition, they call '?' implicitly so you can throw any value at them.

```
  | 'foo' %%, $ if &upper
...
['FOO'@1]

  | #nil else { say 'not true' }
...
not true
[]

  | 42 if-else `not-zero `zero
...
[`not zero]
```

Values may be chained using ```and``` / ```or```.

```
   | #t and 42
...
[42]

   | 0 and 42
...
[0]

   | 42 or #t
...
[42]

   | 0 or 42
...
[42]
```

Lambdas may be used to to prevent evaluating unused arguments when chaining.

```
   | 42 or {say 'Bummer!' #t}
...
[42]
```

### Functions
The ```func:``` macro may be used to define named functions. Several implementations may be defined for the same name as long as they have the same arity and different argument types. Each function captures its defining environment and opens an implicit child scope that is closed on exit. Functions are allowed anywhere in the code, but are defined in order of appearance during compilation.

```
   func: foo() (Int) 42;
...| foo
...
[42]
```

Prefixing a function name with ```&``` pushes a reference on the stack.

```
   | &foo
...
[Func(foo)]

   call
...
[42]
```

Each argument needs a type, ```A``` may be used to accept any type.

```
   func: bar(x A) (A) $x + 35;
...| bar 7
...
[42]
```

Several parameters may share the same type.

```
   func: baz(x y Int) (Int) $x + $y;
...| baz 7 35
...
[42]
```


An index may may be specified instead of type to refer to previous arguments, it is substituted for the actual type on evaluation.

```
   func: baz(x A y T0) (T0) $x + $y;
...| baz 1 2
...
[3]

   | baz 1 'foo'
...
Error in row 1, col 7:
Func not applicable: baz
```

It's possible to specify literal values for arguments instead of names and types.

```
   func: bar(x Int) (Bool) #f;
...func: bar(42) (Bool) #t;
...| bar 41, bar 42
...
[#f #t]
```

Overriding existing implementations is as easy as defining a function with identical argument list.

```

   func: +(x y Int) (Int) 42;
...| 1 + 2
...
[42]
```

```recall``` may be used to call the current function recursively in the same scope, it supports scanning for arguments just like a regular function call.

```
  
   func: fib-rec(a b n Int) (Int)
...  $n? if {, recall $b, $a + $b, -- $n} $a;
...func: fib(n Int) (Int)
...  fib-rec 0 1 $n;
...| fib 50
...
[12586269025]
```

Argument types may be specified in angle brackets to select a specific function implementation. Besides documentation and type checking, this allows disambiguating calls and helps the compiler inline the definition in cases where more than one implementation share the same name.

```
   | &+<Int Int>
...
[Fimp(+ Int Int)]

   &+<Str Str>
...
Error in row 1, col 4:
Func imp not found

   | 7 +<Int Int> 35
...
[42]
```

A vector containing all implementations for a specific function in the order they are considered during dispatch may be retrieved by calling the ```imps``` function.

```
   | &+ imps
...
[[Fimp(+ Rat Rat) Fimp(+ Int Int)]@1]
```

```upcall``` provides an easy way to call the next matching implementation, it also supports scanning for arguments.

```
   
   func: maybe-add(x y Num) (T0)
...  $x + $y;
...func: maybe-add(x y Int) (Int)
...  $x = 42 if 42 {upcall $x $y};
...| maybe-add 1 2 , maybe-add 42 2
...
[3 42]
```

### Conversions
Where conversions to other types make sense, a function named after the target type is provided.

```
   | '42' int
...
[42]

   str<Int>
...
['42'@1]

   1 get
...
[\2]

   int
...
[50]

   + 5 char
...
[\7]
```

### Rationals
Basic rational arithmetics is supported out of the box.

```
   | 1 / 2, -42 / 2 *
...
[-21/2]

   int
...
-10
```

### Optionals
The ```#nil``` value may be used to represent missing values. Since ```Nil``` isn't derived from ```A```, stray ```#nil``` values never get far before being trapped in a function call; ```Opt``` may be used instead where ```#nil``` is allowed.

```
...func: foo(x A) ();
...func: bar(x Opt) (Int) 42;
...| foo #nil
...
Error in row 1, col 1:
Func not applicable: 'foo'

   | bar #nil
...
[42]
```

### Vectors
A vector is a one dimensional dynamic array that supports efficient pushing / popping and random access.

```
   | [1 2, 3 + 4]
...
[[1 2 7]@1]

   % 5 push
...
[[1 2 7 5]@1]

   % pop
...
[[1 2 7]@1 5]

   _ {2 *} for 
...
[2 4 14]
```

### Pairs
Values may be paired by calling ```.```, the result provides reference semantics and access to parts using ```x``` and ```y```.

```
   | 1.2
...
[(1.2)@1]

   % x ~ y
...
[1 2]
```

### Tables
Tables may be used to map ```Cmp``` keys to values, entries are ordered by key.

```
   let: t new Table;
...$t put 2 'bar'
...$t put 1 'foo'
...$t
...
[Table((1 'foo'@1) (2 'bar'@1))@2]

   vect
...
[[(1.'foo'@2)@1 (2.'bar'@2)@1]@1]

...put 1 'baz'
...$t delete 2
...$t
[Table((1 'baz'@1))@2]
```

### Iteration
The ```times``` function may be used to repeat an action N times.

```
   | 10 times 42
...
[42 42 42 42 42 42 42 42 42 42]
```

```
   | 0, 42 times &++
...
[42]
```

While ```for``` loop repeats an action once for each value in any sequence.

```
   | 10 for {+ 42,}
...
[42 43 44 45 46 47 48 49 50 51]
```

```
   | 'foo' for &upper
...
[\F \O \O]
```

Sequences support mapping actions over their values, ```map``` returns an iterator that may be chained further or consumed.

```
   | 'foo' map {int ++ char}
...
[Iter(0x545db40)@1]

   str
...
['gpp'@1]
```

Sequences may be filtered, which also results in a new iterator.

```
10 filter {, $ > 5}
...
[Iter(0x54dfd80)@1]

   for {}
...
[6 7 8 9]
```

Iterators may be created manually by calling ```iter``` on any sequence and consumed manually using ```next``` and ```drop```.

```
   | [1 2 3] iter
...
[Iter(0x53ec8c0)@1]

   % %, $ drop 2 next ~ next
...
[3 #nil]
```

Functions and lambdas are sequences, calling ```iter``` creates an iterator that keeps returning values until the target returns ```#nil```.

```
   func: forever(n Int) (Lambda) {$n};
...| 42 forever iter
...% next ~ next
...
[42 42]
```

### Time
Cixl provides a single concept to represent points in time and intervals. Internally; time is represented as an absolute, zero-based number of months and nanoseconds. The representation is key to providing dual semantics, since it allows remembering enough information to give sensible answers.

Times may be queried for absolute and relative field values;

```
   | let: t now; $t
...
[Time(2018/0/3 20:14:48.105655092)]

   % date ~ time
...
[Time(2018/0/3) Time(20:14:48.105655092)]

   | year $t, month $t, day $t
...
[2018 0 3]

   | months $t
...
[24216]

   / 12 int
...
[2018]

   | hour $t, minute $t, second $t, nsecond $t
...
[20 14 48 105655092]

   | h $t, m $t, s $t, ms $t, us $t, ns $t
...
[93 5591 335485 335485094 335485094756 335485094756404]

   | h $t / 24 int
...
[3]

   | m $t / 60 int
...
[93]
```

manually constructed;

```
   | [2018 0 3 20 14] time
...
[Time(2018/0/3 20:14:0.0)]

   | 3 days
...
[Time(72:0:0.0)]

   days
...
[3]
```

compared, added and subtracted;

```
   | 2m 120s =
...
[#t]

   | 1 years 2 months + 3 days + 12h -
...
[Time(1/2/2) 12:0:0.0]

   <= now
...
[#t]

   | 10 days 1 years -
...
[Time(-1/0/10)]

   days
...
[-356]
```

and scaled.

```
   | 1 months 1 days + 3 *
...
[Time(0/3/3)]
```

### Types
Capitalized names are treated as types, the following types are defined out of the box:

| Type   | Parents     |
| ------ | ----------- |
| A      | Opt         |
| Bin    | A           |
| Bool   | A           |
| Cmp    | A           |
| File   | Cmp         |
| Fimp   | Seq         |
| Func   | Seq         |
| Guid   | A           |
| Int    | Num Seq     |
| Iter   | Seq         |
| Lambda | Seq         |
| Nil    | Opt         |
| Num    | Cmp         |
| Opt    |             |
| Pair   | Cmp         |
| Rat    | Num         |
| Rec    | Cmp         |
| Ref    | A           |
| RFile  | File        |
| RWFile | RFile WFile |
| Seq    | A           |
| Str    | Cmp Seq     |
| Sym    | A           |
| Table  | Seq         |
| Time   | Cmp         |
| Type   | A           |
| Vect   | Cmp Seq     |
| WFile  | File        |

```
   | type 42
...
[Int]

   is A
...
[#t]

   | 42 is Str
...
[#f]
```

### Records
Records map finite sets of typed fields to values. Record types are required to specify an (optionally empty) list of parent types and traits; and will inherit any fields that don't clash with its own. Records are allowed anywhere in the code, but are defined in order of appearance during compilation. ```new``` may be used to create new record instances. Getting and putting field values is accomplished using symbols, uninitialized fields return ```#nil```.


```
   rec: Node()
...  left right Node
...  value A;
...| let: n new Node;
...$n put `value 42
...$n
...
[Node((value 42))@2]

   get `value
...
[42]

   | $n get `left
...
[#nil]
```

Records support full deep equality by default, but ```=``` may be implemented to customize the behavior.

```
   rec: Foo() x Int y Str;
...| let: (bar baz) new Foo %%;
...$bar put `x 42
...$bar put `y 'bar'
...$baz put `x 42
...$baz put `y 'baz'
...$bar = $baz
...
[#f]

   func: =(a b Foo) (Bool) $a get `x =, $b get `x;
...| $bar = $baz
...
[#t]
```

### Traits
Traits are abstract types that may be used to simplify type checking and/or function dispatch. Besides the standard offering; ```A```, ```Cmp```, ```Num```, ```Opt```, ```Rec``` and ```Seq```; new traits may be defined using the ```trait:``` macro. Traits are allowed anywhere in the code, but are defined in order of appearance during compilation.

```
   trait: StrInt Str Int;
...| Str is StrInt, Int is StrInt, Sym is StrInt
...
[#t #t #f]
```

### Meta
A ```Bin``` represents a block of compiled code. The compiler may be invoked from within the language through the ```compile``` function. Binaries may be passed around and called, which simply executes the compiled operations in the current scope.

```
   | new Bin %, $ compile '1 + 2' call
...
[3]
```

### Embedding & Extending
Everything about Cixl has been designed from the ground up to support embedding in, and extending from C. The makefile contains a target named ```libcixl``` that builds a static library containing everything you need to get started. Adding a type and associated function goes something like this:

```C
#include <cixl/box.h>
#include <cixl/cx.h>
#include <cixl/error.h>
#include <cixl/func.h>
#include <cixl/scope.h>

static bool len_imp(struct cx_scope *scope) {
  struct cx_box v = *cx_test(cx_pop(scope, false));
  cx_box_init(cx_push(scope), scope->cx->int_type)->as_int = strlen(v.as_ptr);
  return true;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return x->as_ptr == y->as_ptr;
}

static bool eqval_imp(struct cx_box *x, struct cx_box *y) {
  return strcmp(x->as_ptr, y->as_ptr) == 0;
}

static bool ok_imp(struct cx_box *v) {
  char *s = v->as_ptr;
  return s[0];
}

static void copy_imp(struct cx_box *dst, struct cx_box *src) {
  dst->as_ptr = strdup(src->as_ptr);
}

static void dump_imp(struct cx_box *v, FILE *out) {
  fprintf(out, "'%s'", (char *)v->as_ptr);
}

static void deinit_imp(struct cx_box *v) {
  free(v->as_ptr);
}

int main() {
  struct cx cx;
  cx_init(&cx);
  
  struct cx_type *t = cx_add_type(&cx, "Str", cx.any_type);
  t->eqval = eqval_imp;
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->copy = copy_imp;
  t->write = dump_imp;
  t->dump = dump_imp;
  t->deinit = deinit_imp;
  
  cx_add_cfunc(&cx, "len",
               cx_args(cx_arg("s", t)), cx_rets(cx_ret(cx.int_type)),
	       len_imp);

  cx_add_func(cx, "upper",
	      cx_args(cx_arg("s", t)), cx_rets(cx_ret(t)),
	      "$s map &upper str");

  ...

  cx_deinit(&cx);
  return 0;
}
```

### Modularity
The core language is split into libraries, and may be custom tailored to any level of functionality from C. This is an ongoing process, but you may get an idea of where it's going by having a look on existing [libs](https://github.com/basic-gongfu/cixl/tree/master/src/cixl/libs).

### Performance
There is still plenty of work remaining in the profiling and benchmarking departments, but preliminary indications puts Cixl at around 1-3 times slower than Python. Measured time is displayed in milliseconds.

Let's start with a tail-recursive fibonacci to exercise the interpreter loop, it's worth mentioning that Cixl uses 64-bit integers while Python settles for 32-bit.

```
 
   func: fib-rec(a b n Int) (Int)
...  $n? if-else {$b $a $b + $n -- recall} $a;
...func: fib(n Int) (Int)
...  fib-rec 0 1 $n;
...| clock {10000 times {50 fib _}} / 1000000 int
...
[415]
```

```
from timeit import timeit

def _fib(a, b, n):
    return _fib(b, a+b, n-1) if n > 0 else a

def fib(n):
    return _fib(0, 1, n)

def test():
    for i in range(10000):
        fib(50)

print(int(timeit(test, number=1) * 1000))

$ python3 fib.py 
118
```

Next up is consing a vector.

```
   | clock {(let: v []; 10000000 for {$v ~ push})} / 1000000 int
...
[1958]
```

```
from timeit import timeit

def test():
    v = []
    
    for i in range(10000000):
        v.append(i)

print(int(timeit(test, number=1) * 1000))

$ python3 vect.py 
1348
```

Moving on to instantiating records.

```
   rec: Foo() x Int y Str;
...| clock {10000000 times {new Foo % `x 42 put `y 'bar' put}} / 1000000 int
...
[5702]
```

```
from timeit import timeit

class Foo():
    pass

def test():
    for i in range(10000000):
        foo = Foo()
        foo.x = 42
        foo.y = "bar"

print(int(timeit(test, number=1) * 1000))

$ python3 rec.py
3213
```

### Zen

- Orthogonal is better
- Terseness counts
- There is no right way to do it
- Obvious is overrated
- Symmetry beats consistency
- Rules are for machines
- Only fools predict "the future"
- Intuition goes with the flow
- Duality of syntax is one honking great idea

### License
LGPLv3

Give me a yell if something is unclear, wrong or missing. And please do consider helping out with a donation via [paypal](https://paypal.me/basicgongfu) or [liberapay](https://liberapay.com/basic-gongfu/donate) if you find this worthwhile, every contribution counts.