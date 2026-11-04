#!/bin/bash
# Stage 2 tests (functions). Usage: bash run_stage2_tests.sh /path/to/nexc
# 2b: no-parameter void functions and call statements.
# 2c: typed parameters, gives/give, calls inside expressions, recursion.
set -u
EXE=$(realpath "${1:?usage: $0 /path/to/nexc}")
WORK=$(mktemp -d); cd "$WORK"; pass=0; fail=0

check() {
    local name=$1 expected=$2
    cat > "$name.nex"
    local cout; cout=$("$EXE" "$name.nex" 2>&1); local crc=$?
    local got
    if [ $crc -ne 0 ]; then got="COMPILE_FAIL($crc): $(echo "$cout" | head -1)"
    else
        ./out > run_out.txt; local rrc=$?
        got="$(tr '\n' ' ' < run_out.txt)[exit=$rrc]"
    fi
    if [ "$got" == "$expected" ]; then echo "PASS  $name"; pass=$((pass+1))
    else echo "FAIL  $name"; echo "   expected: $expected"; echo "   got:      $got"; fail=$((fail+1)); fi
}

# ---------------- 2b positive ----------------
check 2b_01_basic "Hello from func done [exit=0]" <<'N'
func hello() {
    print("Hello from func");
}
hello();
print("done");
N
check 2b_02_call_repeatedly "1 1 1 [exit=0]" <<'N'
func tick() {
    print(1);
}
tick();
tick();
tick();
N
check 2b_03_define_after_use "hi [exit=0]" <<'N'
greet();
func greet() {
    print("hi");
}
N
check 2b_04_locals_isolated "5 1 [exit=0]" <<'N'
func f() {
    set x = 5;
    print(x);
}
set x = 1;
f();
print(x);
N
check 2b_05_calls_inside_loop "hi 0 hi 1 hi 2 [exit=0]" <<'N'
func hi() {
    print("hi");
}
for (set i = 0; i < 3; i = i + 1) {
    hi();
    print(i);
}
N
check 2b_06_function_calls_function "a1 b a2 [exit=0]" <<'N'
func b() {
    print("b");
}
func a() {
    print("a1");
    b();
    print("a2");
}
a();
N
check 2b_07_control_flow_inside "big 0 [exit=0]" <<'N'
func check() {
    set n = 3;
    iff (n > 2) {
        print("big");
    } otherwise {
        print("small");
    }
    whilst (n > 0) {
        n = n - 1;
    }
    print(n);
}
check();
N
check 2b_08_exit_inside_function "start bye [exit=3]" <<'N'
func stop() {
    print("bye");
    exit(3);
}
print("start");
stop();
print("never");
N
check 2b_09_empty_function "ok [exit=0]" <<'N'
func nothing() {
}
nothing();
print("ok");
N
check 2b_10_text_and_bool_inside "text true true [exit=0]" <<'N'
func show() {
    set s = "text";
    print(s);
    print(1 < 2);
    print(true);
}
show();
N
check 2b_11_caller_variables_survive "7 30 [exit=0]" <<'N'
func f() {
    set p = 1;
    set q = 2 * 3 + p;
    print(q);
}
set a = 10;
set b = 20;
f();
print(a + b);
N
check 2b_12_unused_function_ok "only [exit=0]" <<'N'
func unused() {
    print("never");
}
print("only");
N
check 2b_13_floats_inside "ok [exit=0]" <<'N'
func f() {
    set x = 1.5f;
    set y = x + 2.5f;
    iff (y == 4.0f) {
        print("ok");
    }
}
f();
N
check 2b_14_call_in_nested_scopes "in 1 out [exit=0]" <<'N'
func p() {
    print("in");
}
set k = 1;
{
    set j = 2;
    p();
    print(k);
}
print("out");
N
check 2b_15_same_name_variable_and_function "9 f [exit=0]" <<'N'
func f() {
    print("f");
}
set f = 9;
print(f);
f();
N

