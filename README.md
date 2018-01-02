<a href="https://liberapay.com/basic-gongfu/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a>

## cixl
#### a minimal, decently typed scripting language

This project aims to produce a minimal, decently typed scripting language for embedding in and extending from C. The language is implemented as a straight forward 3-stage (parse/compile/eval) interpreter that is designed to be as fast as possible without compromising on simplicity, transparency and flexibility. The codebase has no external dependencies and is currently hovering around 3 kloc.

### Getting Started
To get started, you'll need a decent C compiler with GNU-extensions and CMake installed. A primitive REPL is included, the executable weighs in at 400k. It's highly recommended to run the REPL through ```rlwrap``` for a less nerve-wrecking editing experience.

```
git clone https://github.com/basic-gongfu/cixl.git
cd cixl
mkdir build
cd build
cmake ..
make
rlwrap ./cixl

cixl v0.5, 18765 bmips

Press Return twice to eval input.

> 1 2 3
..
[1 2 3]

> quit
```

### Status
Examples from this document should work in the most recent version and run clean in ```valgrind```, outside of that I can't really promise much at the moment. Current work is focused on profiling and filling obvious gaps in functionality.

### Stack
The stack is accessible from user code, just like in Forth. Basic stack operations have dedicated operators; ```%``` for duplicating last value, ```_``` for dropping it, ```~``` for flipping the last two values and ```|``` for clearing the stack.

```
> 1 2 3 %
..
[1 2 3 3]

> _
..
[1 2 3]

> ~
..
[1 3 2]

> |
..
[]
```

### Expressions
But unlike Forth, functions scan forward until enough arguments are on the stack to allow reordering parameters and operations in user code to fit the problem being solved.

```
> 1 + 2
..
[3]

> 1 2 +
..
[3 3]

> + 1 2
..
[6 1 2]

> + +
..
[9]
```

The ```,``` operator may be used to cut the stack into discrete pieces and force functions to scan forward.

```
> 1 + 2
..3 + 4
..
[6 4]

> 1 + 2,
..3 + 4
..
[3 7]
```

### Variables
Named variables may be defined once per scope using the ```let:``` macro.

```
> let: foo 'bar';
..
[]

> $foo
..
['bar']

> let: foo 'baz';
..
Error in row 1, col 10:
Attempt to rebind variable: 'foo'
```

### Equality
Two flavors of equality are provided.

Value equality:
```
> 'foo' = 'foo'
..
[t]
```

And identity:
```
> 'foo' == 'foo'
..
[f]
```

```
> 42 == 42
..
[t]
```

### IO
```say``` and ```ask``` may be used to perform basic IO.

```
> say 'hello'  
..ask 'what\'s your name? '
..
hello
what's your name: Sifoo
['Sifoo']
```

Code may be loaded from file using ```load```, the loaded code is evaluated in the current scope.

test.cx:
```
+2
```

```
> 1, load 'test.cx'
..
[3]
```

### Scopes
Enclosing code in parens evaluates it in a separate scope with a clean stack. The last value on the stack is automatically returned on scope exit.

```
> (1 2 3)
..
[3]
```

Variables in the parent scope may be referenced from within the scope, but variables defined inside are not visible from the outside.

```
> let: foo 1;
..(let: foo 2; $foo)
..$foo
..
[2 1]
```

### Conditions
All types are useable as conditions; some are always true; integers test true for anything but zero; empty strings test false etc. The ```?``` operator may be used to transform any value to its conditional representation.

```
> 0?
[f]
```

The ```!``` operator negates any condition.

```
> 42!
[f]
```

The ```if``` statement may be used to branch on a condition, it calls '?' implicitly so you can throw any value at it.

```
> 42 if 'not zero' 'zero'
..
['not zero']

> ''! if 'empty' 'not empty'
..
['empty']
```

### Lambdas
Putting braces around a block of code defines a lambda, which is then pushed on the stack.

```
> {1 2 3}
..
[Lambda(0x52d97d0@1)]

> call
..
[1 2 3]
```

Lambdas inherit the defining scope.

```
> (let: x 42; {$x}) call
..
[42]
```

### Functions
The ```func:``` macro may be used to define named functions. Several implementations may be defined for the same name as long as they have the same arity and different argument types. Each function opens an implicit scope that is closed on exit.

```
> func: foo() 42;
..foo
..
[42]
```

Prefixing a function name with ```&``` pushes a reference on the stack.

```
> func: foo() 42;
..&foo
..
[Func(foo)]

> call
..
[42]
```

Each argument needs a type, ```A``` may be used to accept any type.

```
> func: bar(x A) $x + 35;
..bar 7
..
[42]
```

Several parameters may share the same type. An index may may be specified instead of type to refer to previous arguments, it is substituted for the actual type on evaluation.

```
> func: baz(x y Int z 0)
    $x + $y + $z;
..baz 1 3 5
..
[9]
```

```recall``` may be used to call the current function recursively in the same scope, it supports scanning for arguments just like a regular function call.

