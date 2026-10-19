#!/bin/bash
# Usage: ./run_stage0_tests.sh /path/to/nexc
# Compiles and runs each Stage 0 test, comparing stdout + exit code (or the compile error) to the expected value.
set -u
EXE=$(realpath "${1:?usage: $0 /path/to/nexc}")
WORK=$(mktemp -d); cd "$WORK"; pass=0; fail=0

# check NAME EXPECTED_RUN_OUTPUT   (program text on stdin)
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

check 01_temp_clobber "5 13 [exit=0]" <<'N'
set a = 5;
set b = 7 + 2 * 3;
print(a);
print(b);
exit(0);
N
check 02_nested_scopes "2 9 2 1 3 [exit=0]" <<'N'
set x = 1;
{
    set y = 2;
    print(y);
    {
        set w = 9;
        print(w);
    }
    print(y);
}
set z = 3;
print(x);
print(z);
exit(0);
N
check 03_float_vars "1 9 [exit=0]" <<'N'
set f = 1.5f;
set g = 2;
set h = f + 2.5f;
set k = g * 4 + 1;
iff (h == 4.0f) {
    print(1);
} otherwise {
    print(0);
}
print(k);
exit(0);
N
check 04_loop_vars "0 1 2 0 1 100 [exit=0]" <<'N'
for (set i = 0; i < 3; i = i + 1) {
    print(i);
}
for (set i = 0; i < 2; i = i + 1) {
    print(i);
}
set j = 100;
print(j);
exit(0);
N
check 05_loop_zero_iterations "42 [exit=0]" <<'N'
whilst (1 == 2) {
    exit(7);
}
for (set i = 5; i < 1; i = i + 1) {
    exit(3);
}
print(42);
N
check 06_exit_in_branch "7 9 [exit=0]" <<'N'
iff (1 == 2) {
    set a = 1;
    exit(1);
}
set b = 7;
set c = b + 1 * 2;
print(b);
print(c);
N
check 07_body_variables "5 [exit=0]" <<'N'
set total = 0;
set n = 0;
whilst (n < 3) {
    set sq = n * n;
    total = total + sq;
    n = n + 1;
}
print(total);
N
check 08_regression "Yamen Musleh Adult 25 0 1 2 3 4 [exit=0]" <<'N'
set name = "Yamen";
set age = 20;
name = "Yamen Musleh";
print(name);
iff (age >= 18) {
    print("Adult");
} otherwise {
    print("Minor");
}
whilst (age < 25) {
    age = age + 1;
}
print(age);
for (set i = 0; i < 5; i = i + 1) {
    print(i);
}
exit(0);
N
check neg_01_undeclared "COMPILE_FAIL(1): error: Unknown variable: zzz" <<'N'
print(zzz);
N
check neg_02_type_mismatch "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: a" <<'N'
set a = 1;
a = "hi";
N
check neg_03_syntax "COMPILE_FAIL(1): -- line 1 col 9: invalid UnaryExpr" <<'N'
set x = ;
print(x);
N
check neg_04_loop_var_escapes "COMPILE_FAIL(1): error: Unknown variable: i" <<'N'
for (set i = 0; i < 2; i = i + 1) {
    print(i);
}
print(i);
N

echo; echo "passed=$pass failed=$fail"; [ $fail -eq 0 ]