# ---------------- 2b negative ----------------
check 2b_neg_01_unknown_function "COMPILE_FAIL(1): error: Unknown function: nothing" <<'N'
nothing();
N
check 2b_neg_02_duplicate_function "COMPILE_FAIL(1): error: Function 'a' is already defined" <<'N'
func a() {
}
func a() {
}
N
check 2b_neg_03_missing_parens "COMPILE_FAIL(1): -- line 1 col 8: open_paren expected" <<'N'
func a { }
N
check 2b_neg_04_missing_body "COMPILE_FAIL(1): -- line 2 col 1: open_scope expected" <<'N'
func a()
N
check 2b_neg_05_func_inside_block "COMPILE_FAIL(1): -- line 1 col 3: close_scope expected" <<'N'
{ func a() { } }
N
check 2b_neg_06_func_is_reserved "COMPILE_FAIL(1): -- line 1 col 5: identifier expected" <<'N'
set func = 1;
N
check 2b_neg_07_no_access_to_top_level_vars "COMPILE_FAIL(1): error: Unknown variable: x" <<'N'
set x = 1;
func f() {
    print(x);
}
f();
N
check 2b_neg_08_missing_name "COMPILE_FAIL(1): -- line 1 col 6: identifier expected" <<'N'
func () { }
N
check 2b_neg_09_bare_identifier "COMPILE_FAIL(1): -- line 1 col 2: invalid IdentifierStmt" <<'N'
x;
N


