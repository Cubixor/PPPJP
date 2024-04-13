# PPPJP dictionary

```
<program> → <statement>* 
<statement> → { 
    <function>
    <scope>
    zmienna <type> `ident` równa <expr> 
    tablica <type> `ident` rozmiaru <int_expr>
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
    kończwaść ( <int_expr> )
    wyświetl_liczbę ( <int_expr> )
    wyświetl_znak ( <char_expr> )
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
    <int_expr>
    <boolean_expr>
    <char_expr>
}
<array_expr> → {
    {<expr>, <expr>...}
}
<int_expr> → {
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
    (5) <int_expr> modulo <int_expr>
    (5) <int_expr> razy <int_expr>
    (5) <int_expr> podzielić <int_expr>
    (4) <int_expr> dodać <int_expr>
    (4) <int_expr> odjąć <int_expr>
}
<boolean_math_expr> → {
    (3) <int_expr> równe <int_expr>
    (3) <int_expr> różne <int_expr>
    (3) <int_expr> większe od <int_expr>
    (3) <int_expr> większe równe <int_expr>
    (3) <int_expr> mniejsze <int_expr>
    (3) <int_expr> mniejsze równe <int_expr>
}
<boolean_logic_expr> → {
    (2) nie <boolean_expr>
    (1) <boolean_expr> oraz <boolean_expr>
    (1) <boolean_expr> lub <boolean_expr>
}
<char_term> → {
    `identifier`
    '<ascii_char>'
}