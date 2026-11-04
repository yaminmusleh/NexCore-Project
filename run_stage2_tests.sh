#!/bin/bash
# Stage 2 tests (functions). Usage: bash run_stage2_tests.sh /path/to/nexc
# 2b: no-parameter void functions and call statements.
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

echo; echo "passed=$pass failed=$fail"; [ $fail -eq 0 ]