# ======== 2c positive ========
check 2c_01_int_params "5 [exit=0]" <<'N'
func add(a: int, b: int) gives int {
    give a + b;
}
print(add(2, 3));
N
check 2c_02_text_param "Yamen [exit=0]" <<'N'
func greet(name: text) {
    print(name);
}
greet("Yamen");
N
check 2c_03_calls_in_math "41 [exit=0]" <<'N'
func sq(x: int) gives int {
    give x * x;
}
print(sq(3) + sq(4) * 2);
N
check 2c_04_set_from_call "10 [exit=0]" <<'N'
func add(a: int, b: int) gives int {
    give a + b;
}
set total = add(2, 3);
total = total * 2;
print(total);
N
check 2c_05_factorial "120 3628800 [exit=0]" <<'N'
func fact(n: int) gives int {
    iff (n <= 1) {
        give 1;
    }
    give n * fact(n - 1);
}
print(fact(5));
print(fact(10));
N
check 2c_06_fibonacci "55 [exit=0]" <<'N'
func fib(n: int) gives int {
    iff (n < 2) {
        give n;
    }
    give fib(n - 1) + fib(n - 2);
}
print(fib(10));
N
check 2c_07_nested_calls_as_arguments "10 [exit=0]" <<'N'
func add(a: int, b: int) gives int {
    give a + b;
}
func dbl(x: int) gives int {
    give add(x, x);
}
print(add(dbl(2), dbl(3)));
N
check 2c_08_bool_return "true false even [exit=0]" <<'N'
func isEven(n: int) gives bool {
    give n % 2 == 0;
}
print(isEven(4));
print(isEven(7));
iff (isEven(10)) {
    print("even");
}
N
check 2c_09_text_return "Nex [exit=0]" <<'N'
func name() gives text {
    give "Nex";
}
print(name());
N
check 2c_10_float_function "float ok [exit=0]" <<'N'
func half(x: float) gives float {
    give x / 2.0f;
}
iff (half(5.0f) == 2.5f) {
    print("float ok");
}
N
check 2c_11_double_function "double ok [exit=0]" <<'N'
func twice(x: double) gives double {
    give x * 2.0d;
}
iff (twice(1.25d) == 2.5d) {
    print("double ok");
}
N
check 2c_12_argument_order "7 -7 [exit=0]" <<'N'
func sub(a: int, b: int) gives int {
    give a - b;
}
print(sub(10, 3));
print(sub(3, 10));
N
check 2c_13_many_parameters "1234 [exit=0]" <<'N'
func f(a: int, b: int, c: int, d: int) gives int {
    give a * 1000 + b * 100 + c * 10 + d;
}
print(f(1, 2, 3, 4));
N
check 2c_14_call_in_loop_condition "0 1 2 [exit=0]" <<'N'
func lt(a: int, b: int) gives bool {
    give a < b;
}
set i = 0;
whilst (lt(i, 3)) {
    print(i);
    i = i + 1;
}
N
check 2c_15_gcd "6 [exit=0]" <<'N'
func gcd(a: int, b: int) gives int {
    iff (b == 0) {
        give a;
    }
    give gcd(b, a % b);
}
print(gcd(48, 18));
N
check 2c_16_bare_give_in_void "negative ok [exit=0]" <<'N'
func check(n: int) {
    iff (n < 0) {
        print("negative");
        give;
    }
    print("ok");
}
check(-1);
check(5);
N
check 2c_17_parameters_are_copies "6 5 [exit=0]" <<'N'
func bump(n: int) gives int {
    n = n + 1;
    give n;
}
set a = 5;
print(bump(a));
print(a);
N
check 2c_18_caller_state_survives "100 16 [exit=0]" <<'N'
func work(x: int) gives int {
    set t = x * 2;
    set u = t + 1;
    give u;
}
set a = 100;
set b = work(3) + work(4);
print(a);
print(b);
N
check 2c_19_mutual_recursion "true true false [exit=0]" <<'N'
func isEven(n: int) gives bool {
    iff (n == 0) {
        give true;
    }
    give isOdd(n - 1);
}
func isOdd(n: int) gives bool {
    iff (n == 0) {
        give false;
    }
    give isEven(n - 1);
}
print(isEven(10));
print(isOdd(7));
print(isEven(7));
N
check 2c_20_result_can_be_discarded "7 [exit=0]" <<'N'
func f(x: int) gives int {
    print(x);
    give x;
}
f(7);
N
check 2c_21_give_inside_loop "8 -1 [exit=0]" <<'N'
func firstOver(limit: int) gives int {
    for (set i = 0; i < 100; i = i + 1) {
        iff (i * i > limit) {
            give i;
        }
    }
    give 0 - 1;
}
print(firstOver(50));
print(firstOver(100000));
N
check 2c_22_deep_recursion "500500 [exit=0]" <<'N'
func sum(n: int) gives int {
    iff (n == 0) {
        give 0;
    }
    give n + sum(n - 1);
}
print(sum(1000));
N
check 2c_23_bool_parameters "true false [exit=0]" <<'N'
func both(a: bool, b: bool) gives bool {
    give a && b;
}
print(both(true, 1 < 2));
print(both(true, false));
N
check 2c_24_float_parameters "avg ok [exit=0]" <<'N'
func avg(a: float, b: float) gives float {
    give (a + b) / 2.0f;
}
iff (avg(1.0f, 4.0f) == 2.5f) {
    print("avg ok");
}
N
check 2c_25_expression_arguments "13 [exit=0]" <<'N'
func add(a: int, b: int) gives int {
    give a + b;
}
set x = 4;
print(add(x * 2, add(1, x)));
N
check 2c_26_text_passes_through "abc [exit=0]" <<'N'
func id(s: text) gives text {
    give s;
}
print(id("abc"));
N
check 2c_27_result_as_exit_code "[exit=7]" <<'N'
func code() gives int {
    give 7;
}
exit(code());
N
check 2c_28_mixed_types_in_one_call "ok 3 [exit=0]" <<'N'
func mix(n: int, s: text, f: float, b: bool) gives int {
    iff (b && f > 1.0f) {
        print(s);
    }
    give n;
}
print(mix(3, "ok", 2.5f, true));
N