```
> func: fib-rec(a b n Int)
..  $n? if {, recall $b, $a + $b, -- $n} $a;
..func: fib(n Int)
..  fib-rec 0 1 $n;
..fib 50
[12586269025]
```

Argument types may be specified in angle brackets to select a specific function implementation. Besides documentation and sanity checking, this allows the compiler to inline the definition in cases where more than one implementation share the same name.

```
> &+<Int>
..
[Fimp(+ Int Int)]

> &+<Str>
..
Error in row 1, col 4:
Func imp not found
```

A vector containing all implementations for a specific function may be retrieved by calling the ```imps``` function.

```
> &+ imps
..
[[Fimp(+ Int Int) Fimp(+ Rat Rat)]@1]
```

### Conversions
Where conversions to other types make sense, a function named after the target type is provided.

```
> '42' int
..
[42]

> str
..
['42']

> 1 get
..
[\2]

> int
..
[50]

> + 5 char
..
[\7]
```

### Rationals
Basic rational arithmetics is supported out of the box.

```
> 1 / 2, -42 / 2 *
..
[-21/2]

> int
..
-10
```

### Optionals
The nil value may be used to represent missing values. Since ```Nil``` isn't derived from ```A```, stray nil values never get far before being trapped in a function call; ```Opt``` may be used instead where nil values are allowed.

```
> func: foo(x A);
..func: bar(x Opt) 42;
..
[]

> foo nil
..
Error in row 1, col 1:
Func not applicable: 'foo'

> | bar nil
..
[42]
```

### Vectors
A vector is a one dimensional dynamic array that supports efficient pushing / popping and random access. The stack itself is a vector which may be retrieved using the ```vect``` function.

```
> 1 2 (3 4 vect)
..
[1 2 [3 4]@1]

> % 5 push
..
[1 2 [3 4 5]@1]

> % pop
..
[1 2 [3 4]@1 5]

> _ {2 *} for 
..
[1 2 6 8]
```

### Loops
The ```times``` loop may be used to repeat an action N times.

```
> 10 times 42
..
[42 42 42 42 42 42 42 42 42 42]
```

```
> 0, 42 times &++
..
[42]
```

While the ```for``` loop repeats an action once for each value in a sequence, the current value is pushed on the stack before calling the action.

```
> 10 for {+ 42,}
..
[42 43 44 45 46 47 48 49 50 51]
```

```
> 'foo' for &upper
..
[\F \O \O]
```

Some types support mapping actions over their contents using ```map```.

```
> 'FOO' map &lower
..
['foo']
```

### Types
Capitalized names are treated as type references, the following types are defined out of the box:

- A (Opt)
- Bin (A)
- Bool (A)
- Fimp (A)
- Func (A)
- Int (Num)
- Lambda (A)
- Nil (Opt)
- Num (A)
- Opt ()
- Rat (Num)
- Str (A)
- Type (A)
- Vect (A)

```
> type 42
[Int]

> is A
[t]
```

### Traits
Traits are abstract types, they are useful for simplifying type checking and/or function dispatch. Besides the standard offering; 'A', 'Num' and 'Opt'; new traits may be defined using the ```trait:``` macro.

```
> trait: StrInt Str Int;
..
[]

> Str is StrInt,
..Int is StrInt,
..StrInt is A
[t t f]

> trait: StrIntChar StrInt Char;
..
[]
```

### Meta
The compiler may be invoked from within the language through the ```compile``` function, the result is a compiled sequence of operations that may be passed around and called.

```
> '1 + 2' compile
..
[Bin(0x8899a0)@1]

> call
..
[3]
```

### Embedding & Extending
Everything about cixl has been designed from the ground up to support embedding in, and extending from C. The makefile contains a target named ```libcixl``` that builds a static library containing everything you need to get started. Adding a type and associated function goes something like this:

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

static void fprint_imp(struct cx_box *v, FILE *out) {
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
  t->fprint = fprint_imp;
  t->deinit = deinit_imp;
  
  cx_add_func(&cx, "len", cx_arg(t))->ptr = len_imp;

  ...

  cx_deinit(&cx);
  return 0;
}
```

### Performance
There is still a lot of work remaining in the profiling and benchmarking departments, but preliminary results puts cixl at around 1-5 times slower than Python. Measured time is displayed in milliseconds.

Let's start with a tail-recursive fibonacci to exercise the interpreter loop, it's worth mentioning that cixl uses 64-bit integers while Python settles for 32-bit.

```
> func: fib-rec(a b n Int)
..  $n? if {$b $a $b + $n -- recall} $a;
..func: fib(n Int)
..  fib-rec 0 1 $n;
..clock {10000 times {50 fib _}} / 1000000 int
..
[571]
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
> | clock {(let: v vect; 10000000 for {$v ~ push})} / 1000000 int
..
[2034]
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
GPLv3

Give me a yell if something is unclear, wrong or missing. And please do consider helping out with a donation via [paypal](https://paypal.me/basicgongfu) or [liberapay](https://liberapay.com/basic-gongfu/donate) if you find this worthwhile. I have the opportunity and motivation to work full time on this project, but I still need some help with covering basic needs for that to happen.