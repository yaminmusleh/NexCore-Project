#!/bin/bash
# Stage 1 tests. Usage: bash run_stage1_tests.sh /path/to/nexc
# Increment 1 (1a_*): bool type and comparisons/logic as values.
# Increment 2 (1b_*): true / false literals (needs regenerated parser).
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

# ---------------- positive ----------------
check 1a_01_cmp_true "true [exit=0]" <<'N'
set age = 20;
set adult = age >= 18;
print(adult);
N
check 1a_02_cmp_false "false [exit=0]" <<'N'
set age = 20;
set minor = age < 18;
print(minor);
N
check 1a_03_logical_values "true false [exit=0]" <<'N'
set a = 20;
set both = (a > 10) && (a < 30);
set either = (a < 10) || (a > 100);
print(both);
print(either);
N
check 1a_04_not "false true [exit=0]" <<'N'
set adult = 20 >= 18;
set n = !adult;
print(n);
print(!n);
N
check 1a_05_bool_in_conditions "yes 0 1 2 [exit=0]" <<'N'
set ready = 3 > 1;
iff (ready) {
    print("yes");
} otherwise {
    print("no");
}
set i = 0;
set going = i < 3;
whilst (going) {
    print(i);
    i = i + 1;
    going = i < 3;
}
N
check 1a_06_short_circuit "false true [exit=0]" <<'N'
set z = 0;
set ok = (z != 0) && (10 / z > 1);
set ok2 = (z == 0) || (10 / z > 1);
print(ok);
print(ok2);
N
check 1a_07_float_compare "true true [exit=0]" <<'N'
set f = 1.5f;
set c = f < 2.5f;
set d = 2.5d;
set e = d == 2.5d;
print(c);
print(e);
N
check 1a_08_reassign_bool "true false [exit=0]" <<'N'
set b = 1 < 2;
print(b);
b = 1 > 2;
print(b);
N
check 1a_09_bool_equality "true true [exit=0]" <<'N'
set a = 1 < 2;
set b = 3 < 4;
set same = a == b;
set diff = a != (1 > 2);
print(same);
print(diff);
N
check 1a_10_bool_in_loop_body "false true false [exit=0]" <<'N'
for (set i = 0; i < 3; i = i + 1) {
    set odd = i % 2 == 1;
    print(odd);
}
N
check 1a_11_not_of_comparison "true false [exit=0]" <<'N'
print(!(3 > 4));
print(!(3 < 4));
N
check 1a_12_arithmetic_unchanged "14 6 [exit=0]" <<'N'
print(2 + 3 * 4);
set x = 10;
print(x - 4);
N
check 1a_13_precedence "true false [exit=0]" <<'N'
print(1 < 2 && 2 < 3 || 1 > 2);
print(1 > 2 || 2 > 3 && 1 < 2);
N
check 1a_14_int_condition_still_ok "3 2 1 one [exit=0]" <<'N'
set n = 3;
whilst (n) {
    print(n);
    n = n - 1;
}
iff (1) {
    print("one");
}
N
check 1a_15_bool_survives_temporaries "true 21 [exit=0]" <<'N'
set a = 1 < 2;
set b = 7 + 2 * 7;
print(a);
print(b);
N

# ---------------- negative ----------------
check 1a_neg_01_bool_plus_int "COMPILE_FAIL(1): error: Operator '+' cannot be applied to bool" <<'N'
set b = 1 < 2;
set c = b + 1;
N
check 1a_neg_02_assign_int_to_bool "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: b" <<'N'
set b = 1 < 2;
b = 5;
N
check 1a_neg_03_bool_relational "COMPILE_FAIL(1): error: Operator '<' cannot be applied to bool" <<'N'
set b = 1 < 2;
set c = b < 2;
N
check 1a_neg_04_negate_bool "COMPILE_FAIL(1): error: Operator '-' cannot be applied to bool" <<'N'
set b = 1 < 2;
set c = -b;
N
check 1a_neg_05_bool_eq_int "COMPILE_FAIL(1): error: Cannot compare bool with int using '=='" <<'N'
set b = 1 < 2;
set c = b == 1;
N
check 1a_neg_06_assign_bool_to_int "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: n" <<'N'
set n = 5;
n = n > 1;
N
check 1a_neg_07_bool_arith_in_condition "COMPILE_FAIL(1): error: Operator '+' cannot be applied to bool" <<'N'
set b = 1 < 2;
iff (b + 1 > 0) {
    print(1);
}
N
check 1a_neg_08_unknown_variable "COMPILE_FAIL(1): error: Unknown variable: zz" <<'N'
print(zz > 1);
N