# ======== 2c negative ========
check 2c_neg_01_too_few_arguments "COMPILE_FAIL(1): error: Function 'add' expects 2 arguments, but 1 was given" <<'N'
func add(a: int, b: int) gives int {
    give a + b;
}
print(add(1));
N
check 2c_neg_02_too_many_arguments "COMPILE_FAIL(1): error: Function 'f' expects 0 arguments, but 1 was given" <<'N'
func f() {
}
f(1);
N
check 2c_neg_03_wrong_argument_type "COMPILE_FAIL(1): error: Argument 1 of 'f' must be int, but got text" <<'N'
func f(x: int) {
}
f("a");
N
check 2c_neg_04_void_used_as_value "COMPILE_FAIL(1): error: Function 'f' does not give a value, so it cannot be used in an expression" <<'N'
func f() {
}
set x = f();
N
check 2c_neg_05_give_wrong_type "COMPILE_FAIL(1): error: Function 'f' must give int, but this gives text" <<'N'
func f() gives int {
    give "no";
}
N
check 2c_neg_06_bare_give_in_valued_function "COMPILE_FAIL(1): error: Function 'f' must give a value of type int" <<'N'
func f() gives int {
    give;
}
N
check 2c_neg_07_value_given_by_void_function "COMPILE_FAIL(1): error: Function 'f' does not give a value, so 'give' cannot have an expression" <<'N'
func f() {
    give 1;
}
N
check 2c_neg_08_give_outside_function "COMPILE_FAIL(1): error: 'give' can only be used inside a function" <<'N'
give 1;
N
check 2c_neg_09_missing_give_on_a_path "COMPILE_FAIL(1): error: Function 'f' may reach its end without giving a value" <<'N'
func f(n: int) gives int {
    iff (n > 0) {
        give 1;
    }
}
N
check 2c_neg_10_duplicate_parameter "COMPILE_FAIL(1): error: Duplicate parameter 'a' in function 'f'" <<'N'
func f(a: int, a: int) {
}
N
check 2c_neg_11_unknown_type_name "COMPILE_FAIL(1): -- line 1 col 11: invalid TypeName" <<'N'
func f(a: number) {
}
N
check 2c_neg_12_missing_parameter_type "COMPILE_FAIL(1): -- line 1 col 9: colon expected" <<'N'
func f(a) {
}
N
check 2c_neg_13_float_condition "COMPILE_FAIL(1): error: A condition must be bool or int, but got float" <<'N'
set x = 1.5f;
iff (x) {
    print(1);
}
N
check 2c_neg_14_text_condition "COMPILE_FAIL(1): error: A condition must be bool or int, but got text" <<'N'
whilst ("a") {
}
N
check 2c_neg_15_bool_parameter_given_int "COMPILE_FAIL(1): error: Argument 1 of 'f' must be bool, but got int" <<'N'
func f(b: bool) {
}
f(1);
N
check 2c_neg_16_trailing_comma "COMPILE_FAIL(1): -- line 1 col 15: identifier expected" <<'N'
func f(a: int,) {
}
N
check 2c_neg_17_unknown_function_in_expression "COMPILE_FAIL(1): error: Unknown function: g" <<'N'
print(g(1));
N
check 2c_neg_18_result_type_mismatch "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: s" <<'N'
func f() gives int {
    give 1;
}
set s = "a";
s = f();
N
check 2c_neg_19_bool_result_in_arithmetic "COMPILE_FAIL(1): error: Operator '+' cannot be applied to bool" <<'N'
func b() gives bool {
    give true;
}
print(b() + 1);
N
check 2c_neg_20_parameter_not_visible_outside "COMPILE_FAIL(1): error: Unknown variable: a" <<'N'
func f(a: int) {
}
print(a);
N
check 2c_neg_21_loop_only_give "COMPILE_FAIL(1): error: Function 'f' may reach its end without giving a value" <<'N'
func f() gives int {
    whilst (true) {
        give 1;
    }
}
N
check 2c_neg_22_gives_without_type "COMPILE_FAIL(1): -- line 1 col 16: invalid TypeName" <<'N'
func f() gives {
}
N

echo; echo "passed=$pass failed=$fail"; [ $fail -eq 0 ]