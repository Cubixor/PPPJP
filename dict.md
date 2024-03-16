# PPPJP dictionary

```
<program> → <statement>* 
<statement> → { 
    <function>
    <scope>
    zmienna <type> `ident` równa <expr> 
    tablica <type> `ident` rozmiaru <numerical_expr>
    tablica <type> `ident` równa <array_expr>
    `ident` równa <expr>
    `ident` element <expr> równa <expr>
    jeśli ( <boolean_expr> ): <statement> 
        [przeciwnie jeśli ( <boolean_expr> ): <statement>] 
        [przeciwnie: <statement>]
    powtarzaj: <statement>/<loop_flow_stmt>
    powtarzaj jeśli ( <boolean_expr> ): <statement>/<loop_flow_stmt>
    
}
<function> → {
    kończwaść ( <numerical_expr> )
}
<loop_flow_stmt> → {
    przerwij
    kontynuuj
}
<scope> → {
    { <statement>* }
}
<type> → { 
    całkowita 
    logiczna
    znak
}
<expr> → {
    <numerical_expr>
    <boolean_expr>
}
<array_expr> → {
    <expr>*
}
<numerical_expr> → {
    <int_term>
    <arithmetic_expr>
}
<int_term> → {
    [integer_literal]
    `identifier`
}
<boolean_expr> → {
    <boolean_term>
    <boolean_math_expr>
    <boolean_logic_expr>
}

<boolean_term> → {
    <boolean_literal>
    `identifier`
}
<boolean_literal> → {
    prawda
    fałsz
}
<arithmetic_expr> → {
    (5) <numerical_expr> modulo <numerical_expr>
    (5) <numerical_expr> razy <numerical_expr>
    (5) <numerical_expr> podzielić <numerical_expr>
    (4) <numerical_expr> dodać <numerical_expr>
    (4) <numerical_expr> odjąć <numerical_expr>
}
<boolean_math_expr> → {
    (3) <numerical_expr> równe <numerical_expr>
    (3) <numerical_expr> różne <numerical_expr>
    (3) <numerical_expr> większe od <numerical_expr>
    (3) <numerical_expr> większe równe <numerical_expr>
    (3) <numerical_expr> mniejsze <numerical_expr>
    (3) <numerical_expr> mniejsze równe <numerical_expr>
}
<boolean_logic_expr> → {
    (2) nie <boolean_expr>
    (1) <boolean_expr> oraz <boolean_expr>
    (1) <boolean_expr> lub <boolean_expr>
}
```

## Code example //TODO

```
zmienna całkowita `a1` równa [dwa tysiące dwadzieścia cztery]

jeśliby `a1` było równe [pięć] to:
§1 wyświetlić ( "Tak było równo" )
przeciwnie:
§1 kończwaść [jeden]
```