# ======== Increment 2: true / false literals ========
check 1b_01_literals "true false [exit=0]" <<'N'
set t = true;
set f = false;
print(t);
print(f);
N
check 1b_02_print_literals "true false [exit=0]" <<'N'
print(true);
print(false);
N
check 1b_03_logic_with_literals "true false true [exit=0]" <<'N'
print(true && !false);
print(true && false);
print(false || true);
N
check 1b_04_while_flag "0 1 2 [exit=0]" <<'N'
set done = false;
set i = 0;
whilst (!done) {
    print(i);
    i = i + 1;
    iff (i == 3) {
        done = true;
    }
}
N
check 1b_05_literal_conditions "yes else [exit=0]" <<'N'
iff (true) {
    print("yes");
}
iff (false) {
    print("no");
} otherwise {
    print("else");
}
N
check 1b_06_equality_with_literals "true true false [exit=0]" <<'N'
set a = 1 < 2;
print(a == true);
print(a != false);
print(a == false);
N
check 1b_07_reassign "true false [exit=0]" <<'N'
set flag = false;
flag = true;
print(flag);
flag = 5 > 9;
print(flag);
N
check 1b_08_literal_survives_temporaries "true 21 [exit=0]" <<'N'
set a = true;
set b = 7 + 2 * 7;
print(a);
print(b);
N
check 1b_09_not_literal "false true [exit=0]" <<'N'
print(!true);
print(!false);
N
check 1b_10_similar_identifiers_ok "1 3 2 [exit=0]" <<'N'
set trueish = 1;
set True = 3;
set falsey = 2;
print(trueish);
print(True);
print(falsey);
N
check 1b_11_while_true_with_exit "[exit=5]" <<'N'
whilst (true) {
    exit(5);
}
N
check 1b_12_literal_in_loop_body "true true true [exit=0]" <<'N'
for (set i = 0; i < 3; i = i + 1) {
    set ok = true;
    print(ok);
}
N

check 1b_neg_01_true_plus_int "COMPILE_FAIL(1): error: Operator '+' cannot be applied to bool" <<'N'
set x = true + 1;
N
check 1b_neg_02_assign_int_to_bool_var "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: flag" <<'N'
set flag = true;
flag = 1;
N
check 1b_neg_03_assign_to_literal "COMPILE_FAIL(1): -- line 1 col 1: EOF expected" <<'N'
true = false;
N
check 1b_neg_04_literal_as_name "COMPILE_FAIL(1): -- line 1 col 5: identifier expected" <<'N'
set true = 1;
N
check 1b_neg_05_negate_literal "COMPILE_FAIL(1): error: Operator '-' cannot be applied to bool" <<'N'
set x = -true;
N
check 1b_neg_06_literal_relational "COMPILE_FAIL(1): error: Operator '<' cannot be applied to bool" <<'N'
print(true < false);
N
check 1b_neg_07_assign_bool_to_int_var "COMPILE_FAIL(1): error: Type mismatch in assignment to variable: n" <<'N'
set n = 1;
n = true;
N
check 1b_neg_08_int_equals_literal "COMPILE_FAIL(1): error: Cannot compare int with bool using '=='" <<'N'
print(1 == true);
N

echo; echo "passed=$pass failed=$fail"; [ $fail -eq 0 ]