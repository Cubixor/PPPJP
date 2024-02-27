# PPPJP dictionary

```
<program> → <statement>* 
<statement> → { 
    <function>
    <scope>
    zmienna <type> `ident` równa <expr> 
    `ident` równa <expr>
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
}
<expr> → {
    <numerical_expr>
    <boolean_expr>
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
    (1) <numerical_expr> modulo <numerical_expr>
    (1) <numerical_expr> razy <numerical_expr>
    (1) <numerical_expr> podzielić <numerical_expr>
    (0) <numerical_expr> dodać <numerical_expr>
    (0) <numerical_expr> odjąć <numerical_expr>
}
<boolean_math_expr> → {
    <numerical_expr> równe <numerical_expr>
    <numerical_expr> różne <numerical_expr>
    <numerical_expr> większe od <numerical_expr>
    <numerical_expr> większe równe <numerical_expr>
    <numerical_expr> mniejsze <numerical_expr>
    <numerical_expr> mniejsze równe <numerical_expr>
}
<boolean_logic_expr> → {
    <boolean_expr> oraz <boolean_expr>
    <boolean_expr> lub <boolean_expr>
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